// Copyright 2026 Roger Linn Design (https://www.rogerlinndesign.com)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package main

import (
	"bufio"
	"encoding/base64"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"os"
	"os/exec"
	"os/signal"
	"path/filepath"
	"runtime"
	"strconv"
	"strings"
	"syscall"
	"time"
)

// The Virtual LinnStrument as an MCP server over stdio: an AI assistant gets the app's text
// commands as tools (touches, lights, screenshots, the model) and MIDI both ways, without a
// MIDI port or a pipe of its own. The app is started on the first tool call and told to quit
// when the client goes away.

const mcpProtocolVersion = "2025-06-18"

var mcpProtocolVersions = map[string]bool{"2024-11-05": true, "2025-03-26": true, "2025-06-18": true}

// virtualApp talks the --script protocol: one command per line, the reply is its output
// followed by "ok", or one line starting with "error:".
type virtualApp struct {
	in   io.Writer
	out  *bufio.Reader
	done func() // ends the process after quit, nil for a fake
}

func (a *virtualApp) command(line string) (string, error) {
	if _, err := io.WriteString(a.in, line+"\n"); err != nil {
		return "", fmt.Errorf("the app is gone: %w", err)
	}
	var lines []string
	for {
		text, err := a.out.ReadString('\n')
		if err != nil {
			return "", fmt.Errorf("the app stopped answering: %w", err)
		}
		text = strings.TrimRight(text, "\r\n")
		if text == "ok" {
			return strings.Join(lines, "\n"), nil
		}
		if strings.HasPrefix(text, "error:") {
			return "", errors.New(strings.TrimSpace(strings.TrimPrefix(text, "error:")))
		}
		lines = append(lines, text)
	}
}

// quit waits for the app's answer, so the command has been read before the process is reaped
func (a *virtualApp) quit() {
	_, _ = a.command("quit")
	if a.done != nil {
		a.done()
	}
}

// virtualBinary is the app the build made, or the one LINN_VIRTUAL names.
func virtualBinary(repo string) (string, error) {
	if path := os.Getenv("LINN_VIRTUAL"); path != "" {
		return path, nil
	}
	dir := filepath.Join(repo, "build", "virtual", "VirtualLinnStrument_artefacts", "Release")
	var path string
	switch runtime.GOOS {
	case "darwin":
		path = filepath.Join(dir, "Virtual LinnStrument.app", "Contents", "MacOS", "Virtual LinnStrument")
	case "windows":
		path = filepath.Join(dir, "Virtual LinnStrument.exe")
	default:
		path = filepath.Join(dir, "Virtual LinnStrument")
	}
	if _, err := os.Stat(path); err != nil {
		return "", fmt.Errorf("no Virtual LinnStrument at %s: build it with cmake as tools/virtual/README.md says, or set LINN_VIRTUAL", path)
	}
	return path, nil
}

func startVirtualApp(repo string, model string) (*virtualApp, error) {
	binary, err := virtualBinary(repo)
	if err != nil {
		return nil, err
	}
	cmd := exec.Command(binary, "--script", "-", "--model", model)
	cmd.Stderr = os.Stderr
	in, err := cmd.StdinPipe()
	if err != nil {
		return nil, err
	}
	out, err := cmd.StdoutPipe()
	if err != nil {
		return nil, err
	}
	if err := cmd.Start(); err != nil {
		return nil, fmt.Errorf("cannot start %s: %w (only one Virtual LinnStrument runs at a time, quit any other first)", binary, err)
	}
	app := &virtualApp{in: in, out: bufio.NewReader(out)}
	app.done = func() {
		in.Close()
		exited := make(chan struct{})
		go func() { _ = cmd.Wait(); close(exited) }()
		select {
		case <-exited:
		case <-time.After(5 * time.Second):
			_ = cmd.Process.Kill()
		}
	}
	// the first command waits for the firmware to boot, so a reply proves the app is up; a
	// second instance exits at once, which reads as the app stopping
	if _, err := app.command("status"); err != nil {
		app.quit()
		return nil, fmt.Errorf("%w; a Virtual LinnStrument that is already running has to be quit first", err)
	}
	return app, nil
}

type mcpTool struct {
	Name        string          `json:"name"`
	Description string          `json:"description"`
	InputSchema json.RawMessage `json:"inputSchema"`
	run         func(app *virtualApp, args map[string]any) ([]map[string]any, error)
}

func schema(required []string, properties string) json.RawMessage {
	req, _ := json.Marshal(required)
	return json.RawMessage(`{"type":"object","properties":{` + properties + `},"required":` + string(req) + `}`)
}

