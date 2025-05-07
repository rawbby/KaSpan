#!/usr/bin/env bash
set -euo pipefail

dd="$(cd "$(dirname "$0")" && pwd)"
cd "$dd"

OUTPUT_DIR="${dd}/graphs"
mkdir -p "$OUTPUT_DIR"

FW_TOOL="${dd}/bin/iSpan_txt_to_bin_fw"
BW_TOOL="${dd}/bin/iSpan_txt_to_bin_bw"

URLS=(
  "http://konect.cc/files/download.tsv.zhishi-baidu-internallink.tar.bz2"
  "http://konect.cc/files/download.tsv.dbpedia-all.tar.bz2"
  "http://konect.cc/files/download.tsv.twitter.tar.bz2"
  "http://konect.cc/files/download.tsv.flickr-growth.tar.bz2"
  "http://konect.cc/files/download.tsv.zhishi-hudong-relatedpages.tar.bz2"
  "http://konect.cc/files/download.tsv.soc-LiveJournal1.tar.bz2"
)

process_url() {
  local url="$1"

  echo "Processing $url..."

  tmpdir="$(mktemp -d)"
  pushd "$tmpdir" > /dev/null

  # Stream download and extract
  curl -L "$url" | tar -xj > /dev/null 2>&1

  # Find meta and out file
  meta_file="$(find . -name 'meta.*' | head -n1)"
  out_file="$(find . -name 'out.*' | head -n1)"

  if [[ -z "$meta_file" || -z "$out_file" ]]; then
    echo "Missing expected files in $url"
    popd > /dev/null
    rm -rf "$tmpdir"
    return 1
  fi

  # Extract code from meta file
  code="$(grep -m1 '^code:' "$meta_file" | cut -d':' -f2 | xargs)"
  
  if [[ -z "$code" ]]; then
    echo "No code found in meta file for $url"
    popd > /dev/null
    rm -rf "$tmpdir"
    return 1
  fi

  # Run converters in out_file directory
  workdir="$(dirname "$out_file")"
  pushd "$workdir" > /dev/null

  "$FW_TOOL" "$(basename "$out_file")" > /dev/null 2>&1
  "$BW_TOOL" "$(basename "$out_file")" > /dev/null 2>&1

  # Copy and prefix result files
  files=(
    "fw_adjacent.bin"
    "head.bin"
    "out_degree.bin"
    "fw_begin.bin"
    "bw_adjacent.bin"
    "bw_head.bin"
    "in_degree.bin"
    "bw_begin.bin"
  )

  for file in "${files[@]}"; do
    cp "$file" "$OUTPUT_DIR/${code}_$file"
  done

  popd > /dev/null
  popd > /dev/null
  rm -rf "$tmpdir"

  echo "Finished processing $url"
}

# ---------- Main Execution ----------

for url in "${URLS[@]}"; do
  process_url "$url" &
done
wait

echo "All downloads and processing complete."
