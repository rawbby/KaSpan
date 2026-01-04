#!/bin/bash
BASENAMES=(
"enwiki-2013"
"enwiki-2015"
"enwiki-2016"
"enwiki-2017"
"enwiki-2018"
"enwiki-2019"
"enwiki-2020"
"enwiki-2021"
"enwiki-2022"
"enwiki-2023"
"enwiki-2024"
"enwiki-2025"
"itwiki-2013"
"eswiki-2013"
"frwiki-2013"
"dewiki-2013"
"enron"
"amazon-2008"
"ljournal-2008"
"orkut-2007"
"hollywood-2009"
"hollywood-2011"
"imdb-2021"
"dblp-2010"
"dblp-2011"
"hu-tel-2006"
"twitter-2010"
"wordassociation-2011"
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
