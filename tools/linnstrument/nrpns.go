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

// Turns the NRPN table in midi.md into examples/nrpn_parameters.js, the parameter description
// the settings editor builds its form from, so the page and the documentation cannot drift.

import (
	"encoding/json"
	"fmt"
	"os"
	"path/filepath"
	"regexp"
	"sort"
	"strconv"
	"strings"
)

type nrpnOption struct {
	Value int    `json:"value"`
	Label string `json:"label"`
}

type nrpnParameter struct {
	Number      int          `json:"number"`
	Group       string       `json:"group"`
	Name        string       `json:"name"`
	Kind        string       `json:"kind"`
	Min         int          `json:"min"`
	Max         int          `json:"max"`
	Options     []nrpnOption `json:"options,omitempty"`
	Note        string       `json:"note,omitempty"`
	Description string       `json:"description"`
}

var (
	nrpnRow      = regexp.MustCompile(`^\| (\d+)\s*\| ([^|]*?)\s*\| (.*?)\s*$`)
	valueRow     = regexp.MustCompile(`^\| (\d+)\s*\| (.*?)\s*$`)
	optionEntry  = regexp.MustCompile(`^(\d+(?: \d+)*)(?:-(\d+))?: (.+)$`)
	rangeLabel   = regexp.MustCompile(`^([+-]?\d+) to ([+-]?\d+)$`)
	tableRef     = regexp.MustCompile(`,? ?\(?see (?:the )?(color value table|touch animation values|switch assignment values) below\)?`)
	nrpnGroups   = []string{"Split Left", "Split Right", "Global", "Device"}
	tableHeading = map[string]string{"Color Values": "color", "Touch Animation Values": "touchAnimation", "Switch Assignment Values": "switchAssignment"}
)

const licenseText = `Copyright 2026 Roger Linn Design (https://www.rogerlinndesign.com)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
`

func writeNrpnTable(repo, out string) error {
	text, err := os.ReadFile(filepath.Join(repo, "midi.md"))
	if err != nil {
		return err
	}
	lines := strings.Split(string(text), "\n")
	tables := map[string][]nrpnOption{}
	for i, line := range lines {
		key, ok := tableHeading[strings.TrimSpace(line)]
		if !ok {
			continue
		}
		for _, row := range lines[i+1:] {
			if m := valueRow.FindStringSubmatch(row); m != nil {
				value, _ := strconv.Atoi(m[1])
				tables[key] = append(tables[key], nrpnOption{value, m[2]})
			} else if len(tables[key]) > 0 {
				break
			}
		}
	}
	var parameters []nrpnParameter
	inTable := false
	for _, line := range lines {
		if strings.Contains(line, "lists all the NRPN input values") {
			inTable = true
			continue
		}
		if !inTable {
			continue
		}
		m := nrpnRow.FindStringSubmatch(line)
		if m == nil {
			if len(parameters) > 0 && strings.TrimSpace(line) == "" {
				break
			}
			continue
		}
		number, _ := strconv.Atoi(m[1])
		if number == 299 {
			continue
		}
		p, err := parseNrpnRow(number, m[2], m[3], tables)
		if err != nil {
			return fmt.Errorf("midi.md NRPN %d: %w", number, err)
		}
		parameters = append(parameters, p)
	}
	if len(parameters) < 200 || len(tables) != 3 {
		return fmt.Errorf("midi.md: found %d parameters and %d value tables, the table format must have changed", len(parameters), len(tables))
	}
	data, err := json.MarshalIndent(map[string]interface{}{"parameters": parameters, "tables": tables}, "  ", "  ")
	if err != nil {
		return err
	}
	js := "/*\n" + licenseText + "*/\n" +
		"// Generated from the NRPN table in midi.md by `go run ./tools/linnstrument nrpns`, do not edit\n" +
		"// by hand. Each parameter: number, group, name, kind (toggle, enum, number, action, choice-action,\n" +
		"// readonly), min, max, the options of an enum, a note for the person, and the original description.\n" +
		"(function (root, factory) {\n" +
		"  if (typeof module === \"object\" && module.exports) {\n    module.exports = factory();\n  } else {\n    root.LinnNrpns = factory();\n  }\n" +
		"})(typeof self !== \"undefined\" ? self : this, function () {\n  \"use strict\";\n  return " + string(data) + ";\n});\n"
	if err := os.MkdirAll(filepath.Dir(out), 0o755); err != nil {
		return err
	}
	if err := os.WriteFile(out, []byte(js), 0o644); err != nil {
		return err
	}
	fmt.Printf("%d parameters and %d value tables written to %s\n", len(parameters), len(tables), out)
	return checkNrpnTable(repo, parameters)
}

