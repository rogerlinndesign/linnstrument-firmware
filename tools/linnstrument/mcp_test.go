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
	"bytes"
	"encoding/base64"
	"encoding/json"
	"io"
	"os"
	"path/filepath"
	"strconv"
	"strings"
	"sync"
	"testing"
	"time"
)

// fakeApp answers the --script protocol the way the Virtual LinnStrument does and records
// every command it was given.
type fakeApp struct {
	mu       sync.Mutex
	commands []string
	reader   *io.PipeReader
	writer   *io.PipeWriter
}

func (f *fakeApp) given() []string {
	f.mu.Lock()
	defer f.mu.Unlock()
	return append([]string(nil), f.commands...)
}

func newFakeApp(t *testing.T) (*fakeApp, *virtualApp) {
	t.Helper()
	commandsIn, commandsOut := io.Pipe()
	repliesIn, repliesOut := io.Pipe()
	fake := &fakeApp{reader: commandsIn, writer: repliesOut}
	go func() {
		scanner := bufio.NewScanner(commandsIn)
		for scanner.Scan() {
			line := scanner.Text()
			fake.mu.Lock()
			fake.commands = append(fake.commands, line)
			fake.mu.Unlock()
			words := strings.Fields(line)
			switch words[0] {
			case "leds":
				io.WriteString(repliesOut, "7: 0 1 2\n6: 0 0 0\nok\n")
			case "status":
				io.WriteString(repliesOut, "LinnStrument 200, firmware 240.075\nok\n")
			case "received":
				io.WriteString(repliesOut, "ok\n")
			case "send":
				if words[1] == "zz" {
					io.WriteString(repliesOut, "error: zz is not a byte in hex\n")
				} else {
					io.WriteString(repliesOut, "ok\n")
				}
			case "screenshot":
				path := strings.Trim(strings.TrimSuffix(line[len("screenshot "):], " 1"), "\"")
				os.WriteFile(path, []byte("\x89PNG\r\n\x1a\nfake"), 0o644)
				io.WriteString(repliesOut, "ok\n")
			case "quit":
				io.WriteString(repliesOut, "ok\n")
				repliesOut.Close()
				return
			default:
				io.WriteString(repliesOut, "ok\n")
			}
		}
	}()
	return fake, &virtualApp{in: commandsOut, out: bufio.NewReader(repliesIn)}
}

func TestVirtualAppCommandReadsUntilOkOrError(t *testing.T) {
	fake, app := newFakeApp(t)
	reply, err := app.command("leds")
	if err != nil || reply != "7: 0 1 2\n6: 0 0 0" {
		t.Fatalf("leds: %q, %v", reply, err)
	}
	reply, err = app.command("press 3 4")
	if err != nil || reply != "" {
		t.Fatalf("press: %q, %v", reply, err)
	}
	if _, err = app.command("send zz"); err == nil || err.Error() != "zz is not a byte in hex" {
		t.Fatalf("send zz: %v", err)
	}
	if len(fake.given()) != 3 || fake.given()[1] != "press 3 4" {
		t.Fatalf("commands %v", fake.given())
	}
}

type rpcReply struct {
	ID     json.RawMessage `json:"id"`
	Result struct {
		Tools           []map[string]any `json:"tools"`
		Content         []map[string]any `json:"content"`
		IsError         bool             `json:"isError"`
		ProtocolVersion string           `json:"protocolVersion"`
		ServerInfo      struct {
			Version string `json:"version"`
		} `json:"serverInfo"`
	} `json:"result"`
	Error *jsonRpcError `json:"error"`
}

func runMcp(t *testing.T, requests ...string) ([]rpcReply, *fakeApp) {
	t.Helper()
	fake, app := newFakeApp(t)
	var out bytes.Buffer
	server := &mcpServer{out: &out, tools: mcpTools(), start: func() (*virtualApp, error) { return app, nil }}
	server.serve(strings.NewReader(strings.Join(requests, "\n") + "\n"))
	var replies []rpcReply
	for _, line := range strings.Split(strings.TrimSpace(out.String()), "\n") {
		var reply rpcReply
		if err := json.Unmarshal([]byte(line), &reply); err != nil {
			t.Fatalf("not JSON-RPC: %s", line)
		}
		replies = append(replies, reply)
	}
	return replies, fake
}