const (
	colProperty = `"col":{"type":"integer","description":"column, 1 to 25 on a 200 and 1 to 16 on a 128; 0 is the control column"}`
	rowProperty = `"row":{"type":"integer","description":"row, 0 at the bottom to 7 at the top"}`
	xyProperty  = `"x":{"type":"number","description":"position inside the cell, 0 left to 1 right (default 0.5)"},"y":{"type":"number","description":"position inside the cell, 0 bottom to 1 top (default 0.5)"}`
	zProperty   = `"z":{"type":"integer","description":"pressure 1 to 4095 (default: the app's pressure slider, a medium touch)"}`
)

func number(args map[string]any, name string) (string, bool) {
	value, ok := args[name]
	if !ok || value == nil {
		return "", false
	}
	switch v := value.(type) {
	case float64:
		return strconv.FormatFloat(v, 'f', -1, 64), true
	case bool:
		if v {
			return "1", true
		}
		return "0", true
	case string:
		return v, true
	}
	return "", false
}

func text(s string) []map[string]any {
	if s == "" {
		s = "ok"
	}
	return []map[string]any{{"type": "text", "text": s}}
}

// cellCommand builds "name COL ROW ..." from the arguments, in the order the app wants them,
// stopping at the first optional one that is missing.
func cellCommand(name string, args map[string]any, optional ...string) (string, error) {
	col, okCol := number(args, "col")
	row, okRow := number(args, "row")
	if !okCol || !okRow {
		return "", errors.New("col and row are required")
	}
	parts := []string{name, col, row}
	for _, field := range optional {
		value, ok := number(args, field)
		if !ok {
			break
		}
		parts = append(parts, value)
	}
	return strings.Join(parts, " "), nil
}

