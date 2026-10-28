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
	"context"
	"errors"
	"fmt"
	"os"
	"os/exec"
	"os/signal"
	"path/filepath"
	"regexp"
	"sort"
	"strings"
)

// A mutation names a test, a firmware file and a change to it. The test must pass on the real
// code and fail on the mutated code; a mutation the test does not notice is reported, and so is
// a test without a mutation. This is how the suite proves its tests can fail, see
// tests/mutations.tsv.
type mutation struct {
	test string
	file string
	old  string
	new  string
	line int
}

func readMutations(path string) ([]mutation, error) {
	f, err := os.Open(path)
	if err != nil {
		return nil, err
	}
	defer f.Close()
	var list []mutation
	scanner := bufio.NewScanner(f)
	n := 0
	for scanner.Scan() {
		n++
		line := strings.TrimRight(scanner.Text(), "\r")
		if strings.TrimSpace(line) == "" || strings.HasPrefix(line, "#") {
			continue
		}
		parts := strings.Split(line, "\t")
		if len(parts) != 4 {
			return nil, fmt.Errorf("%s:%d: expected 4 tab separated fields, got %d", path, n, len(parts))
		}
		// \n in a field stands for a line break, so a mutation can span lines
		old := strings.ReplaceAll(parts[2], `\n`, "\n")
		new := strings.ReplaceAll(parts[3], `\n`, "\n")
		list = append(list, mutation{test: parts[0], file: parts[1], old: old, new: new, line: n})
	}
	return list, scanner.Err()
}

var testName = regexp.MustCompile(`(?m)^TEST\(([A-Za-z0-9_]+)\)`)

func testNames(repo string) ([]string, error) {
	files, err := filepath.Glob(filepath.Join(repo, "tests", "test_*.cpp"))
	if err != nil {
		return nil, err
	}
	var names []string
	for _, f := range files {
		data, err := os.ReadFile(f)
		if err != nil {
			return nil, err
		}
		for _, m := range testName.FindAllSubmatch(data, -1) {
			names = append(names, string(m[1]))
		}
	}
	sort.Strings(names)
	return names, nil
}

type outcome int

const (
	outcomeFailed outcome = iota
	outcomePassed
	outcomeNoBuild
	outcomeNoSuchTest
)

// the tests a filter selects, an error when it selects none
func selectTests(names []string, filter string) ([]string, error) {
	var selected []string
	for _, name := range names {
		if strings.Contains(name, filter) {
			selected = append(selected, name)
		}
	}
	if len(selected) == 0 {
		return nil, fmt.Errorf("no test name contains %q (the match is case-sensitive)", filter)
	}
	return selected, nil
}

// the tests without a line in mutations.tsv
func uncovered(names []string, list []mutation) []string {
	covered := map[string]bool{}
	for _, m := range list {
		covered[m.test] = true
	}
	var missing []string
	for _, name := range names {
		if !covered[name] {
			missing = append(missing, name)
		}
	}
	return missing
}

func testOutcome(ctx context.Context, repo, filter string) outcome {
	cmd := exec.CommandContext(ctx, "go", "run", "./tools/linnstrument", "test", filter)
	cmd.Dir = repo
	out, err := cmd.CombinedOutput()
	switch {
	case !bytes.Contains(out, []byte(" tests, ")):
		return outcomeNoBuild
	case bytes.Contains(out, []byte("\n0 tests, ")):
		return outcomeNoSuchTest
	case err == nil:
		return outcomePassed
	default:
		return outcomeFailed
	}
}

func runMutations(repo string, only string) error {
	list, err := readMutations(filepath.Join(repo, "tests", "mutations.tsv"))
	if err != nil {
		return err
	}
	names, err := testNames(repo)
	if err != nil {
		return err
	}
	selected, err := selectTests(names, only)
	if err != nil {
		return err
	}
	problems := 0
	for _, name := range uncovered(selected, list) {
		fmt.Printf("NO MUTATION  %s\n", name)
		problems++
	}

	ctx, stop := signal.NotifyContext(context.Background(), os.Interrupt)
	defer stop()

	fmt.Println("Checking that the suite passes on the real firmware")
	if o := testOutcome(ctx, repo, only); o != outcomePassed {
		return errors.New("the tests do not pass before any mutation; fix that first")
	}

	ran := 0
	for _, m := range list {
		if only != "" && !strings.Contains(m.test, only) {
			continue
		}
		if ctx.Err() != nil {
			break
		}
		ran++
		path := filepath.Join(repo, m.file)
		original, err := os.ReadFile(path)
		if err != nil {
			return err
		}
		if bytes.Count(original, []byte(m.old)) != 1 {
			fmt.Printf("NOT APPLICABLE %s: %s does not contain the text exactly once (mutations.tsv line %d)\n", m.test, m.file, m.line)
			problems++
			continue
		}
		if err := os.WriteFile(path, bytes.Replace(original, []byte(m.old), []byte(m.new), 1), 0o644); err != nil {
			return err
		}
		result := func() outcome {
			// the original goes back whatever happens, including an interrupt
			defer os.WriteFile(path, original, 0o644)
			return testOutcome(ctx, repo, m.test)
		}()
		switch result {
		case outcomeNoBuild:
			fmt.Printf("NO BUILD     %s: the mutation of %s does not compile\n", m.test, m.file)
			problems++
		case outcomeNoSuchTest:
			fmt.Printf("NO SUCH TEST %s (mutations.tsv line %d)\n", m.test, m.line)
			problems++
		case outcomePassed:
			fmt.Printf("SURVIVED     %s does not notice %q -> %q in %s\n", m.test, m.old, m.new, m.file)
			problems++
		default:
			fmt.Printf("caught       %s\n", m.test)
		}
	}
	if ctx.Err() != nil {
		return errors.New("interrupted, the firmware files were restored")
	}
	fmt.Printf("\n%d mutations, %d problems\n", ran, problems)
	if problems > 0 {
		return errors.New("some tests cannot fail, or have no mutation")
	}
	return nil
}
