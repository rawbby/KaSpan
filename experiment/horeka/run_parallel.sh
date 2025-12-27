#!/bin/bash

# Global array to store PIDs of backgrounded tasks.
CUR_PIDS=()

function run_async() {
    # Arguments: func_name app output_file ...
    local output_file="$3"
    
    if [[ -s "$output_file" ]]; then
        # Call directly to handle skipping message synchronously
        "$@"
    else
        # Run in background
        "$@" &
        CUR_PIDS+=($!)
    fi
}

function wait_all() {
    local any_failed=0
    for pid in "${CUR_PIDS[@]}"; do
        if ! wait "$pid"; then
            any_failed=1
        fi
    done
    CUR_PIDS=()
    return $any_failed
}