func mcpTools() []mcpTool {
	simple := func(build func(args map[string]any) (string, error)) func(*virtualApp, map[string]any) ([]map[string]any, error) {
		return func(app *virtualApp, args map[string]any) ([]map[string]any, error) {
			line, err := build(args)
			if err != nil {
				return nil, err
			}
			reply, err := app.command(line)
			if err != nil {
				return nil, err
			}
			return text(reply), nil
		}
	}
	return []mcpTool{
		{Name: "press", Description: "Touch a cell of the playing surface and hold it. The firmware sees the pressure rise over a few scans, so it measures a velocity. A press on a held cell keeps the touch.",
			InputSchema: schema([]string{"col", "row"}, colProperty+","+rowProperty+","+xyProperty+","+zProperty),
			run: simple(func(args map[string]any) (string, error) {
				// the app takes X Y before Z, so a pressure alone needs the default position
				for _, field := range []string{"x", "y", "z"} {
					if _, ok := number(args, field); ok {
						for _, axis := range []string{"x", "y"} {
							if _, ok := number(args, axis); !ok {
								args[axis] = 0.5
							}
						}
						break
					}
				}
				return cellCommand("press", args, "x", "y", "z")
			})},
		{Name: "move", Description: "Move a held touch inside its cell, for pitch bend (x), timbre (y) or a pressure change (z). The touch keeps its pressure unless z is given.",
			InputSchema: schema([]string{"col", "row", "x", "y"}, colProperty+","+rowProperty+","+xyProperty+","+zProperty),
			run:         simple(func(args map[string]any) (string, error) { return cellCommand("move", args, "x", "y", "z") })},
		{Name: "release", Description: "Lift a held touch.",
			InputSchema: schema([]string{"col", "row"}, colProperty+","+rowProperty),
			run:         simple(func(args map[string]any) (string, error) { return cellCommand("release", args) })},
		{Name: "tap", Description: "Press a cell and release it after ms milliseconds (default 120, at least 20). The tool returns after the release.",
			InputSchema: schema([]string{"col", "row"}, colProperty+","+rowProperty+`,"ms":{"type":"integer","description":"how long the touch lasts in milliseconds"}`),
			run:         simple(func(args map[string]any) (string, error) { return cellCommand("tap", args, "ms") })},
		{Name: "foot", Description: "Press or release a foot switch.",
			InputSchema: schema([]string{"switch", "down"}, `"switch":{"type":"string","enum":["left","right"]},"down":{"type":"boolean","description":"true presses, false releases"}`),
			run: simple(func(args map[string]any) (string, error) {
				side, _ := args["switch"].(string)
				down, ok := args["down"].(bool)
				if (side != "left" && side != "right") || !ok {
					return "", errors.New("switch is left or right, down is true or false")
				}
				action := "up"
				if down {
					action = "down"
				}
				return "foot " + side + " " + action, nil
			})},
		{Name: "leds", Description: "The lights of every cell as one line per row, the top row first: \"ROW: c c c ...\" with a color number per column from the control column at 0. Colors: 0 off, 1 red, 2 yellow, 3 green, 4 cyan, 5 blue, 6 magenta, 7 black, 8 white, 9 orange, 10 lime, 11 pink.",
			InputSchema: schema(nil, ""),
			run:         simple(func(map[string]any) (string, error) { return "leds", nil })},
		{Name: "status", Description: "The instrument's status line: model, firmware version, the virtual MIDI port, calibration, user firmware mode and serial mode.",
			InputSchema: schema(nil, ""),
			run:         simple(func(map[string]any) (string, error) { return "status", nil })},
		{Name: "screenshot", Description: "A picture of the app's window: the lights, the touches and the control column.",
			InputSchema: schema(nil, ""),
			run: func(app *virtualApp, args map[string]any) ([]map[string]any, error) {
				file, err := os.CreateTemp("", "virtual-linnstrument-*.png")
				if err != nil {
					return nil, err
				}
				path := file.Name()
				file.Close()
				defer os.Remove(path)
				if _, err := app.command("screenshot \"" + path + "\" 1"); err != nil {
					return nil, err
				}
				data, err := os.ReadFile(path)
				if err != nil {
					return nil, err
				}
				return []map[string]any{{"type": "image", "data": base64.StdEncoding.EncodeToString(data), "mimeType": "image/png"}}, nil
			}},
		{Name: "model", Description: "Restart the app as the other model, a LinnStrument 200 (25 playing columns) or 128 (16). Settings are kept per model. The virtual MIDI port is recreated, so a listener on it has to reconnect.",
			InputSchema: schema([]string{"model"}, `"model":{"type":"integer","enum":[200,128]}`),
			run: simple(func(args map[string]any) (string, error) {
				model, ok := number(args, "model")
				if !ok {
					return "", errors.New("model is 200 or 128")
				}
				return "model " + model, nil
			})},
		{Name: "send_midi", Description: "Send MIDI into the firmware, as if it arrived on the instrument's USB port: bytes in hex separated by spaces, like \"B0 14 05\" for CC 20 value 5 on channel 1. Several messages can be sent at once.",
			InputSchema: schema([]string{"bytes"}, `"bytes":{"type":"string","description":"the raw MIDI bytes in hex, like \"90 3C 64\""}`),
			run: simple(func(args map[string]any) (string, error) {
				bytes, _ := args["bytes"].(string)
				if strings.TrimSpace(bytes) == "" {
					return "", errors.New("bytes are required")
				}
				return "send " + strings.Join(strings.Fields(bytes), " "), nil
			})},
		{Name: "received_midi", Description: "The MIDI the firmware sent since the previous call, one message per line in hex (the newest 4096 are kept). With wait_seconds it waits up to that long for a first message, which is how to catch the answer to a query or the notes of a tap.",
			InputSchema: schema(nil, `"wait_seconds":{"type":"number","description":"how long to wait for a first message (default 0)"}`),
			run: func(app *virtualApp, args map[string]any) ([]map[string]any, error) {
				line := "received"
				if wait, ok := number(args, "wait_seconds"); ok {
					line += " " + wait
				}
				reply, err := app.command(line)
				if err != nil {
					return nil, err
				}
				if reply == "" {
					reply = "no MIDI received"
				}
				return text(reply), nil
			}},
		{Name: "wait", Description: "Let time pass, for the firmware to act on what was sent or for a held touch to last.",
			InputSchema: schema([]string{"seconds"}, `"seconds":{"type":"number"}`),
			run: simple(func(args map[string]any) (string, error) {
				seconds, ok := number(args, "seconds")
				if !ok {
					return "", errors.New("seconds is required")
				}
				return "sleep " + seconds, nil
			})},
	}
}

const mcpInstructions = "The Virtual LinnStrument runs the real LinnStrument firmware on this computer behind a window " +
	"and a virtual MIDI port named \"Virtual LinnStrument\". Touch it with press, move, release and tap " +
	"(columns from 1, rows from 0 at the bottom), read what it shows with leds or screenshot, send it MIDI " +
	"with send_midi and read what it sent with received_midi. Everything goes through the firmware, so the " +
	"MIDI is what an instrument would send. Settings persist between runs; the control column (column 0) " +
	"opens the settings screens the way it does on the instrument."

type jsonRpcRequest struct {
	JSONRPC string          `json:"jsonrpc"`
	ID      json.RawMessage `json:"id"`
	Method  string          `json:"method"`
	Params  json.RawMessage `json:"params"`
}