func parseNrpnRow(number int, valueField, description string, tables map[string][]nrpnOption) (nrpnParameter, error) {
	p := nrpnParameter{Number: number, Description: description}
	for _, g := range nrpnGroups {
		if strings.HasPrefix(description, g+" ") {
			p.Group = g
			break
		}
	}
	if p.Group == "" {
		return p, fmt.Errorf("no group prefix in %q", description)
	}
	name := strings.TrimPrefix(description, p.Group+" ")
	if m := tableRef.FindStringSubmatch(name); m != nil {
		name = strings.TrimSpace(tableRef.ReplaceAllString(name, ""))
		p.Options = tables[map[string]string{"color value table": "color", "touch animation values": "touchAnimation", "switch assignment values": "switchAssignment"}[m[1]]]
	}
	paren := ""
	if at := strings.Index(name, "("); at >= 0 && strings.HasSuffix(name, ")") {
		paren = name[at+1 : len(name)-1]
		name = strings.TrimSpace(name[:at])
	} else if at >= 0 && strings.TrimSpace(valueField) == "-" {
		if end := strings.Index(name, ")"); end > at {
			if options, ok := parseOptions(name[at+1 : end]); ok {
				p.Options = options
			}
		}
		p.Note = strings.TrimSpace(name[at:])
		name = strings.TrimSpace(name[:at])
	}
	p.Name = name

	valueField = strings.TrimSpace(valueField)
	switch {
	case valueField == "-":
		p.Kind = "readonly"
	case valueField == "1":
		p.Kind = "action"
		p.Min, p.Max = 1, 1
	default:
		parts := strings.SplitN(valueField, "-", 2)
		if len(parts) != 2 {
			return p, fmt.Errorf("value range %q", valueField)
		}
		p.Min, _ = strconv.Atoi(parts[0])
		p.Max, _ = strconv.Atoi(parts[1])
	}

	if paren != "" && p.Options == nil {
		options, ok := parseOptions(paren)
		if ok {
			p.Options = options
		} else {
			p.Note = paren
		}
	} else if paren != "" {
		p.Note = paren
	}
	if p.Options != nil && p.Kind == "" {
		filtered := p.Options[:0:0]
		for _, o := range p.Options {
			if o.Value >= p.Min && o.Value <= p.Max || o.Value > p.Max && !strings.Contains(description, "see") {
				filtered = append(filtered, o)
			}
		}
		p.Options = filtered
		for _, o := range p.Options {
			if o.Value > p.Max {
				p.Max = o.Value
			}
		}
		if len(p.Options) > p.Max-p.Min+1 || len(p.Options) == 0 {
			return p, fmt.Errorf("options %v do not fit %d-%d", p.Options, p.Min, p.Max)
		}
	}
	switch {
	case p.Kind != "":
	case strings.Contains(p.Name, "Preset Load"):
		p.Kind = "choice-action"
		for v := p.Min; v <= p.Max; v++ {
			p.Options = append(p.Options, nrpnOption{v, "Preset " + strconv.Itoa(v+1)})
		}
	case len(p.Options) == 2 && p.Min == 0 && p.Max == 1:
		p.Kind = "toggle"
	case len(p.Options) > 0:
		p.Kind = "enum"
	default:
		p.Kind = "number"
	}
	if number == 271 {
		p.Note = "Takes the instrument off MIDI until OS Update is tapped on it or the updater returns it; " + p.Note
	}
	return p, nil
}

// "0: Off, 1: On", "3 4 5 6 7 12: Intervals", "0-6: -7 to -1", with a leading remark allowed
func parseOptions(paren string) ([]nrpnOption, bool) {
	for _, prefix := range []string{"unprinted parameter, ", "only supports, "} {
		paren = strings.TrimPrefix(paren, prefix)
	}
	var options []nrpnOption
	for _, entry := range strings.Split(paren, ", ") {
		m := optionEntry.FindStringSubmatch(strings.TrimSpace(entry))
		if m == nil {
			return nil, false
		}
		label := strings.TrimSpace(m[3])
		if m[2] != "" {
			from, _ := strconv.Atoi(m[1])
			to, _ := strconv.Atoi(m[2])
			if to-from > 16 {
				return nil, false
			}
			r := rangeLabel.FindStringSubmatch(label)
			for v := from; v <= to; v++ {
				text := label
				if r != nil {
					lo, _ := strconv.Atoi(r[1])
					n := lo + (v - from)
					text = strconv.Itoa(n)
					if n > 0 {
						text = "+" + text
					}
				}
				options = append(options, nrpnOption{v, text})
			}
			continue
		}
		values := strings.Fields(m[1])
		for _, field := range values {
			v, _ := strconv.Atoi(field)
			text := label
			if len(values) > 1 {
				text = label + " " + field
			}
			options = append(options, nrpnOption{v, text})
		}
	}
	return options, len(options) > 0
}

// The firmware's own NRPN handling, read from ls_midi.ino: which numbers receivedNrpn() takes
// and the range it checks them against, and which numbers sendNrpnParameter() answers.
type firmwareNrpn struct {
	min, max int
	ranged   bool
}

