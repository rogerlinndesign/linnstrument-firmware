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
	"errors"
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"runtime"
	"strings"
	"time"

	"go.bug.st/serial"
)

func arduinoDataDir() (string, error) {
	out, err := exec.Command("arduino-cli", "config", "get", "directories.data").Output()
	if err != nil {
		return "", errors.New("arduino-cli not found or not configured, install it from https://arduino.github.io/arduino-cli/")
	}
	return strings.TrimSpace(string(out)), nil
}

func exeSuffix() string {
	if runtime.GOOS == "windows" {
		return ".exe"
	}
	return ""
}

// findBossac prefers LINN_BOSSAC, then the copy in tools/bossac (universal on macOS, where the
// one in the Arduino package is Intel only), then arduino-cli's installed bossac.
func findBossac(repo string) (string, error) {
	if env := os.Getenv("LINN_BOSSAC"); env != "" {
		return env, nil
	}
	bundled := map[string]string{"darwin": filepath.Join("macos", "bossac"), "windows": filepath.Join("windows", "bossac.exe")}
	if rel, ok := bundled[runtime.GOOS]; ok {
		candidate := filepath.Join(repo, "tools", "bossac", rel)
		if _, err := os.Stat(candidate); err == nil {
			return candidate, nil
		}
	}
	data, err := arduinoDataDir()
	if err != nil {
		return "", err
	}
	candidate := filepath.Join(data, "packages", "arduino", "tools", "bossac", "1.6.1-arduino", "bossac"+exeSuffix())
	if _, err := os.Stat(candidate); err == nil {
		return candidate, nil
	}
	return "", fmt.Errorf("bossac not found at %s, install the SAM core (arduino-cli core install arduino:sam@1.6.12) or set LINN_BOSSAC", candidate)
}

// touchBootloader opens the port at 1200 baud and closes it, which makes the USB bridge
// erase the SAM3X and start its bootloader, exactly as the Updater does before running bossac.
func touchBootloader(name string) error {
	port, err := serial.Open(name, &serial.Mode{BaudRate: 1200})
	if err != nil {
		return err
	}
	port.SetDTR(false)
	time.Sleep(100 * time.Millisecond)
	return port.Close()
}

func flash(repo, explicitPort, firmware string) error {
	if _, err := os.Stat(firmware); err != nil {
		return fmt.Errorf("no firmware at %s, run the build command first", firmware)
	}
	if err := sketchFolderName(repo); err != nil {
		return err
	}
	bossac, err := findBossac(repo)
	if err != nil {
		return err
	}
	name, err := waitForPort(explicitPort, 6*time.Second)
	if err != nil {
		return err
	}
	fmt.Printf("Resetting %s into the bootloader\n", name)
	if err := touchBootloader(name); err != nil {
		return err
	}
	time.Sleep(time.Second)
	// the Updater keeps talking to the port it detected before the touch, so do the same;
	// on Windows the port number can change, so fall back to detection there
	if runtime.GOOS == "windows" {
		name, err = waitForPort(explicitPort, 10*time.Second)
	} else {
		name, err = waitForPort(name, 10*time.Second)
	}
	if err != nil {
		return err
	}
	portArg := name
	if runtime.GOOS != "windows" {
		portArg = filepath.Base(name)
	}
	fmt.Printf("Flashing %s with %s\n", firmware, bossac)
	cmd := exec.Command(bossac, "-i", "--port="+portArg, "-U", "false", "-e", "-w", "-v", "-R", "-b", firmware)
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	if err := cmd.Run(); err != nil {
		return fmt.Errorf("bossac failed: %w", err)
	}
	return nil
}
