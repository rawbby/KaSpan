#!/bin/bash
function run_generic() {
local app="$1"
local output_file="$2"
local kagen_string="$3"
local app_name="$4"
local np="$5"
local n="$6"
local d="$7"
local graph="$8"
local srun_args="$9"
shift 9
local msg="${app_name} np=${np}"
[[ -n "$n" ]] && msg="${msg} n=${n}"
[[ -n "$d" ]] && msg="${msg} d=${d}"
msg="${msg} graph=${graph}"
m=$(( n * d / 100 ))
if [[ -s "$output_file" ]] || [[ $m -gt 4000000000 ]]; then
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
--kagen_option_string "$kagen_string" \
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
