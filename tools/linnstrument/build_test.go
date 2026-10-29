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
	"os"
	"os/exec"
	"path/filepath"
	"strings"
	"testing"
)

func TestFirmwareVersionIsWhatTheScreenShows(t *testing.T) {
	for sketch, want := range map[string]string{
		"const char* OSVersion = \"240.\";\nconst char* OSVersionBuild = \".075\";\n": "240.075",
		"const char* OSVersion = \"234\";\nconst char* OSVersionBuild = \".074\";\n":  "234.074",
	} {
		if got, err := firmwareVersion(sketch); err != nil || got != want {
			t.Errorf("%q gives %q, %v; want %q", sketch, got, err, want)
		}
	}
	if _, err := firmwareVersion("no version here"); err == nil {
		t.Error("a sketch without a version gives no error")
	}
}

func TestFirmwareNameCarriesTheCommitAndLocalChanges(t *testing.T) {
	repo := t.TempDir()
	sketch := filepath.Join(repo, sketchName+".ino")
	os.WriteFile(sketch, []byte("const char* OSVersion = \"240.\";\nconst char* OSVersionBuild = \".075\";\n"), 0o644)
	git := func(args ...string) string {
		out, err := exec.Command("git", append([]string{"-C", repo, "-c", "user.name=t", "-c", "user.email=t@t", "-c", "commit.gpgsign=false"}, args...)...).Output()
		if err != nil {
			t.Fatalf("git %v: %v", args, err)
		}
		return strings.TrimSpace(string(out))
	}
	git("init", "-q")
	git("add", ".")
	git("commit", "-q", "-m", "sketch")
	commit := git("rev-parse", "--short=7", "HEAD")
	if got, _ := firmwareName(repo); got != "240.075-"+commit {
		t.Errorf("clean checkout is named %q", got)
	}
	os.WriteFile(sketch, []byte("const char* OSVersion = \"240.\";\nconst char* OSVersionBuild = \".076\";\n"), 0o644)
	if got, _ := firmwareName(repo); got != "240.076-"+commit+"-modified" {
		t.Errorf("changed sketch is named %q", got)
	}
}
