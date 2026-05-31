#!/usr/bin/env bash
# Convert plain PCAP-NG captures to pqcap format and verify wire readability is retained.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="${LIBPQCAP_BUILD_DIR:-${ROOT}/build}"
FIXTURES="${ROOT}/tests/fixtures"
OUT_DIR="${LIBPQCAP_TEST_OUT:-${BUILD}/integration}"
CONVERT_CLI="${BUILD}/pqcap_convert_cli"

fail() {
  echo "FAIL: $1"
  exit 1
}

if [[ "${LIBPQCAP_SKIP_TSHARK:-}" == "1" ]]; then
  echo "SKIP pcapng to pqcap convert test (LIBPQCAP_SKIP_TSHARK=1)"
  exit 0
fi

command -v tshark >/dev/null 2>&1 || fail "tshark is required for convert/readability tests"
[[ -x "${CONVERT_CLI}" ]] || fail "${CONVERT_CLI} not built; run cmake --build build"

mkdir -p "${OUT_DIR}"
# shellcheck source=lib/tshark_embed_check.sh
source "${ROOT}/tests/lib/tshark_embed_check.sh"

convert_libpqcap_case() {
  local plain="$1"
  local metadata="$2"
  local converted="$3"
  local app_filter="$4"
  local label="$5"
  local extract_out="$6"

  convert_pcapng_to_pqcap "${plain}" "${metadata}" "${converted}"
  validate_converted_readability "${plain}" "${converted}" "${app_filter}" "${label}"

  python3 "${ROOT}/tests/lib/pqcap_file_checks.py" assert-parquet \
    "${converted}" "${metadata}" "${extract_out}"
  echo "PASS ${label} convert parquet extract round-trip"

  if "${CONVERT_CLI}" "${converted}" "${metadata}" "${converted}.double" 2>/dev/null; then
    fail "${label}: double convert should fail on indexed capture"
  fi
  echo "PASS ${label} convert rejects already-indexed capture"
}

convert_libpqcap_case "${FIXTURES}/udp1.pcapng" \
  "${FIXTURES}/minimal_metadata.parquet" \
  "${OUT_DIR}/udp1.pqcapng" \
  "udp" \
  "udp1" \
  "${OUT_DIR}/udp1.extracted.parquet"

convert_libpqcap_case "${FIXTURES}/http.pcapng" \
  "${FIXTURES}/http_metadata.parquet" \
  "${OUT_DIR}/http.pqcapng" \
  "http" \
  "http" \
  "${OUT_DIR}/http.extracted.parquet"

HTTP_GET_PLAIN="$(tshark_count "${FIXTURES}/http.pcapng" "http.request.method==GET")"
HTTP_GET_CONVERTED="$(tshark_count "${OUT_DIR}/http.pqcapng" "http.request.method==GET")"
[[ "${HTTP_GET_CONVERTED}" == "${HTTP_GET_PLAIN}" ]] || fail "HTTP GET count changed after libpqcap convert"
(( HTTP_GET_PLAIN >= 2 )) || fail "expected at least 2 HTTP GET requests"

HTTP_200_PLAIN="$(tshark_count "${FIXTURES}/http.pcapng" "http.response.code==200")"
HTTP_200_CONVERTED="$(tshark_count "${OUT_DIR}/http.pqcapng" "http.response.code==200")"
[[ "${HTTP_200_CONVERTED}" == "${HTTP_200_PLAIN}" ]] || fail "HTTP 200 count changed after libpqcap convert"

echo "PASS pcapng to pqcap convert (libpqcap + readability retained)"
