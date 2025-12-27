#!/bin/bash

export SETVARS_ARGS="--force" && source /opt/intel/oneapi/setvars.sh >/dev/null 2>&1 || true

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BIN_DIR="${PROJECT_ROOT}/cmake-build-valgrind/bin"
RWD_DIR="${PROJECT_ROOT}/experiment/rwd"

VALGRIND="valgrind --leak-check=full --show-leak-kinds=definite,possible --track-origins=yes --error-exitcode=1 --quiet --suppressions=${PROJECT_ROOT}/experiment/local/valgrind.supp"

GRAPHS=(
    "gnm-directed;n=23456;m=23456;seed=13"
    "rmat;directed;n=23456;m=65432;a=0.59;b=0.19;c=0.19;seed=13"
    "rmat;directed;n=65432;m=23456;a=0.25;b=0.25;c=0.25;seed=13"
)

MANIFESTS=($(ls "${RWD_DIR}"/*.manifest | xargs -n 1 basename))

NP_LIST=(4 1)

function run_benchmark() {
    local app_name="$1"
    local app="${BIN_DIR}/${app_name}"

    if [[ ! -x "$app" ]]; then
        echo "Error: $app not found or not executable."
        return 1
    fi

    # 1. Run Generated Graphs
    for np in "${NP_LIST[@]}"; do
        for graph in "${GRAPHS[@]}"; do
            output_file=$(mktemp ./tmp_gen_XXXXXX.json)
            trap "rm -f $output_file" EXIT

            echo "--------------------------------------------------------"
            echo "Running $app_name with $np processes on graph: $graph"
            echo "--------------------------------------------------------"

            mpirun -np $np $VALGRIND "$app" --kagen_option_string "$graph" --output_file "$output_file"

            if [[ $? -ne 0 ]]; then
                echo "FAILED: $app_name np=$np graph=$graph"
                exit 1
            else
                echo "SUCCESS: $app_name np=$np graph=$graph"
                if [[ -s "$output_file" ]]; then
                    echo "Result content:"
                    cat "$output_file"
                    echo ""
                fi
            fi
            rm -f "$output_file"
            trap - EXIT
        done
    done

    # 2. Run Real-World Data
    for manifest in "${MANIFESTS[@]}"; do
        manifest_path="${RWD_DIR}/${manifest}"
        if [[ ! -f "$manifest_path" ]]; then
            echo "Warning: $manifest_path not found. Skipping."
            continue
        fi

        for np in "${NP_LIST[@]}"; do
            if [[ $np -gt 4 ]]; then continue; fi

            output_file=$(mktemp -p /tmp XXXXXXXX.json)
            trap "rm -f $output_file" EXIT

            echo "--------------------------------------------------------"
            echo "Running $app_name with $np processes on RWD: $manifest"
            echo "--------------------------------------------------------"
            
            mpirun -np $np $VALGRIND "$app" --manifest_file "$manifest_path" --output_file "$output_file"
            
            if [[ $? -ne 0 ]]; then
                echo "FAILED: $app_name np=$np manifest=$manifest"
                exit 1
            else
                echo "SUCCESS: $app_name np=$np manifest=$manifest"
                if [[ -s "$output_file" ]]; then
                    echo "Result content:"
                    cat "$output_file"
                    echo ""
                fi
            fi
            rm -f "$output_file"
            trap - EXIT
        done
    done
}
