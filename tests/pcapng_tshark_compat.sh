#!/usr/bin/env bash
# Validate PCAP-NG wire compatibility: embedded pqcap files must remain readable by tshark.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="${LIBPQCAP_BUILD_DIR:-${ROOT}/build}"
FIXTURES="${ROOT}/tests/fixtures"
OUT_DIR="${LIBPQCAP_TEST_OUT:-${BUILD}/integration}"
EMBED_CLI="${BUILD}/pqcap_embed_cli"

fail() {
  echo "FAIL: $1"
  exit 1
}

if [[ "${LIBPQCAP_SKIP_TSHARK:-}" == "1" ]]; then
  echo "SKIP pcapng tshark compatibility (LIBPQCAP_SKIP_TSHARK=1)"
  exit 0
fi

command -v tshark >/dev/null 2>&1 || fail "tshark is required for PCAP-NG compatibility tests"
[[ -x "${EMBED_CLI}" ]] || fail "${EMBED_CLI} not built; run cmake --build build"
[[ -f "${FIXTURES}/minimal_metadata.parquet" ]] || fail "missing minimal_metadata.parquet"
[[ -f "${FIXTURES}/http_metadata.parquet" ]] || fail "missing http_metadata.parquet"

mkdir -p "${OUT_DIR}"
# shellcheck source=lib/tshark_embed_check.sh
source "${ROOT}/tests/lib/tshark_embed_check.sh"

validate_embed_tshark "${FIXTURES}/udp1.pcapng" \
  "${FIXTURES}/minimal_metadata.parquet" \
  "${OUT_DIR}/udp1.pqcapng" \
  "udp" \
  "udp1"

validate_embed_tshark "${FIXTURES}/http.pcapng" \
  "${FIXTURES}/http_metadata.parquet" \
  "${OUT_DIR}/http.pqcapng" \
  "http" \
  "http"

# Wireshark sample capture: simple HTTP request/response exchange.
HTTP_GET_PLAIN="$(tshark_count "${FIXTURES}/http.pcapng" "http.request.method==GET")"
HTTP_GET_EMBED="$(tshark_count "${OUT_DIR}/http.pqcapng" "http.request.method==GET")"
[[ "${HTTP_GET_EMBED}" == "${HTTP_GET_PLAIN}" ]] || fail "HTTP GET count changed after embed"
(( HTTP_GET_PLAIN >= 2 )) || fail "expected at least 2 HTTP GET requests in http.pcapng"

HTTP_200_PLAIN="$(tshark_count "${FIXTURES}/http.pcapng" "http.response.code==200")"
HTTP_200_EMBED="$(tshark_count "${OUT_DIR}/http.pqcapng" "http.response.code==200")"
[[ "${HTTP_200_EMBED}" == "${HTTP_200_PLAIN}" ]] || fail "HTTP 200 response count changed after embed"
(( HTTP_200_PLAIN >= 2 )) || fail "expected at least 2 HTTP 200 responses in http.pcapng"

echo "PASS pcapng tshark compatibility (udp1 + wireshark http sample)"
