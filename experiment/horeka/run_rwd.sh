#!/bin/bash

function run_rwd() {
    local app="$1"
    local output_file="$2"
    local manifest="$3"
    local app_name="$4"
    local np="$5"
    local manifest_name="$6"
    local srun_args="$7"
    shift 7

    local msg="${app_name} np=${np} graph=${manifest_name}"

    if [[ -s "$output_file" ]]; then
        echo "[SKIPPING] ${msg}"
    else
        echo "[STARTING] ${msg}"
        srun \
            --time=3:00 \
            --oom-kill-step=1 \
            --mpi=pmi2 \
            --exclusive \
            --cpu-bind=cores \
            $srun_args \
            "$app" \
            --output_file "$output_file" \
            --manifest_file "$manifest" \
            "$@"
        local ec=$?
        if [[ $ec -ne 0 ]]; then
            echo "{\"ec\":$ec}" > "$output_file"
            local display_ec=$ec
            [[ $ec -eq 137 ]] && display_ec="${ec} (oom)"
            echo "[FAILURE] ${msg} ec=${display_ec}"
        else
            echo "[SUCCESS] ${msg}"
        fi
        return $ec
    fi
}
