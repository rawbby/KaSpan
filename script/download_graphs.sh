#!/usr/bin/env bash
source "$(dirname "$0")/env.sh"

# todo: make logging more consistent

# Default flags
NO_FETCH=0
NO_CONVERSION=0
NO_CACHE=0
CODES_ARG=""

usage() {
  cat <<EOF
Usage: $0 [--no-fetch] [--no-conversion] [--no-cache] [--codes <list>]

Options:
  --no-fetch         Skip fetching edgelists if already present in \$GRAPH_DIR
  --no-conversion    Skip converting edgelist to binary if outputs already exist
  --no-cache         Force re-fetching and re-conversion (ignore existing cache)
  --codes LIST       Comma or semicolon separated list of codes to process
EOF
}

# Parse flags
while [[ $# -gt 0 ]]; do
  case $1 in
    --no-fetch)      NO_FETCH=1; shift ;;
    --no-conversion) NO_CONVERSION=1; shift ;;
    --no-cache)      NO_CACHE=1; shift ;;
    --codes)         CODES_ARG="$2"; shift 2 ;;
    -h|--help)       usage; deactivate; exit 0 ;;
    *)               break ;;
  esac
done

# Define sources: "<code>|<url-or-path>|<method>"
SOURCES=(
  "BAi|http://konect.cc/files/download.tsv.zhishi-baidu-internallink.tar.bz2|konnect"
  "DB|http://konect.cc/files/download.tsv.dbpedia-all.tar.bz2|konnect"
  "TW|http://konect.cc/files/download.tsv.twitter.tar.bz2|konnect"
  "FL|http://konect.cc/files/download.tsv.flickr-growth.tar.bz2|konnect"
  "HUr|http://konect.cc/files/download.tsv.zhishi-hudong-relatedpages.tar.bz2|konnect"
  "LJ|http://konect.cc/files/download.tsv.soc-LiveJournal1.tar.bz2|konnect"
  "TOY|${PROJECT_DIR}/solver/iSpan/graph_converter/toy_edge_list.txt|local:copy"
)

# Filter SOURCES by --codes <list>
if [[ -n "$CODES_ARG" ]]; then
  IFS=',;' read -r -a CODES_FILTER <<< "$CODES_ARG"
  all_codes=()
  filtered=()
  for entry in "${SOURCES[@]}"; do
    code="${entry%%|*}"
    all_codes+=("$code")
    if printf '%s\n' "${CODES_FILTER[@]}" | grep -Fxq "$code"; then
      filtered+=("$entry")
    fi
  done
  for c in "${CODES_FILTER[@]}"; do
    if ! printf '%s\n' "${all_codes[@]}" | grep -Fxq "$c"; then
      echo "[download_graphs.sh] warning: code '$c' passed to --codes but not found in SOURCES"
    fi
  done
  SOURCES=("${filtered[@]}")
fi

# Permission helpers
make_readonly() { chmod a-w "$1" 2>/dev/null || true; }
make_writable() { chmod u+w "$1" 2>/dev/null || true; }

# Fetch functions
fetch_konnect() {
  local code="$1" url="$2" target="${GRAPH_DIR}/${code}_edgelist.txt"
  echo "[download_graphs.sh][fetch] konnect: $code"
  make_writable "$target"
  [[ $NO_CACHE -eq 1 && -f "$target" ]] && rm -f "$target"
  [[ -f "$target" && $NO_FETCH -eq 1 && $NO_CACHE -eq 0 ]] && { echo "  -> exists, skipping"; return; }
  tmpdir=$(mktemp -d)
  pushd "$tmpdir" >/dev/null
  curl -sL "$url" | tar -xj >/dev/null
  meta=$(find . -name 'meta.*' | head -n1)
  out=$(find . -name 'out.*'  | head -n1)
  if [[ -z "$meta" || -z "$out" ]]; then echo "  -> ! missing meta or out"; popd >/dev/null; rm -rf "$tmpdir"; return 1; fi
  code_file=$(grep -m1 '^code:' "$meta" | cut -d: -f2 | xargs)
  [[ "$code_file" != "$code" ]] && echo "  -> ! code mismatch: expected $code, got $code_file"
  cp "$out" "$target"
  make_readonly "$target"
  popd >/dev/null; rm -rf "$tmpdir"
}

