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
	"strings"
	"testing"
)

func TestBuildNameUsesTheStampedVersion(t *testing.T) {
	kept := version
	defer func() { version = kept }()
	version = "240.075-1a2b3c4"
	if got := buildName(); got != "240.075-1a2b3c4" {
		t.Errorf("stamped build is named %q", got)
	}
	version = ""
	if got := buildName(); !strings.HasPrefix(got, "development build") {
		t.Errorf("unstamped build is named %q", got)
	}
}
