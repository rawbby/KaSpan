#!/bin/bash

# Function to get the correct binary path based on edge count m
# Usage: get_binary_path <binary_name> <m>
function get_binary_path() {
  local binary_name="$1"
  local m="$2"
  local base_dir="$HOME/workspace/KaSpan"
  local threshold=2147483647 # 2^31 - 1

  if (( m > threshold )); then
    echo "${base_dir}/build/release64/bin/${binary_name}"
  else
    echo "${base_dir}/build/release/bin/${binary_name}"
  fi
}
