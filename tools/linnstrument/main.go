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

// linnstrument is the command line companion for LinnStrument firmware development: it builds
// the firmware and runs the desktop tests, flashes a connected instrument, speaks the Updater's
// serial protocol to back up and restore settings, and serves the browser pages in examples/browser/.
// Run it from the repository root:
//
//	go run ./tools/linnstrument <command>
package main

import (
	"encoding/json"
	"flag"
	"fmt"
	"os"
	"path/filepath"
	"strconv"
	"time"

	"go.bug.st/serial/enumerator"
)

const usage = `Usage: go run ./tools/linnstrument [-port PORT] <command> [arguments]

Commands:
  build                  compile the firmware into build/ and report flash and RAM use
  test [NAME]            build and run the desktop unit tests in tests/ (needs a C++ compiler),
                         NAME restricts the run to tests whose name contains it
  mutate [NAME]          prove the tests can fail: apply each change in tests/mutations.tsv to the
                         firmware and check the named test fails, restoring the file afterwards
  merge [FILE]           write the sketch as the one C++ file the desktop builds compile
                         (default: build/sketch/linnstrument-firmware.cpp)
  nrpns [FILE]           write the NRPN table of midi.md as the JavaScript the settings editor
                         reads (default: examples/browser/nrpn_parameters.js)
  serve [PORT]           serve the browser pages in examples/browser/ over http (default port 8765, 0 picks
                         a free one) and open them, unless LINN_NO_BROWSER is set; served from one
                         origin, the browser remembers the MIDI permission
  mcp [200|128]          serve the Virtual LinnStrument to an AI assistant as an MCP server over
                         stdio: touches, lights, screenshots and MIDI both ways (needs the app
                         built as tools/virtual/README.md says, or LINN_VIRTUAL naming it)
  ports                  list serial ports and mark the LinnStrument
  probe                  handshake with the instrument and report its settings layout version
  led COL ROW COLOR      light one LED (color numbers as in linnstrument-firmware.ino)
  backup [FILE]          save settings and sequencer projects to a file (default: linnstrument-backup-<time>.json)
  restore FILE           send a backup back; the instrument then leaves serial mode
  flash [FIRMWARE.bin]   flash build/linnstrument-firmware.ino.bin; ERASES settings and calibration
  update [FIRMWARE.bin]  backup, flash, restore: what the LinnStrument Updater does
  serialmode             switch a unit in MIDI mode to serial mode (needs sendmidi and firmware with NRPN 271)
  midimode               switch a unit in serial mode back to MIDI mode (firmware with the m command)

The instrument must be in serial mode (OS Update in Global Settings). -port overrides the
automatic detection by USB id. The backup, restore and update commands need firmware 2.0 or later.
`

type Backup struct {
	Created         time.Time `json:"created"`
	SettingsVersion int       `json:"settingsVersion"`
	Settings        []byte    `json:"settings"`
	Projects        [][]byte  `json:"projects"`
}

func main() {
	flag.Usage = func() { fmt.Fprint(os.Stderr, usage) }
	port := flag.String("port", "", "serial port of the LinnStrument")
	flag.Parse()
	args := flag.Args()
	if len(args) == 0 {
		flag.Usage()
		os.Exit(2)
	}
	if err := dispatch(*port, args[0], args[1:]); err != nil {
		fmt.Fprintln(os.Stderr, "error:", err)
		os.Exit(1)
	}
}

