#!/usr/bin/env bash
source "$(dirname "$0")/env.sh"

[ $# -eq 1 ] || { echo "Usage: $0 RESULT_DIR" >&2; exit 1; }
OUT_DIR=$1

get_lscpu() {
  echo "$LSCPU" | awk -F':' -v k="$1" '$1==k {gsub(/^[ \t]+|[ \t]+$/, "", $2); print $2; exit}'
}

to_kib() {                          # arg: "NUM UNIT"
  read -r num unit <<<"$1"          # split “2.5 GiB” -> num=2.5  unit=GiB
  [[ $num == .* ]] && num="0${num}" # .5 → 0.5

  case $unit in
    ''|KiB|kB|KB) printf '%s\n' "$num" ;;
    MiB|MB)       awk -v n="$num" 'BEGIN{printf "%.0f\n", n*1024}' ;;
    GiB|GB)       awk -v n="$num" 'BEGIN{printf "%.0f\n", n*1024*1024}' ;;
    *)            printf '0\n' ;;
  esac
}

write() {
  val="$2"
  [[ $val == .* ]] && val="0${val}"
  printf '%s\n' "$val" > "$OUT_DIR/$1";
}

LSCPU=$(LC_ALL=C lscpu)
host_name=$(hostname)
cpu_vendor=$(get_lscpu "Vendor ID")
cpu_model_name=$(get_lscpu "Model name")
cpu_architecture=$(get_lscpu "Architecture")
cpu_family=$(get_lscpu "CPU family")
cpu_model=$(get_lscpu "Model")
cpu_stepping=$(get_lscpu "Stepping")
cpu_sockets=$(get_lscpu "Socket(s)")
cpu_cores_per_socket=$(get_lscpu "Core(s) per socket")
cpu_threads_per_core=$(get_lscpu "Thread(s) per core")
cpu_cores=$((cpu_sockets * cpu_cores_per_socket))
cpu_threads=$((cpu_cores * cpu_threads_per_core))
cpu_base_mhz=$(get_lscpu "CPU MHz")
cpu_max_mhz=$(get_lscpu "CPU max MHz")
numa_nodes=$(get_lscpu "NUMA node(s)")
cache_l1d_kib=$(to_kib "$(get_lscpu 'L1d cache')")
cache_l1i_kib=$(to_kib "$(get_lscpu 'L1i cache')")
cache_l2_kib=$(to_kib  "$(get_lscpu 'L2 cache')")
cache_l3_kib=$(to_kib  "$(get_lscpu 'L3 cache')")
memory_total_kib=$(awk '/MemTotal:/ {print $2}' /proc/meminfo)

write host_name              "$host_name"
write cpu_vendor             "$cpu_vendor"
write cpu_model_name         "$cpu_model_name"
write cpu_architecture       "$cpu_architecture"
write cpu_family             "$cpu_family"
write cpu_model              "$cpu_model"
write cpu_stepping           "$cpu_stepping"
write cpu_sockets            "$cpu_sockets"
write cpu_cores_per_socket   "$cpu_cores_per_socket"
write cpu_threads_per_core   "$cpu_threads_per_core"
write cpu_cores              "$cpu_cores"
write cpu_threads            "$cpu_threads"
write cpu_base_mhz           "$cpu_base_mhz"
write cpu_max_mhz            "$cpu_max_mhz"
write cache_l1d_kib          "$cache_l1d_kib"
write cache_l1i_kib          "$cache_l1i_kib"
write cache_l2_kib           "$cache_l2_kib"
write cache_l3_kib           "$cache_l3_kib"
write numa_nodes             "$numa_nodes"
write memory_total_kib       "$memory_total_kib"

deactivate  # deactivate env.sh