type jsonRpcError struct {
	Code    int    `json:"code"`
	Message string `json:"message"`
}

// mcpServer answers one client on in/out; start makes the app the first tool call needs.
type mcpServer struct {
	out   io.Writer
	start func() (*virtualApp, error)
	app   *virtualApp
	tools []mcpTool
}

func (s *mcpServer) reply(id json.RawMessage, result any, rpcErr *jsonRpcError) {
	if len(id) == 0 {
		return
	}
	message := map[string]any{"jsonrpc": "2.0", "id": id}
	if rpcErr != nil {
		message["error"] = rpcErr
	} else {
		message["result"] = result
	}
	data, _ := json.Marshal(message)
	_, _ = s.out.Write(append(data, '\n'))
}

func (s *mcpServer) handle(request jsonRpcRequest) {
	switch request.Method {
	case "initialize":
		var params struct {
			ProtocolVersion string `json:"protocolVersion"`
		}
		_ = json.Unmarshal(request.Params, &params)
		version := mcpProtocolVersion
		if mcpProtocolVersions[params.ProtocolVersion] {
			version = params.ProtocolVersion
		}
		s.reply(request.ID, map[string]any{
			"protocolVersion": version,
			"capabilities":    map[string]any{"tools": map[string]any{}},
			"serverInfo":      map[string]any{"name": "virtual-linnstrument", "version": "1.0"},
			"instructions":    mcpInstructions,
		}, nil)
	case "ping":
		s.reply(request.ID, map[string]any{}, nil)
	case "tools/list":
		list := make([]map[string]any, 0, len(s.tools))
		for _, tool := range s.tools {
			list = append(list, map[string]any{"name": tool.Name, "description": tool.Description, "inputSchema": tool.InputSchema})
		}
		s.reply(request.ID, map[string]any{"tools": list}, nil)
	case "tools/call":
		var params struct {
			Name      string         `json:"name"`
			Arguments map[string]any `json:"arguments"`
		}
		if err := json.Unmarshal(request.Params, &params); err != nil {
			s.reply(request.ID, nil, &jsonRpcError{Code: -32602, Message: "invalid params: " + err.Error()})
			return
		}
		if params.Arguments == nil {
			params.Arguments = map[string]any{}
		}
		for _, tool := range s.tools {
			if tool.Name != params.Name {
				continue
			}
			content, err := s.call(tool, params.Arguments)
			if err != nil {
				s.reply(request.ID, map[string]any{"content": text(err.Error()), "isError": true}, nil)
				return
			}
			s.reply(request.ID, map[string]any{"content": content}, nil)
			return
		}
		s.reply(request.ID, nil, &jsonRpcError{Code: -32602, Message: "unknown tool " + params.Name})
	default:
		if strings.HasPrefix(request.Method, "notifications/") {
			return
		}
		s.reply(request.ID, nil, &jsonRpcError{Code: -32601, Message: "method not found: " + request.Method})
	}
}

func (s *mcpServer) call(tool mcpTool, args map[string]any) ([]map[string]any, error) {
	if s.app == nil {
		app, err := s.start()
		if err != nil {
			return nil, err
		}
		s.app = app
	}
	return tool.run(s.app, args)
}

// serve reads newline-delimited JSON-RPC until in ends, then quits the app.
func (s *mcpServer) serve(in io.Reader) {
	scanner := bufio.NewScanner(in)
	scanner.Buffer(make([]byte, 1024*1024), 16*1024*1024)
	for scanner.Scan() {
		line := strings.TrimSpace(scanner.Text())
		if line == "" {
			continue
		}
		var request jsonRpcRequest
		if err := json.Unmarshal([]byte(line), &request); err != nil {
			s.reply(json.RawMessage("null"), nil, &jsonRpcError{Code: -32700, Message: "parse error: " + err.Error()})
			continue
		}
		s.handle(request)
	}
	if s.app != nil {
		s.app.quit()
		s.app = nil
	}
}

func serveMcp(repo string, model string, in io.Reader, out io.Writer) error {
	if model != "200" && model != "128" {
		return errors.New("the model is 200 or 128")
	}
	server := &mcpServer{out: out, tools: mcpTools(), start: func() (*virtualApp, error) { return startVirtualApp(repo, model) }}
	// a client that is killed takes the server with it; the app must not stay behind
	signals := make(chan os.Signal, 1)
	signal.Notify(signals, os.Interrupt, syscall.SIGTERM)
	go func() {
		<-signals
		if server.app != nil {
			server.app.quit()
		}
		os.Exit(0)
	}()
	server.serve(in)
	return nil
}
