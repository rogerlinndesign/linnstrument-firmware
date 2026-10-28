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
	"reflect"
	"strings"
	"testing"
)

func TestSelectTestsRefusesAFilterThatMatchesNothing(t *testing.T) {
	names := []string{"lowrow_strum_stops_on_release", "lowrow_bend_follows_slide", "notes_default_layout"}
	got, err := selectTests(names, "lowrow")
	if err != nil || !reflect.DeepEqual(got, names[:2]) {
		t.Errorf("lowrow: %v %v", got, err)
	}
	if got, err := selectTests(names, ""); err != nil || len(got) != 3 {
		t.Errorf("no filter: %v %v", got, err)
	}
	for _, filter := range []string{"lowRow", "nosuchtest"} {
		if got, err := selectTests(names, filter); err == nil || !strings.Contains(err.Error(), filter) {
			t.Errorf("%s selected %v, error %v", filter, got, err)
		}
	}
}

func TestUncoveredNamesTheTestsWithoutAMutationLine(t *testing.T) {
	list := []mutation{{test: "lowrow_strum_stops_on_release"}}
	got := uncovered([]string{"lowrow_strum_stops_on_release", "lowrow_new_test"}, list)
	if !reflect.DeepEqual(got, []string{"lowrow_new_test"}) {
		t.Errorf("uncovered: %v", got)
	}
}
