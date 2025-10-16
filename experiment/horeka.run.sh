#!/bin/bash

export SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export PROJECT_DIR="${SCRIPT_DIR}/.."
export BUILD_DIR="${PROJECT_DIR}/cmake-build-release"
export EXPERIMENT_DATA_DIR="${BUILD_DIR}/experiment_data"

FLAG_S=0
FLAG_M=0
FLAG_X=0
FLAD_DRY=0
POSITIONAL=()
while [[ $# -gt 0 ]]; do
  case "$1" in
    -s       ) FLAG_S=1;   shift ;;
    -m       ) FLAG_M=1;   shift ;;
    -x       ) FLAG_X=1;   shift ;;
    -d|--dry ) FLAG_DRY=1; shift ;;
    --       ) shift; break ;;
    -*       ) echo "Unknown flag: $1"; exit 1 ;;
    *        ) POSITIONAL+=("$1");      shift  ;;
  esac
done
set -- "${POSITIONAL[@]}"

module purge
module load compiler/gnu/14
module load mpi/impi/2021.11
module load devel/cmake/3.30

cmake -S "${PROJECT_DIR}" -B "${BUILD_DIR}" -G Ninja -Wno-dev -DCMAKE_BUILD_TYPE=Release
source "${PROJECT_DIR}/.venv/bin/activate"

cd "${BUILD_DIR}"
cmake --build "${BUILD_DIR}"

PROG_ENTRY_LIST=(
  "kaspan|${BUILD_DIR}/bin/bench_kaspan"
  "kaspan_async|${BUILD_DIR}/bin/bench_kaspan --async"
  "ispan12|${BUILD_DIR}/bin/bench_ispan --alpha 12"
)
LOG_NTASKS_LIST=(6 7 8)
GRAPH_D_PER_CENT_LIST=(60 90 140 210)

(( FLAG_M )) && {
  LOG_NTASKS_LIST=(4 5 6 7 8 9 10)
  GRAPH_D_PER_CENT_LIST=(50 70 110 170 250)
}
(( FLAG_X )) && {
  LOG_NTASKS_LIST=(1 2 3 4 5 6 7 8 9 10 11 12)
  GRAPH_D_PER_CENT_LIST=(50 60 70 90 110 140 170 210 250)
}

export PARTITION="cpuonly"
export MEM="230gb"
CPUS_PER_NODE=76
DATE=$(date +%F)
SEED=$(tr -dc '0-9' </dev/urandom | head -c 8)

export BASE_DIR="${EXPERIMENT_DATA_DIR}/${DATE}-${SEED}"
INDEX_FILE="${BASE_DIR}/index.json"

mkdir "${EXPERIMENT_DATA_DIR}"
mkdir "${BASE_DIR}"
touch "${INDEX_FILE}"
printf "%s" "{" >> "${INDEX_FILE}"

NOT_FIRST_PROG=0

for PROG_ENTRY in "${PROG_ENTRY_LIST[@]}"; do
  PROG_TAG=${PROG_ENTRY%%|*}
  PROG=${PROG_ENTRY#*|}

  (( NOT_FIRST_PROG )) && printf "%s" "," >> "${INDEX_FILE}"
  printf "\n  %s" "\"${PROG_TAG}\": [" >> "${INDEX_FILE}"
  NOT_FIRST_PROG=1
  NOT_FIRST_ENTRY=0

  for LOG_NTASKS in "${LOG_NTASKS_LIST[@]}"; do
    export NTASKS=$(( 2**LOG_NTASKS ))
    export NTASKS_PER_NODE=$(( NTASKS < ${CPUS_PER_NODE} ? NTASKS : ${CPUS_PER_NODE} ))
    export NODES=$(( (NTASKS + ${CPUS_PER_NODE} - 1) / ${CPUS_PER_NODE} ))
    TIME_MINUTES=$(( LOG_NTASKS + 1 ))
    export TIME_SECONDS=$(( TIME_MINUTES * 60 ))
    export TIME=$(printf '%d-%02d:%02d:00' $((TIME_MINUTES/1440)) $(((TIME_MINUTES%1440)/60)) $((TIME_MINUTES%60)))

    for GRAPH_D_PER_CENT in "${GRAPH_D_PER_CENT_LIST[@]}"; do
      GRAPH_BIG_N=$(( LOG_NTASKS + 19 )) # 2**25 vertices per rank
      GRAPH_N=$(( 2**(GRAPH_BIG_N) ))
      GRAPH_M=$(( (GRAPH_N * GRAPH_D_PER_CENT) / 100 ))
      GRAPH_A=0.57
      GRAPH_B=0.19
      GRAPH_C=0.19

      export JOB_NAME="${PROG_TAG}_rmat_np${NTASKS}_N${GRAPH_BIG_N}_d${GRAPH_D_PER_CENT}"
      RES_FILE="${BASE_DIR}/horeka.job.${JOB_NAME}.json"
      JOB_SCRIPT="${BASE_DIR}/horeka.job.${JOB_NAME}.sh"

      (( NOT_FIRST_ENTRY )) &&  printf "%s" "," >> "${INDEX_FILE}"
      printf "\n    %s" "\"${RES_FILE}\"" >> "${INDEX_FILE}"
      NOT_FIRST_ENTRY=1

      export OUT_FILE="${BASE_DIR}/horeka.job.${JOB_NAME}.out"
      export ERR_FILE="${BASE_DIR}/horeka.job.${JOB_NAME}.err"
      export CMD="${PROG} --kagen_option_string \"rmat;n=${GRAPH_N};m=${GRAPH_M};a=${GRAPH_A};b=${GRAPH_B};c=${GRAPH_C};directed;s=${SEED}\" --output_file ${RES_FILE}"

      envsubst < "${SCRIPT_DIR}/horeka.job.sh" > "${JOB_SCRIPT}"

      if (( FLAG_DRY )); then
        echo ""
        echo "${JOB_SCRIPT}"
        echo '```bash'
        cat "${JOB_SCRIPT}"
        echo '```'
      fi
    done
  done

  printf "\n  %s" "]" >> "${INDEX_FILE}"

done

printf "\n%s\n" "}" >> "${INDEX_FILE}"

cp "${SCRIPT_DIR}/horeka.queue.sh" "${BASE_DIR}/horeka.queue.sh"
cp "${SCRIPT_DIR}/horeka.cancel.sh" "${BASE_DIR}/horeka.cancel.sh"
cp "${SCRIPT_DIR}/horeka.watch.sh" "${BASE_DIR}/horeka.watch.sh"
cp "${SCRIPT_DIR}/horeka.check.sh" "${BASE_DIR}/horeka.check.sh"
chmod +x "${BASE_DIR}"/*.sh

if (( FLAG_DRY )); then
  echo ""
  echo "${INDEX_FILE}"
  echo '```json'
  cat "${INDEX_FILE}"
  echo '```'
else
  "${BASE_DIR}/horeka.queue.sh"
fi
