#!/usr/bin/env bash
source "$(dirname "$0")/env.sh"

THREAD_LIST=(1 2 3 4 6 8)
CODE_LIST=(FL LJ WE RD)

for THREAD in "${THREAD_LIST[@]}"; do
  for CODE in "${CODE_LIST[@]}"; do

    echo "---"

    SIZE_LIST=(1 2 3 4 6 8 12 16 24 32)

    RESULT_DIRS=()
    for N in "${SIZE_LIST[@]}"; do
      RESULT_DIR=$( "$(dirname "$0")/benchmark_iSpan.sh" --code "$CODE" -n "$N" -t "$THREAD" | sed -n 's/.*(\([^)]*\)).*/\1/p')
      RESULT_DIRS+=("$RESULT_DIR")
      printf '  %s\n' "$RESULT_DIR"
    done

    RESULT_DIR="./result/$(date +%Y%m%d_%H%M%S)_$(printf "%03d" $((RANDOM%1000)))"
    mkdir -p "${RESULT_DIR}"
    "$(dirname "$0")/hw_info.sh" "${RESULT_DIR}"

    python3 "$(dirname "$0")/plot.py" "$RESULT_DIR" "${RESULT_DIRS[@]}"
    printf '  %s\n' "$RESULT_DIR"

  done
done

echo "---"

deactivate # env.sh
