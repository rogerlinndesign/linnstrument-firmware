#!/bin/sh
# Copyright 2026 Roger Linn Design (https://www.rogerlinndesign.com)
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Publishes the browser pages in examples/browser/ to https://uwyn.com/linnstrument/.
#
#   tools/deploy-pages.sh        publish
#   tools/deploy-pages.sh -n     show what would change, publish nothing
#
# The server is named by LINN_PAGES_TARGET, an rsync destination such as user@host:folder/,
# set by whoever has access to it. It stays out of this public repository so that the origin
# server behind Cloudflare isn't advertised.
#
# Only what the pages load goes up: the tests stay here, and --delete removes anything else in
# that one folder on the server. Cloudflare keeps scripts and stylesheets for four hours while
# the HTML is never cached, so the published HTML asks for each script and stylesheet with its
# content hash (connection.js?v=1a2b3c4d), and a change reaches visitors at once.
set -e
ROOT=$(cd "$(dirname "$0")/.." && pwd)
TARGET=${LINN_PAGES_TARGET:?set LINN_PAGES_TARGET to the rsync destination of the pages, such as user@host:folder/}
STAGE=$(mktemp -d)
trap 'rm -rf "$STAGE"' EXIT

cd "$ROOT/examples/browser"
for f in *.html *.js *.css; do
  case "$f" in *.test.js) continue ;; esac
  cp "$f" "$STAGE/"
done
for f in *.js *.css; do
  case "$f" in *.test.js) continue ;; esac
  hash=$(shasum -a 256 "$f" | cut -c1-8)
  for page in "$STAGE"/*.html; do
    sed -i.bak -e "s|src=\"$f\"|src=\"$f?v=$hash\"|g" -e "s|href=\"$f\"|href=\"$f?v=$hash\"|g" "$page"
  done
done
rm -f "$STAGE"/*.bak

# the staging folder is private (mktemp), and rsync copies its permissions onto the server's
# folder, which would lock Apache out of it: the stage gets the permissions the web server needs
chmod 755 "$STAGE"
chmod 644 "$STAGE"/*
rsync -rlptz --delete --itemize-changes "$@" "$STAGE/" "$TARGET"
case " $* " in
  *" -n "*) echo "dry run, nothing was published" ;;
  *) echo "published to https://uwyn.com/linnstrument/" ;;
esac