fetch_local_copy() {
  local code="$1" src="$2" target="${GRAPH_DIR}/${code}_edgelist.txt"
  echo "[download_graphs.sh][fetch] local-copy: $code"
  make_writable "$target"
  [[ $NO_CACHE -eq 1 && -f "$target" ]] && rm -f "$target"
  [[ -f "$target" && $NO_FETCH -eq 1 && $NO_CACHE -eq 0 ]] && { echo "  -> exists, skipping"; return; }
  cp "$src" "$target"
  make_readonly "$target"
}

fetch_local_link() {
  local code="$1" src="$2" target="${GRAPH_DIR}/${code}_edgelist.txt"
  echo "[download_graphs.sh][fetch] local-link: $code"
  [[ $NO_CACHE -eq 1 && -e "$target" ]] && rm -f "$target"
  [[ -e "$target" && $NO_FETCH -eq 1 && $NO_CACHE -eq 0 ]] && { echo "  -> exists, skipping"; return; }
  ln -sf "$src" "$target"
  make_readonly "$target"
}

# Convert using a temporary working directory
convert_graph() {
  local code="$1"
  local edgelist="${GRAPH_DIR}/${code}_edgelist.txt"
  local bins=(fw_adjacent.bin head.bin out_degree.bin fw_begin.bin bw_adjacent.bin bw_head.bin in_degree.bin bw_begin.bin)

  [[ ! -f "$edgelist" ]] && { echo "[download_graphs.sh][convert] $code -> no edgelist, skipping"; return 1; }
  make_writable "$edgelist"
  [[ $NO_CACHE -eq 1 ]] && rm -f "${GRAPH_DIR}/${code}_"*.bin

  local missing=0
  for f in "${bins[@]}"; do
    [[ ! -f "${GRAPH_DIR}/${code}_$f" ]] && missing=1 && break
  done
  if [[ $missing -eq 0 && $NO_CONVERSION -eq 1 && $NO_CACHE -eq 0 ]]; then
    echo "[download_graphs.sh][convert] $code -> binaries exist, skipping"
    return
  fi

  echo "[download_graphs.sh][convert] processing: $code"
  tmpdir=$(mktemp -d)
  cp "$edgelist" "$tmpdir/"
  pushd "$tmpdir" >/dev/null
  "${BINARY_DIR}/iSpan_txt_to_bin_fw" "${code}_edgelist.txt" >/dev/null 2>&1
  "${BINARY_DIR}/iSpan_txt_to_bin_bw" "${code}_edgelist.txt" >/dev/null 2>&1
  for f in "${bins[@]}"; do
    if [[ -f "$f" ]]; then
      mv "$f" "${GRAPH_DIR}/${code}_$f"
      make_readonly "${GRAPH_DIR}/${code}_$f"
    else
      echo "[download_graphs.sh][convert] error: missing file $f after conversion ran successfully ($code)!"
    fi
  done
  popd >/dev/null
  rm -rf "$tmpdir"
}

# Main
echo "[download_graphs.sh] Starting fetch & conversion..."
for entry in "${SOURCES[@]}"; do
  IFS='|' read -r code loc method <<< "$entry"
  case "$method" in
    konnect)    fetch_konnect "$code" "$loc" ;;
    local:copy) fetch_local_copy "$code" "$loc" ;;
    local:link) fetch_local_link "$code" "$loc" ;;
    *)          echo "[download_graphs.sh] warning: Unknown method '$method' for $code" ;;
  esac
  convert_graph "$code"
done

echo "[download_graphs.sh] All done."
deactivate