func dispatch(port, command string, args []string) error {
	switch command {
	case "build":
		repo, err := repoRoot()
		if err != nil {
			return err
		}
		return build(repo)
	case "test":
		repo, err := repoRoot()
		if err != nil {
			return err
		}
		filter := ""
		if len(args) > 0 {
			filter = args[0]
		}
		return runTests(repo, filter)
	case "merge":
		repo, err := repoRoot()
		if err != nil {
			return err
		}
		out := filepath.Join(repo, "build", "sketch", sketchName+".cpp")
		if len(args) > 0 {
			out = args[0]
		}
		return writeMergedSketch(repo, out)
	case "nrpns":
		repo, err := repoRoot()
		if err != nil {
			return err
		}
		out := filepath.Join(repo, "examples", "browser", "nrpn_parameters.js")
		if len(args) > 0 {
			out = args[0]
		}
		return writeNrpnTable(repo, out)
	case "serve":
		repo, err := repoRoot()
		if err != nil {
			return err
		}
		listen := "8765"
		if len(args) > 0 {
			listen = args[0]
		}
		return serveExamples(repo, listen, os.Getenv("LINN_NO_BROWSER") == "")
	case "mcp":
		repo, err := repoRoot()
		if err != nil {
			return err
		}
		model := "200"
		if len(args) > 0 {
			model = args[0]
		}
		return serveMcp(repo, model, os.Stdin, os.Stdout)
	case "mutate":
		repo, err := repoRoot()
		if err != nil {
			return err
		}
		only := ""
		if len(args) > 0 {
			only = args[0]
		}
		return runMutations(repo, only)
	case "ports":
		return listPorts()
	case "serialmode":
		return enterSerialMode(port)
	case "midimode":
		return enterMidiMode(port)
	case "probe":
		return probe(port)
	case "led":
		if len(args) != 3 {
			return fmt.Errorf("led needs COL ROW COLOR")
		}
		var v [3]byte
		for i, a := range args {
			n, err := strconv.Atoi(a)
			if err != nil || n < 0 || n > 255 {
				return fmt.Errorf("led: %q is not a byte value", a)
			}
			v[i] = byte(n)
		}
		s, err := openSession(port, 3)
		if err != nil {
			return err
		}
		defer s.Close()
		if err := s.LightLed(v[0], v[1], v[2]); err != nil {
			return err
		}
		fmt.Printf("lit column %d row %d with color %d\n", v[0], v[1], v[2])
		return nil
	case "backup":
		file := "linnstrument-backup-" + time.Now().Format("20060102-150405") + ".json"
		if len(args) > 0 {
			file = args[0]
		}
		s, err := openSession(port, 3)
		if err != nil {
			return err
		}
		defer s.Close()
		_, err = backup(s, file)
		return err
	case "restore":
		if len(args) != 1 {
			return fmt.Errorf("restore needs the backup FILE")
		}
		b, err := loadBackup(args[0])
		if err != nil {
			return err
		}
		s, err := openSession(port, 3)
		if err != nil {
			return err
		}
		defer s.Close()
		return restore(s, b)
	case "flash":
		repo, firmware, err := firmwareArg(args)
		if err != nil {
			return err
		}
		return flash(repo, port, firmware)
	case "update":
		repo, firmware, err := firmwareArg(args)
		if err != nil {
			return err
		}
		return update(repo, port, firmware)
	default:
		flag.Usage()
		return fmt.Errorf("unknown command %q", command)
	}
}

func firmwareArg(args []string) (repo, firmware string, err error) {
	repo, err = repoRoot()
	if err != nil {
		return "", "", err
	}
	firmware = filepath.Join(repo, "build", sketchName+".ino.bin")
	if len(args) > 0 {
		firmware = args[0]
	}
	return repo, firmware, nil
}

func listPorts() error {
	ports, err := enumerator.GetDetailedPortsList()
	if err != nil {
		return err
	}
	for _, p := range ports {
		mark := ""
		if p.IsUSB {
			mark = fmt.Sprintf("  usb %s:%s %s", p.VID, p.PID, p.Product)
			if p.VID == linnVID && p.PID == linnPID {
				mark += "  <- LinnStrument"
			}
		}
		fmt.Printf("%s%s\n", p.Name, mark)
	}
	return nil
}

