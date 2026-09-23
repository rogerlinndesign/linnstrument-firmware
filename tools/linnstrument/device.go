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
	"bytes"
	"encoding/binary"
	"errors"
	"fmt"
	"hash/crc32"
	"os"
	"runtime"
	"strings"
	"time"

	"go.bug.st/serial"
	"go.bug.st/serial/enumerator"
)

// USB identity of a LinnStrument in serial mode, as the Updater checks it
const (
	linnVID = "F055"
	linnPID = "0070"
)

const (
	batchSize       = 96
	batchPause      = 20 * time.Millisecond
	settleAfterOpen = 2 * time.Second
	ioTimeout       = 3 * time.Second
)

var (
	countdownCode = []byte("5, 4, 3, 2, 1 ...\n")
	goCode        = []byte("LinnStruments are go!\n")
	ackCode       = []byte("ACK\n")
)

func findPort(explicit string) (string, error) {
	if explicit != "" {
		return explicit, nil
	}
	ports, err := enumerator.GetDetailedPortsList()
	if err != nil {
		return "", err
	}
	var matches []string
	for _, p := range ports {
		if p.IsUSB && strings.EqualFold(p.VID, linnVID) && strings.EqualFold(p.PID, linnPID) {
			name := p.Name
			if runtime.GOOS == "darwin" {
				name = strings.Replace(name, "/dev/tty.", "/dev/cu.", 1)
			}
			matches = append(matches, name)
		}
	}
	switch len(matches) {
	case 0:
		return "", errors.New("no LinnStrument in serial mode found (USB F055:0070), turn on OS Update in Global Settings")
	case 1:
		return matches[0], nil
	default:
		return "", fmt.Errorf("several LinnStruments found (%s), choose one with -port", strings.Join(matches, ", "))
	}
}

func waitForPort(explicit string, timeout time.Duration) (string, error) {
	deadline := time.Now().Add(timeout)
	var last error
	for {
		name, err := findPort(explicit)
		if err == nil {
			if runtime.GOOS == "windows" {
				return name, nil
			}
			if _, statErr := os.Stat(name); statErr == nil {
				return name, nil
			}
			err = fmt.Errorf("%s is not present", name)
		}
		last = err
		if time.Now().After(deadline) {
			return "", fmt.Errorf("the LinnStrument's serial port did not appear within %s: %w", timeout, last)
		}
		time.Sleep(100 * time.Millisecond)
	}
}

// Session is one open conversation with the firmware's serial protocol (ls_serial.ino).
// Opening or closing the port toggles DTR, which resets the instrument about one time in
// three up to 1.5 s later, so a session waits that window out after opening, retries the
// handshake on a fresh open, and callers keep all their commands inside one session.
type Session struct {
	port     serial.Port
	Name     string
	explicit string
}

func openSession(explicit string, attempts int) (*Session, error) {
	var last error
	for i := 0; i < attempts; i++ {
		if i > 0 {
			time.Sleep(time.Second)
		}
		name, err := waitForPort(explicit, 6*time.Second)
		if err != nil {
			last = err
			continue
		}
		port, err := serial.Open(name, &serial.Mode{BaudRate: 115200})
		if err != nil {
			last = err
			continue
		}
		port.SetReadTimeout(ioTimeout)
		time.Sleep(settleAfterOpen)
		port.ResetInputBuffer()
		s := &Session{port: port, Name: name, explicit: explicit}
		if err := s.handshake(); err != nil {
			port.Close()
			last = err
			continue
		}
		return s, nil
	}
	return nil, fmt.Errorf("no handshake after %d attempts: %w", attempts, last)
}

func (s *Session) Close() {
	s.port.Close()
}

func (s *Session) write(b []byte) error {
	_, err := s.port.Write(b)
	return err
}

func (s *Session) readExact(n int) ([]byte, error) {
	buf := make([]byte, n)
	got := 0
	deadline := time.Now().Add(ioTimeout)
	for got < n {
		if time.Now().After(deadline) {
			return buf[:got], fmt.Errorf("timeout after %d of %d bytes", got, n)
		}
		k, err := s.port.Read(buf[got:])
		if err != nil {
			return buf[:got], err
		}
		got += k
	}
	return buf, nil
}

func (s *Session) expect(code []byte, what string) error {
	got, err := s.readExact(len(code))
	if err != nil {
		return fmt.Errorf("%s: %w", what, err)
	}
	if !bytes.Equal(got, code) {
		return fmt.Errorf("%s: expected %q, got %q", what, code, got)
	}
	return nil
}

func (s *Session) readInt32() (int32, error) {
	b, err := s.readExact(4)
	if err != nil {
		return 0, err
	}
	return int32(binary.LittleEndian.Uint32(b)), nil
}

func (s *Session) writeInt32(v int32) error {
	b := make([]byte, 4)
	binary.LittleEndian.PutUint32(b, uint32(v))
	return s.write(b)
}