func call(id int, name string, args string) string {
	return `{"jsonrpc":"2.0","id":` + itoa(id) + `,"method":"tools/call","params":{"name":"` + name + `","arguments":` + args + `}}`
}

func itoa(n int) string { return strconv.Itoa(n) }

func TestMcpSessionTurnsToolCallsIntoAppCommands(t *testing.T) {
	replies, fake := runMcp(t,
		`{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2024-11-05"}}`,
		`{"jsonrpc":"2.0","method":"notifications/initialized"}`,
		`{"jsonrpc":"2.0","id":2,"method":"tools/list"}`,
		call(3, "press", `{"col":3,"row":4,"z":900}`),
		call(4, "move", `{"col":3,"row":4,"x":0.25,"y":1}`),
		call(5, "leds", `{}`),
		call(6, "send_midi", `{"bytes":" B0 14  05 "}`),
		call(7, "send_midi", `{"bytes":"zz"}`),
		call(8, "foot", `{"switch":"left","down":true}`),
		call(9, "received_midi", `{"wait_seconds":0.5}`),
		call(10, "slide", `{"col":3,"row":4,"to_col":4,"y":0.25}`),
	)
	if len(replies) != 10 {
		t.Fatalf("%d replies", len(replies))
	}
	if replies[0].Result.ProtocolVersion != "2024-11-05" {
		t.Fatalf("protocol version %q", replies[0].Result.ProtocolVersion)
	}
	if len(replies[1].Result.Tools) != len(mcpTools()) || replies[1].Result.Tools[0]["name"] != "press" {
		t.Fatalf("tools %v", replies[1].Result.Tools)
	}
	want := []string{"press 3 4 0.5 0.5 900", "move 3 4 0.25 1", "leds", "send B0 14 05", "send zz", "foot left down", "received 0.5", "slide 3 4 4 0.25", "quit"}
	if strings.Join(fake.given(), "|") != strings.Join(want, "|") {
		t.Fatalf("commands %v", fake.given())
	}
	if replies[4].Result.Content[0]["text"] != "7: 0 1 2\n6: 0 0 0" {
		t.Fatalf("leds reply %v", replies[4].Result.Content)
	}
	if !replies[6].Result.IsError || replies[6].Result.Content[0]["text"] != "zz is not a byte in hex" {
		t.Fatalf("bad bytes reply %v", replies[6].Result)
	}
	if replies[8].Result.Content[0]["text"] != "no MIDI received" {
		t.Fatalf("received reply %v", replies[8].Result.Content)
	}
}

func TestMcpScreenshotReturnsAnImage(t *testing.T) {
	replies, fake := runMcp(t, call(1, "screenshot", `{}`))
	content := replies[0].Result.Content[0]
	data, err := base64.StdEncoding.DecodeString(content["data"].(string))
	if err != nil || content["type"] != "image" || content["mimeType"] != "image/png" || !bytes.HasPrefix(data, []byte("\x89PNG")) {
		t.Fatalf("screenshot content %v", content)
	}
	path := strings.Trim(strings.TrimSuffix(strings.TrimPrefix(fake.given()[0], "screenshot "), " 1"), "\"")
	if _, err := os.Stat(path); !os.IsNotExist(err) {
		t.Fatalf("the screenshot file %s was left behind", path)
	}
}

func TestMcpRejectsWhatItCannotDo(t *testing.T) {
	replies, fake := runMcp(t,
		`{"jsonrpc":"2.0","id":1,"method":"nope"}`,
		call(2, "unknown", `{}`),
		call(3, "press", `{"row":4}`),
		call(4, "foot", `{"switch":"middle","down":true}`),
		`not json`,
	)
	if replies[0].Error == nil || replies[0].Error.Code != -32601 {
		t.Fatalf("unknown method: %v", replies[0].Error)
	}
	if replies[1].Error == nil || replies[1].Error.Code != -32602 {
		t.Fatalf("unknown tool: %v", replies[1].Error)
	}
	if !replies[2].Result.IsError || !replies[3].Result.IsError {
		t.Fatalf("bad arguments were accepted: %v %v", replies[2].Result, replies[3].Result)
	}
	if replies[4].Error == nil || replies[4].Error.Code != -32700 {
		t.Fatalf("parse error: %v", replies[4].Error)
	}
	if strings.Join(fake.given(), "|") != "quit" {
		t.Fatalf("the app was asked something: %v", fake.given())
	}
}

