#!/usr/bin/env bash
# Smoke-run examples against committed fixtures (same patterns as tests/CI).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="${LIBPQCAP_BUILD_DIR:-${ROOT}/build}"
FIXTURES="${ROOT}/tests/fixtures"
OUT="${BUILD}/examples/out"

CONVERT="${BUILD}/examples/convert_pcapng"
EXTRACT="${BUILD}/examples/extract_metadata"
SCAN="${BUILD}/examples/scan_packets"

fail() {
  echo "FAIL: $1"
  exit 1
}

for bin in "${CONVERT}" "${EXTRACT}" "${SCAN}"; do
  [[ -x "${bin}" ]] || fail "missing ${bin}; run: cmake --build build"
done

for f in \
  "${FIXTURES}/http.pcapng" \
  "${FIXTURES}/http_metadata.parquet" \
  "${FIXTURES}/udp1.pcapng" \
  "${FIXTURES}/minimal_metadata.parquet"; do
  [[ -f "${f}" ]] || fail "missing fixture ${f}"
done

mkdir -p "${OUT}"

echo "==> scan plain http.pcapng"
"${SCAN}" "${FIXTURES}/http.pcapng" | head -1 | grep -q ": 43$" || fail "expected 43 packets in http.pcapng"

echo "==> convert http.pcapng -> pqcap"
"${CONVERT}" \
  "${FIXTURES}/http.pcapng" \
  "${FIXTURES}/http_metadata.parquet" \
  "${OUT}/http.pqcapng"

echo "==> extract metadata"
"${EXTRACT}" "${OUT}/http.pqcapng" "${OUT}/http.extracted.parquet"

python3 - <<'PY' "${OUT}/http.pqcapng" "${FIXTURES}/http_metadata.parquet" "${OUT}/http.extracted.parquet" "${FIXTURES}/http.pcapng"
import pathlib, struct, sys

pqcap_path = pathlib.Path(sys.argv[1])
ref_path = pathlib.Path(sys.argv[2])
extracted_path = pathlib.Path(sys.argv[3])
plain_path = pathlib.Path(sys.argv[4])

ref = ref_path.read_bytes()
extracted = extracted_path.read_bytes()
if extracted != ref:
    raise SystemExit("extracted parquet does not match fixture")

data = pqcap_path.read_bytes()
footer = data[-44:]
_, magic, _, _, off, length = struct.unpack_from("<I8sHHQQ", footer, 8)
if magic != b"PQCAPFTR":
    raise SystemExit("missing PQCAPFTR footer")
if data[off : off + length] != ref:
    raise SystemExit("footer slice mismatch")

plain_bytes = plain_path.read_bytes()
if data[: len(plain_bytes)] != plain_bytes:
    raise SystemExit("capture prefix changed after convert")
PY

echo "==> scan indexed http.pqcapng (same packet count)"
"${SCAN}" "${OUT}/http.pqcapng" | head -1 | grep -q ": 43$" || fail "expected 43 packets after convert"

echo "==> convert udp1.pcapng"
"${CONVERT}" \
  "${FIXTURES}/udp1.pcapng" \
  "${FIXTURES}/minimal_metadata.parquet" \
  "${OUT}/udp1.pqcapng"

if command -v tshark >/dev/null 2>&1; then
  echo "==> tshark readability (optional)"
  PLAIN_HTTP="$(tshark -r "${FIXTURES}/http.pcapng" -Y http -T fields -e frame.number 2>/dev/null | wc -l | tr -d ' ')"
  CONV_HTTP="$(tshark -r "${OUT}/http.pqcapng" -Y http -T fields -e frame.number 2>/dev/null | wc -l | tr -d ' ')"
  [[ "${CONV_HTTP}" == "${PLAIN_HTTP}" ]] || fail "http frame count changed after convert"
  echo "PASS tshark http frames=${PLAIN_HTTP}"
else
  echo "SKIP tshark (not installed)"
fi

echo "PASS examples smoke"