// A previous session can leave the firmware waiting for a command, in which state the first
// byte of the countdown would be eaten as an unknown command. A newline first puts the
// firmware's state machine back at the start either way (ls_serial.ino handleSerialIO).
func (s *Session) handshake() error {
	if err := s.write([]byte{'\n'}); err != nil {
		return err
	}
	time.Sleep(50 * time.Millisecond)
	s.port.ResetInputBuffer()
	if err := s.write(countdownCode); err != nil {
		return err
	}
	return s.expect(goCode, "handshake")
}

func (s *Session) receiveBatches(size int) ([]byte, error) {
	data := make([]byte, 0, size)
	for len(data) < size {
		time.Sleep(batchPause)
		n := min(batchSize, size-len(data))
		chunk, err := s.readExact(n)
		if err != nil {
			return nil, fmt.Errorf("batch at %d: %w", len(data), err)
		}
		if err := s.write([]byte{'c'}); err != nil {
			return nil, err
		}
		remote, err := s.readExact(4)
		if err != nil {
			return nil, fmt.Errorf("crc at %d: %w", len(data), err)
		}
		if binary.LittleEndian.Uint32(remote) == crc32.ChecksumIEEE(chunk) {
			if err := s.write([]byte{'o'}); err != nil {
				return nil, err
			}
			data = append(data, chunk...)
		} else if err := s.write([]byte{'w'}); err != nil {
			return nil, err
		}
	}
	return data, nil
}

func (s *Session) sendBatches(data []byte) error {
	sent := 0
	for sent < len(data) {
		time.Sleep(batchPause)
		chunk := data[sent:min(sent+batchSize, len(data))]
		if err := s.write(chunk); err != nil {
			return err
		}
		if err := s.expect([]byte{'c'}, fmt.Sprintf("crc check at %d", sent)); err != nil {
			return err
		}
		crc := make([]byte, 4)
		binary.LittleEndian.PutUint32(crc, crc32.ChecksumIEEE(chunk))
		if err := s.write(crc); err != nil {
			return err
		}
		state, err := s.readExact(1)
		if err != nil {
			return fmt.Errorf("crc state at %d: %w", sent, err)
		}
		switch state[0] {
		case 'o':
			sent += len(chunk)
		case 'w':
		default:
			return fmt.Errorf("crc state at %d: unexpected %q", sent, state)
		}
	}
	return nil
}

func (s *Session) ReadSettings() ([]byte, error) {
	if err := s.write([]byte{'s'}); err != nil {
		return nil, err
	}
	if err := s.expect(ackCode, "settings request"); err != nil {
		return nil, err
	}
	size, err := s.readInt32()
	if err != nil {
		return nil, err
	}
	data, err := s.receiveBatches(int(size))
	if err != nil {
		return nil, err
	}
	return data, s.expect(ackCode, "settings end")
}

// Needs settings layout 10 or later, older firmware had no CRC on project batches.
func (s *Session) ReadProjects() (projects [][]byte, err error) {
	if err := s.write([]byte{'p'}); err != nil {
		return nil, err
	}
	if err := s.expect(ackCode, "projects request"); err != nil {
		return nil, err
	}
	count, err := s.readExact(1)
	if err != nil {
		return nil, err
	}
	size, err := s.readInt32()
	if err != nil {
		return nil, err
	}
	for p := 0; p < int(count[0]); p++ {
		data, err := s.receiveBatches(int(size))
		if err != nil {
			return nil, fmt.Errorf("project %d: %w", p, err)
		}
		projects = append(projects, data)
	}
	return projects, s.expect(ackCode, "projects end")
}

// RestoreSettings sends a settings blob back. The firmware migrates older layouts itself,
// then leaves serial mode: after this the unit is a MIDI device again.
func (s *Session) RestoreSettings(data []byte) error {
	if err := s.write([]byte{'r'}); err != nil {
		return err
	}
	if err := s.expect(ackCode, "restore request"); err != nil {
		return err
	}
	if err := s.writeInt32(int32(len(data))); err != nil {
		return err
	}
	if err := s.expect(ackCode, "restore size"); err != nil {
		return err
	}
	if err := s.sendBatches(data); err != nil {
		return err
	}
	return s.expect(ackCode, "restore end")
}

func (s *Session) RestoreProject(version byte, index int, data []byte) error {
	if err := s.write([]byte{'q'}); err != nil {
		return err
	}
	if err := s.expect(ackCode, "project request"); err != nil {
		return err
	}
	if err := s.write([]byte{version}); err != nil {
		return err
	}
	if err := s.expect(ackCode, "project version"); err != nil {
		return err
	}
	if err := s.writeInt32(int32(len(data))); err != nil {
		return err
	}
	if err := s.expect(ackCode, "project size"); err != nil {
		return err
	}
	if err := s.write([]byte{byte(index)}); err != nil {
		return err
	}
	if err := s.expect(ackCode, "project index"); err != nil {
		return err
	}
	if err := s.sendBatches(data); err != nil {
		return err
	}
	return s.expect(ackCode, "project end")
}

func (s *Session) LightLed(col, row, color byte) error {
	return s.write([]byte{'l', col, row, color})
}
