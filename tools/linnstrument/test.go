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
	"regexp"
	"runtime"
	"sort"
	"strings"
)

// The sketch is merged into one translation unit the way the Arduino build does it (main .ino
// first, the others in name order), with a prototype for every function inserted before the
// first definition, then compiled for the desktop against the stand-ins in tests/support with
// every tests/test_*.cpp included into the same unit.

var functionDefinition = regexp.MustCompile(`^((?:static |inline |volatile |const |unsigned |signed )*[A-Za-z_][A-Za-z0-9_]*(?:<[^>]*>)?(?:\s*[*&])*\s+\**[A-Za-z_][A-Za-z0-9_]*\s*\([^;{}]*\))\s*\{\s*(?://.*)?$`)

var notAFunction = regexp.MustCompile(`^(if|for|while|switch|else|return|case|do|catch)\b`)

// a definition whose opening brace sits on the next line
var functionSignatureOnly = regexp.MustCompile(`^((?:static |inline |volatile |const |unsigned |signed )*[A-Za-z_][A-Za-z0-9_]*(?:<[^>]*>)?(?:\s*[*&])*\s+\**[A-Za-z_][A-Za-z0-9_]*\s*\([^;{}]*\))\s*$`)

func isFunctionDefinition(line string, next string) (string, bool) {
	m := functionDefinition.FindStringSubmatch(line)
	if m == nil && strings.TrimSpace(next) == "{" {
		m = functionSignatureOnly.FindStringSubmatch(line)
	}
	// pointer-to-array parameters are skipped like the Arduino builder does; those functions are
	// only called after their definition
	if m == nil || notAFunction.MatchString(line) || strings.Contains(m[1], "::") || strings.Contains(m[1], "(*") {
		return "", false
	}
	return m[1], true
}

// definitionLines returns the indices of the lines that start a function definition, ignoring
// anything inside block comments (ls_touchInfo.ino quotes a whole C program in one).
func definitionLines(lines []string) []int {
	var found []int
	inComment := false
	for n, line := range lines {
		if inComment {
			if strings.Contains(line, "*/") {
				inComment = false
			}
			continue
		}
		if start := strings.Index(line, "/*"); start >= 0 && !strings.Contains(line[start:], "*/") {
			inComment = true
			continue
		}
		if _, ok := isFunctionDefinition(line, lineAfter(lines, n)); ok {
			found = append(found, n)
		}
	}
	return found
}

func lineAfter(lines []string, n int) string {
	if n+1 < len(lines) {
		return lines[n+1]
	}
	return ""
}

func sketchFiles(repo string) ([]string, error) {
	others, err := filepath.Glob(filepath.Join(repo, "ls_*.ino"))
	if err != nil {
		return nil, err
	}
	sort.Strings(others)
	return append([]string{filepath.Join(repo, sketchName+".ino")}, others...), nil
}

func mergeSketch(repo string) (string, error) {
	files, err := sketchFiles(repo)
	if err != nil {
		return "", err
	}
	var contents []string
	var prototypes []string
	for _, f := range files {
		data, err := os.ReadFile(f)
		if err != nil {
			return "", err
		}
		text := string(data)
		contents = append(contents, text)
		lines := strings.Split(text, "\n")
		for _, n := range definitionLines(lines) {
			sig, _ := isFunctionDefinition(lines[n], lineAfter(lines, n))
			prototypes = append(prototypes, sig+";")
		}
	}
	var sb strings.Builder
	sb.WriteString("#include <Arduino.h>\n#include <host_mock.h>\n")
	for i, f := range files {
		lines := strings.Split(contents[i], "\n")
		fmt.Fprintf(&sb, "#line 1 %q\n", f)
		insertAt := -1
		if i == 0 {
			if defs := definitionLines(lines); len(defs) > 0 {
				insertAt = defs[0]
			}
		}
		for n, line := range lines {
			if n == insertAt {
				sb.WriteString(strings.Join(prototypes, "\n"))
				fmt.Fprintf(&sb, "\n#line %d %q\n", n+1, f)
			}
			sb.WriteString(line)
			sb.WriteString("\n")
		}
	}
	return sb.String(), nil
}

func findCompiler() (string, error) {
	if env := os.Getenv("CXX"); env != "" {
		return env, nil
	}
	for _, c := range []string{"c++", "clang++", "g++"} {
		if p, err := exec.LookPath(c); err == nil {
			return p, nil
		}
	}
	hint := "install a C++ compiler"
	switch runtime.GOOS {
	case "darwin":
		hint = "install the Xcode Command Line Tools (xcode-select --install)"
	case "windows":
		hint = "install MinGW-w64 or MSYS2 and put g++ on the PATH, or set CXX"
	}
	return "", errors.New("no C++ compiler found, " + hint)
}

func runTests(repo string) error {
	cxx, err := findCompiler()
	if err != nil {
		return err
	}
	testsDir := filepath.Join(repo, "tests")
	testFiles, err := filepath.Glob(filepath.Join(testsDir, "test_*.cpp"))
	if err != nil {
		return err
	}
	sort.Strings(testFiles)
	if len(testFiles) == 0 {
		return errors.New("no tests/test_*.cpp files")
	}

	merged, err := mergeSketch(repo)
	if err != nil {
		return err
	}
	var sb strings.Builder
	sb.WriteString(merged)
	fmt.Fprintf(&sb, "#include %q\n", filepath.Join(testsDir, "framework.h"))
	fmt.Fprintf(&sb, "#include %q\n", filepath.Join(testsDir, "mocks.cpp"))
	for _, t := range testFiles {
		fmt.Fprintf(&sb, "#include %q\n", t)
	}
	fmt.Fprintf(&sb, "#include %q\n", filepath.Join(testsDir, "runner.cpp"))

	buildDir := filepath.Join(repo, "build", "tests")
	if err := os.MkdirAll(buildDir, 0o755); err != nil {
		return err
	}
	unit := filepath.Join(buildDir, "all.cpp")
	if err := os.WriteFile(unit, []byte(sb.String()), 0o644); err != nil {
		return err
	}
	binary := filepath.Join(buildDir, "run_tests"+exeSuffix())
	support := filepath.Join(testsDir, "support")
	args := []string{"-std=gnu++11", "-g", "-O1", "-w", "-I", support, "-I", repo, "-o", binary, unit, filepath.Join(support, "arduino_mock.cpp")}
	fmt.Printf("Compiling %d test files with the sketch (%s)\n", len(testFiles), cxx)
	if err := run(cxx, args...); err != nil {
		return fmt.Errorf("test build failed: %w", err)
	}
	return run(binary)
}
