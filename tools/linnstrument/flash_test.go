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
	"path/filepath"
	"runtime"
	"testing"
)

func TestFindBossacPrefersTheBundledCopy(t *testing.T) {
	if runtime.GOOS != "darwin" && runtime.GOOS != "windows" {
		t.Skip("no bundled bossac for this platform")
	}
	repo := t.TempDir()
	rel := filepath.Join("macos", "bossac")
	if runtime.GOOS == "windows" {
		rel = filepath.Join("windows", "bossac.exe")
	}
	want := filepath.Join(repo, "tools", "bossac", rel)
	if err := os.MkdirAll(filepath.Dir(want), 0o755); err != nil {
		t.Fatal(err)
	}
	if err := os.WriteFile(want, []byte("x"), 0o755); err != nil {
		t.Fatal(err)
	}
	t.Setenv("LINN_BOSSAC", "")
	got, err := findBossac(repo)
	if err != nil || got != want {
		t.Fatalf("got %q, %v; want %q", got, err, want)
	}
	t.Setenv("LINN_BOSSAC", "/elsewhere/bossac")
	if got, _ := findBossac(repo); got != "/elsewhere/bossac" {
		t.Fatalf("LINN_BOSSAC must win, got %q", got)
	}
}

func TestBundledBossacIsInTheRepository(t *testing.T) {
	repo, err := repoRoot()
	if err != nil {
		t.Skip("not in the repository")
	}
	for _, rel := range []string{filepath.Join("macos", "bossac"), filepath.Join("windows", "bossac.exe"), "LICENSE"} {
		if _, err := os.Stat(filepath.Join(repo, "tools", "bossac", rel)); err != nil {
			t.Errorf("tools/bossac/%s: %v", rel, err)
		}
	}
}
