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
	"errors"
	"fmt"
	"os/exec"
	"strings"
	"time"
)

// serialModeNRPN is the firmware's NRPN that switches the USB port to serial mode (midi.md)
const serialModeNRPN = "271"

func midiDeviceName() (string, error) {
	out, err := exec.Command("sendmidi", "list").Output()
	if err != nil {
		return "", errors.New("sendmidi not found, install it from https://github.com/gbevin/SendMIDI/releases")
	}
	scanner := bufio.NewScanner(strings.NewReader(string(out)))
	for scanner.Scan() {
		name := strings.TrimSpace(scanner.Text())
		if strings.Contains(name, "LinnStrument") {
			return name, nil
		}
	}
	return "", errors.New("no LinnStrument MIDI device found; is it already in serial mode, or not connected?")
}

// enterSerialMode sends NRPN 271 over MIDI and waits for the serial port to appear.
func enterSerialMode(explicitPort string) error {
	name, err := midiDeviceName()
	if err != nil {
		return err
	}
	fmt.Printf("Sending NRPN %s to %q\n", serialModeNRPN, name)
	cmd := exec.Command("sendmidi", "dev", name, "nrpn", serialModeNRPN, "1")
	if out, err := cmd.CombinedOutput(); err != nil {
		return fmt.Errorf("sendmidi failed: %v: %s", err, strings.TrimSpace(string(out)))
	}
	port, err := waitForPort(explicitPort, 10*time.Second)
	if err != nil {
		return err
	}
	fmt.Printf("LinnStrument is in serial mode on %s\n", port)
	return nil
}

// enterMidiMode sends the serial command that leaves serial mode and waits for the port to go.
func enterMidiMode(explicitPort string) error {
	s, err := openSession(explicitPort, sessionAttempts)
	if err != nil {
		return err
	}
	if err := s.write([]byte{'m'}); err != nil {
		s.Close()
		return err
	}
	err = s.expect(ackCode, "switch to MIDI")
	s.Close()
	if err != nil {
		return err
	}
	deadline := time.Now().Add(10 * time.Second)
	for time.Now().Before(deadline) {
		if _, err := findPort(explicitPort); err != nil {
			fmt.Println("LinnStrument is back in MIDI mode")
			return nil
		}
		time.Sleep(200 * time.Millisecond)
	}
	return errors.New("the serial port did not go away; the firmware may predate the m command")
}