var (
	caseLine    = regexp.MustCompile(`case (\d+):`)
	inRangeCall = regexp.MustCompile(`inRange\(value, (\d+), (\d+)\)`)
)

func functionBody(source, name string) string {
	start := strings.Index(source, "void "+name+"(")
	if start < 0 {
		return ""
	}
	depth, body := 0, start
	for i := start; i < len(source); i++ {
		switch source[i] {
		case '{':
			depth++
		case '}':
			depth--
			if depth == 0 {
				return source[body:i]
			}
		}
	}
	return ""
}

// the cases of the function's outermost switch, each with the first inRange() check of its body
func switchCases(body string) map[int]firmwareNrpn {
	cases := map[int]firmwareNrpn{}
	depth, switchDepth := 0, -1
	var pending []int
	for _, line := range strings.Split(body, "\n") {
		trimmed := strings.TrimSpace(line)
		if switchDepth < 0 && strings.HasPrefix(trimmed, "switch (") {
			switchDepth = depth + 1
		}
		if switchDepth >= 0 && depth == switchDepth {
			for _, m := range caseLine.FindAllStringSubmatch(trimmed, -1) {
				n, _ := strconv.Atoi(m[1])
				cases[n] = firmwareNrpn{}
				pending = append(pending, n)
			}
		}
		if trimmed == "break;" && depth == switchDepth {
			pending = nil
		}
		if m := inRangeCall.FindStringSubmatch(trimmed); m != nil && len(pending) > 0 && depth >= switchDepth {
			lo, _ := strconv.Atoi(m[1])
			hi, _ := strconv.Atoi(m[2])
			for _, n := range pending {
				cases[n] = firmwareNrpn{lo, hi, true}
			}
			pending = nil
		}
		depth += strings.Count(line, "{") - strings.Count(line, "}")
		if switchDepth >= 0 && depth < switchDepth {
			break
		}
	}
	return cases
}

// Every number in midi.md must be one the firmware handles, with the same range, and the other
// way round, so the table cannot drift from ls_midi.ino.
func checkNrpnTable(repo string, parameters []nrpnParameter) error {
	source, err := os.ReadFile(filepath.Join(repo, "ls_midi.ino"))
	if err != nil {
		return err
	}
	received := switchCases(functionBody(string(source), "receivedNrpn"))
	answered := switchCases(functionBody(string(source), "sendNrpnParameter"))
	if len(received) < 50 || len(answered) < 50 {
		return fmt.Errorf("ls_midi.ino: found %d received and %d answered NRPN cases, the switch layout must have changed", len(received), len(answered))
	}
	var problems []string
	documented := map[int]bool{}
	for _, p := range parameters {
		documented[p.Number] = true
		code := p.Number
		if code >= 100 && code < 200 {
			code -= 100
		}
		if p.Kind == "readonly" {
			if _, ok := answered[code]; !ok {
				problems = append(problems, fmt.Sprintf("NRPN %d is read-only in midi.md but sendNrpnParameter() does not answer it", p.Number))
			}
			continue
		}
		fw, ok := received[code]
		if !ok {
			problems = append(problems, fmt.Sprintf("NRPN %d is in midi.md but receivedNrpn() has no case for it", p.Number))
			continue
		}
		if fw.ranged && p.Kind != "action" && (fw.min != p.Min || fw.max != p.Max) {
			problems = append(problems, fmt.Sprintf("NRPN %d: midi.md says %d-%d, receivedNrpn() checks inRange(value, %d, %d)", p.Number, p.Min, p.Max, fw.min, fw.max))
		}
		if _, ok := answered[code]; !ok && p.Kind != "action" {
			problems = append(problems, fmt.Sprintf("NRPN %d is settable in midi.md but sendNrpnParameter() cannot answer a query for it", p.Number))
		}
	}
	for n := range received {
		if n == 299 {
			continue
		}
		if !documented[n] {
			problems = append(problems, fmt.Sprintf("receivedNrpn() handles NRPN %d, which midi.md does not list", n))
		}
		if n < 100 && !documented[n+100] {
			problems = append(problems, fmt.Sprintf("NRPN %d exists for the left split but midi.md lists no %d for the right split", n, n+100))
		}
	}
	for n := range answered {
		if !documented[n] {
			problems = append(problems, fmt.Sprintf("sendNrpnParameter() answers NRPN %d, which midi.md does not list", n))
		}
	}
	sort.Strings(problems)
	for _, p := range problems {
		fmt.Fprintln(os.Stderr, p)
	}
	if len(problems) > 0 {
		return fmt.Errorf("midi.md and ls_midi.ino disagree on %d points", len(problems))
	}
	fmt.Printf("midi.md agrees with ls_midi.ino on %d received and %d answered NRPNs\n", len(received), len(answered))
	return nil
}
