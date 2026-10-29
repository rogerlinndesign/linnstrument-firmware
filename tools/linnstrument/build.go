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
	"os"
	"os/exec"
	"path/filepath"
	"regexp"
	"strconv"
	"strings"
)

const (
	fqbn       = "arduino:sam:arduino_due_x"
	samCore    = "arduino:sam@1.6.12"
	sketchName = "linnstrument-firmware"
	dueSRAM    = 98304
)

func repoRoot() (string, error) {
	dir, err := os.Getwd()
	if err != nil {
		return "", err
	}
	for {
		if _, err := os.Stat(filepath.Join(dir, sketchName+".ino")); err == nil {
			return dir, nil
		}
		parent := filepath.Dir(dir)
		if parent == dir {
			return "", errors.New("run this inside the linnstrument-firmware repository")
		}
		dir = parent
	}
}

func run(name string, args ...string) error {
	cmd := exec.Command(name, args...)
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	return cmd.Run()
}

func ensureSamCore() error {
	out, err := exec.Command("arduino-cli", "core", "list").Output()
	if err != nil {
		return errors.New("arduino-cli not found, install it from https://arduino.github.io/arduino-cli/")
	}
	if strings.Contains(string(out), "arduino:sam ") {
		if !strings.Contains(string(out), "1.6.12") {
			return fmt.Errorf("the installed arduino:sam core is not 1.6.12, run: arduino-cli core install %s", samCore)
		}
		return nil
	}
	fmt.Println("Installing the Arduino SAM core (one time)")
	if err := run("arduino-cli", "core", "update-index"); err != nil {
		return err
	}
	return run("arduino-cli", "core", "install", samCore)
}

// arduino-cli refuses a sketch whose folder is not named after it
func sketchFolderName(repo string) error {
	if filepath.Base(repo) != sketchName {
		return fmt.Errorf("the sketch folder must be named %s for arduino-cli, it is %s", sketchName, repo)
	}
	return nil
}

func build(repo string) error {
	if err := sketchFolderName(repo); err != nil {
		return err
	}
	if err := ensureSamCore(); err != nil {
		return err
	}
	err := run("arduino-cli", "compile", "--fqbn", fqbn,
		"--libraries", filepath.Join(repo, "libraries"),
		"--output-dir", filepath.Join(repo, "build"), repo)
	if err != nil {
		return fmt.Errorf("compile failed: %w", err)
	}
	reportRAM(repo)
	return nameFirmware(repo)
}

// nameFirmware copies the build to the name CI gives it, which the Updater shows before it
// flashes, so that a person can tell which build they are about to install
func nameFirmware(repo string) error {
	name, err := firmwareName(repo)
	if err != nil {
		return err
	}
	dir := filepath.Join(repo, "build")
	old, _ := filepath.Glob(filepath.Join(dir, sketchName+"-*.bin"))
	for _, path := range old {
		os.Remove(path)
	}
	data, err := os.ReadFile(filepath.Join(dir, sketchName+".ino.bin"))
	if err != nil {
		return err
	}
	named := filepath.Join(dir, sketchName+"-"+name+".bin")
	if err := os.WriteFile(named, data, 0o644); err != nil {
		return err
	}
	fmt.Printf("For the LinnStrument Updater: %s\n", named)
	return nil
}

// firmwareName is what tools/build-name.sh prints, such as 240.075-1a2b3c4, with -modified
// when the sketch has changes that are not committed
func firmwareName(repo string) (string, error) {
	sketch, err := os.ReadFile(filepath.Join(repo, sketchName+".ino"))
	if err != nil {
		return "", err
	}
	version, err := firmwareVersion(string(sketch))
	if err != nil {
		return "", err
	}
	commit, err := exec.Command("git", "-C", repo, "rev-parse", "--short=7", "HEAD").Output()
	if err != nil {
		return version + "-unknown", nil
	}
	name := version + "-" + strings.TrimSpace(string(commit))
	changes, _ := exec.Command("git", "-C", repo, "status", "--porcelain", "--untracked-files=no", "--", ":(glob)*.ino", ":(glob)*.h").Output()
	if len(strings.TrimSpace(string(changes))) > 0 {
		name += "-modified"
	}
	return name, nil
}

var osVersionPattern = regexp.MustCompile(`(?m)^const char\* OSVersion = "([0-9.]*)";`)
var osVersionBuildPattern = regexp.MustCompile(`(?m)^const char\* OSVersionBuild = "(\.[0-9]*)";`)

// firmwareVersion is the version as the instrument's screen shows it; OSVersion ends with the
// dot OSVersionBuild starts with
func firmwareVersion(sketch string) (string, error) {
	version := osVersionPattern.FindStringSubmatch(sketch)
	build := osVersionBuildPattern.FindStringSubmatch(sketch)
	if version == nil || build == nil {
		return "", errors.New("no OSVersion and OSVersionBuild in the sketch")
	}
	return strings.TrimSuffix(version[1], ".") + build[1], nil
}

// reportRAM prints the static RAM use, which the SAM core itself does not report.
func reportRAM(repo string) {
	used, err := staticRAM(repo)
	if err != nil {
		fmt.Printf("RAM use not reported: %v\n", err)
		return
	}
	fmt.Printf("Global variables use %d bytes (%d%%) of dynamic memory. Maximum is %d bytes.\n", used, used*100/dueSRAM, dueSRAM)
}

func staticRAM(repo string) (int, error) {
	data, err := arduinoDataDir()
	if err != nil {
		return 0, err
	}
	sizeTool := filepath.Join(data, "packages", "arduino", "tools", "arm-none-eabi-gcc", "4.8.3-2014q1", "bin", "arm-none-eabi-size"+exeSuffix())
	out, err := exec.Command(sizeTool, filepath.Join(repo, "build", sketchName+".ino.elf")).Output()
	if err != nil {
		return 0, fmt.Errorf("%s: %w", sizeTool, err)
	}
	scanner := bufio.NewScanner(strings.NewReader(string(out)))
	scanner.Scan()
	if !scanner.Scan() {
		return 0, errors.New("unexpected arm-none-eabi-size output")
	}
	fields := strings.Fields(scanner.Text())
	if len(fields) < 3 {
		return 0, errors.New("unexpected arm-none-eabi-size output")
	}
	dataBytes, err1 := strconv.Atoi(fields[1])
	bssBytes, err2 := strconv.Atoi(fields[2])
	if err1 != nil || err2 != nil {
		return 0, errors.New("unexpected arm-none-eabi-size output")
	}
	return dataBytes + bssBytes, nil
}
