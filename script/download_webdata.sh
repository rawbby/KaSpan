#!/bin/bash
BASENAMES=(
"uk-2014"
"eu-2015"
"gsh-2015"
"uk-2014-host"
"eu-2015-host"
"gsh-2015-host"
"uk-2014-tpd"
"eu-2015-tpd"
"gsh-2015-tpd"
"clueweb12"
"uk-2002"
"indochina-2004"
"it-2004"
"arabic-2005"
"sk-2005"
"uk-2005"
)
for base in "${BASENAMES[@]}"; do
(
for ext in .properties .graph .md5sums; do
url="http://data.law.di.unimi.it/webdata/${base}/${base}${ext}"
wget -c "$url" || echo "[!] Failed to download: $url" >&2
done
) &
done
wait
echo "All downloads complete. Starting verification..."
for base in "${BASENAMES[@]}"; do
if [ -f "${base}.md5sums" ]; then
echo "--- Verification for ${base} ---"
md5sum -c "${base}.md5sums" 2>&1 | grep "FAILED" || echo "[PASS] ${base} integrity confirmed or no major issues found."
else
echo "[!] Warning: ${base}.md5sums not found."
fi
done