func probe(port string) error {
	s, err := openSession(port, 3)
	if err != nil {
		return err
	}
	defer s.Close()
	settings, err := s.ReadSettings()
	if err != nil {
		return err
	}
	// reading the settings blanks the display, a LED command repaints it
	s.LightLed(0, 0, 0)
	fmt.Printf("LinnStrument on %s answered the handshake\n", s.Name)
	fmt.Printf("settings layout version: %d\n", settings[0])
	fmt.Printf("settings size: %d bytes\n", len(settings))
	return nil
}

func backup(s *Session, file string) (*Backup, error) {
	settings, err := s.ReadSettings()
	if err != nil {
		return nil, err
	}
	b := &Backup{Created: time.Now(), SettingsVersion: int(settings[0]), Settings: settings}
	if b.SettingsVersion < 10 {
		return nil, fmt.Errorf("settings layout %d is older than firmware 2.0, which this tool does not support", b.SettingsVersion)
	}
	fmt.Printf("read settings: layout %d, %d bytes\n", b.SettingsVersion, len(settings))
	if b.Projects, err = s.ReadProjects(); err != nil {
		return nil, err
	}
	s.LightLed(0, 0, 0)
	if len(b.Projects) > 0 {
		fmt.Printf("read %d sequencer projects of %d bytes each\n", len(b.Projects), len(b.Projects[0]))
	}
	data, err := json.MarshalIndent(b, "", "  ")
	if err != nil {
		return nil, err
	}
	if err := os.WriteFile(file, data, 0o644); err != nil {
		return nil, err
	}
	fmt.Printf("saved to %s\n", file)
	return b, nil
}

func loadBackup(file string) (*Backup, error) {
	data, err := os.ReadFile(file)
	if err != nil {
		return nil, err
	}
	var b Backup
	if err := json.Unmarshal(data, &b); err != nil {
		return nil, fmt.Errorf("%s is not a backup file: %w", file, err)
	}
	if len(b.Settings) == 0 || int(b.Settings[0]) != b.SettingsVersion {
		return nil, fmt.Errorf("%s has inconsistent settings data", file)
	}
	return &b, nil
}

// restore sends projects first and settings last, like the Updater, because restoring the
// settings makes the firmware leave serial mode. When the firmware refuses a project (its layout
// changed, ls_serial.ino checks the size) it refuses them all, so the rest are skipped and the
// settings still go back.
func restore(s *Session, b *Backup) error {
	for i, p := range b.Projects {
		fmt.Printf("restoring project %d of %d\n", i+1, len(b.Projects))
		if err := s.RestoreProject(byte(b.SettingsVersion), i, p); err != nil {
			fmt.Printf("project %d refused (%v), the firmware's project layout differs; skipping the projects\n", i+1, err)
			if err := s.handshake(); err != nil {
				return fmt.Errorf("no contact after a refused project: %w", err)
			}
			break
		}
	}
	fmt.Println("restoring settings")
	if err := s.RestoreSettings(b.Settings); err != nil {
		return err
	}
	fmt.Println("restored; the instrument is leaving serial mode and will show up as a MIDI device")
	return nil
}

func update(repo, port, firmware string) error {
	if _, err := os.Stat(firmware); err != nil {
		return fmt.Errorf("no firmware at %s, run the build command first", firmware)
	}
	s, err := openSession(port, 3)
	if err != nil {
		return err
	}
	file := "linnstrument-backup-" + time.Now().Format("20060102-150405") + ".json"
	b, err := backup(s, file)
	s.Close()
	if err != nil {
		return err
	}
	if err := flash(repo, port, firmware); err != nil {
		return fmt.Errorf("flash failed, the settings are safe in %s: %w", file, err)
	}
	// after a flash the firmware boots into serial mode and waits for exactly this
	s, err = openSession(port, 10)
	if err != nil {
		return fmt.Errorf("no contact after flashing, restore by hand with: go run ./tools/linnstrument restore %s (%w)", file, err)
	}
	defer s.Close()
	return restore(s, b)
}
