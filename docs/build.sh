#!/usr/bin/env bash
# Builds the Sphinx + MyST + Breathe documentation site into build/docs/html.
# Needs the dev shell (doxygen + the docs python environment). Extra arguments
# are passed to sphinx-build, e.g. `-W` to fail on warnings.
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
out="$root/build/docs/html"

export MOBA_SIM_ROOT="$root"
export MOBA_SIM_DOXYGEN_XML="${TMPDIR:-/tmp}/moba-sim-doxygen-xml"
export MOBA_SIM_DOCS_WARN_AS_ERROR="${MOBA_SIM_DOCS_WARN_AS_ERROR:-NO}"

rm -rf "$MOBA_SIM_DOXYGEN_XML"
doxygen "$root/docs/doxygen-xml.conf"
sphinx-build -b html -c "$root/docs" "$root" "$out" "$@"

echo "docs: $out/index.html"
