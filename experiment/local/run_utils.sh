#!/bin/bash

# Function to run generic experiments
function run_generic() {
    local app="$1"
    local output_file="$2"
    local kagen_string="$3"
    local app_name="$4"
    local np="$5"
    local n="$6"
    local d="$7"
    local graph="$8"
    shift 8
    # Remaining args are passed to the app

    local msg="${app_name} np=${np}"
    [[ -n "$n" ]] && msg="${msg} n=${n}"
    [[ -n "$d" ]] && msg="${msg} d=${d}"
    msg="${msg} graph=${graph}"

    if [[ -s "$output_file" ]]; then
        echo "[SKIPPING] ${msg}"
    else
        echo "[STARTING] ${msg}"
        
        # Wrapping with systemd-run for memory limit
        # mpirun with timeout and pinning
        # We use -H localhost:$np for local execution
        # --bind-to core for pinning
        # timeout 20m for 20 minutes limit
        
        sudo systemd-run --scope -p MemoryMax=20G --slice=machine.slice \
            nice -n -10 \
            timeout 20m \
            bash -c "export SETVARS_ARGS='--force' && source /opt/intel/oneapi/setvars.sh >/dev/null 2>&1 || true && mpirun -np $np \
            --bind-to core \
            --map-by core \
            $app \
            --output_file $output_file \
            --kagen_option_string '$kagen_string' \
            $*"
            
        local ec=$?
        if [[ $ec -ne 0 ]]; then
            echo "{\"ec\":$ec}" > "$output_file"
            local display_ec=$ec
            [[ $ec -eq 124 ]] && display_ec="${ec} (timeout)"
            [[ $ec -eq 137 ]] && display_ec="${ec} (oom/killed)"
            echo "[FAILURE] ${msg} ec=${display_ec}"
        else
            echo "[SUCCESS] ${msg}"
        fi
        return $ec
    fi
}

# Function to run real-world experiments
function run_rwd() {
    local app="$1"
    local output_file="$2"
    local manifest="$3"
    local app_name="$4"
    local np="$5"
    local graph="$6"
    shift 6
    # Remaining args are passed to the app

    local msg="${app_name} np=${np} graph=${graph}"

    if [[ -s "$output_file" ]]; then
        echo "[SKIPPING] ${msg}"
    else
        echo "[STARTING] ${msg}"
        
        sudo systemd-run --scope -p MemoryMax=20G --slice=machine.slice \
            nice -n -10 \
            timeout 20m \
            bash -c "export SETVARS_ARGS='--force' && source /opt/intel/oneapi/setvars.sh >/dev/null 2>&1 || true && mpirun -np $np \
            --bind-to core \
            --map-by core \
            $app \
            --output_file $output_file \
            --manifest_file $manifest \
            $*"
            
        local ec=$?
        if [[ $ec -ne 0 ]]; then
            echo "{\"ec\":$ec}" > "$output_file"
            local display_ec=$ec
            [[ $ec -eq 124 ]] && display_ec="${ec} (timeout)"
            [[ $ec -eq 137 ]] && display_ec="${ec} (oom/killed)"
            echo "[FAILURE] ${msg} ec=${display_ec}"
        else
            echo "[SUCCESS] ${msg}"
        fi
        return $ec
    fi
}