func TestMcpReportsTheBuildOfTheTool(t *testing.T) {
	kept := version
	defer func() { version = kept }()
	version = "240.075-1a2b3c4"
	replies, _ := runMcp(t, `{"jsonrpc":"2.0","id":1,"method":"initialize","params":{}}`)
	if got := replies[0].Result.ServerInfo.Version; got != "240.075-1a2b3c4" {
		t.Fatalf("server version %q", got)
	}
}

// restartingServer hands out a new fake app on every start, so that a test sees each one
func restartingServer(t *testing.T, prepare func(*virtualApp)) (*mcpServer, *[]*fakeApp) {
	fakes := &[]*fakeApp{}
	server := &mcpServer{out: io.Discard, tools: mcpTools(), start: func() (*virtualApp, error) {
		fake, app := newFakeApp(t)
		if prepare != nil {
			prepare(app)
		}
		*fakes = append(*fakes, fake)
		return app, nil
	}}
	return server, fakes
}

func statusTool(t *testing.T, server *mcpServer) mcpTool {
	for _, tool := range server.tools {
		if tool.Name == "status" {
			return tool
		}
	}
	t.Fatal("no status tool")
	return mcpTool{}
}

func TestMcpStartsTheAppAgainAfterItQuit(t *testing.T) {
	server, fakes := restartingServer(t, nil)
	status := statusTool(t, server)
	if _, err := server.call(status, map[string]any{}); err != nil {
		t.Fatal(err)
	}
	// what closing the window does: the app reads and answers nothing more
	(*fakes)[0].reader.Close()
	(*fakes)[0].writer.Close()
	content, err := server.call(status, map[string]any{})
	if err != nil {
		t.Fatalf("after the app quit: %v", err)
	}
	if len(*fakes) != 2 || len((*fakes)[1].given()) != 1 || (*fakes)[1].given()[0] != "status" {
		t.Fatalf("the status call did not reach a new app: %d apps", len(*fakes))
	}
	if !strings.Contains(content[0]["text"].(string), "had quit") {
		t.Fatalf("no word of the restart: %v", content)
	}
}

func TestMcpStartsTheNewBuildAfterARebuild(t *testing.T) {
	binary := filepath.Join(t.TempDir(), "Virtual LinnStrument")
	if err := os.WriteFile(binary, nil, 0o755); err != nil {
		t.Fatal(err)
	}
	built := time.Now().Add(-time.Hour)
	os.Chtimes(binary, built, built)
	server, fakes := restartingServer(t, func(app *virtualApp) {
		info, _ := os.Stat(binary)
		app.binary, app.built = binary, info.ModTime()
	})
	status := statusTool(t, server)
	if _, err := server.call(status, map[string]any{}); err != nil {
		t.Fatal(err)
	}
	rebuilt := built.Add(time.Minute)
	os.Chtimes(binary, rebuilt, rebuilt)
	content, err := server.call(status, map[string]any{})
	if err != nil {
		t.Fatal(err)
	}
	if first := (*fakes)[0].given(); len(*fakes) != 2 || first[len(first)-1] != "quit" {
		t.Fatalf("the old build was not replaced: %d apps, %v", len(*fakes), first)
	}
	if !strings.Contains(content[0]["text"].(string), "rebuilt") {
		t.Fatalf("no word of the rebuild: %v", content)
	}
	if _, err := server.call(status, map[string]any{}); err != nil || len(*fakes) != 2 {
		t.Fatalf("restarted again without a rebuild: %d apps, %v", len(*fakes), err)
	}
}

func TestMcpToolSchemasListRequiredAsAnArray(t *testing.T) {
	replies, _ := runMcp(t, `{"jsonrpc":"2.0","id":1,"method":"tools/list"}`)
	for _, tool := range replies[0].Result.Tools {
		schema := tool["inputSchema"].(map[string]any)
		if _, ok := schema["required"].([]any); !ok {
			t.Errorf("%v: required is %v, not an array", tool["name"], schema["required"])
		}
	}
}
