#!/usr/bin/env bash
# Validate PCAP-NG wire compatibility: embedded pqcap files must remain readable by tshark.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="${LIBPQCAP_BUILD_DIR:-${ROOT}/build}"
FIXTURES="${ROOT}/tests/fixtures"
OUT_DIR="${LIBPQCAP_TEST_OUT:-${BUILD}/integration}"
EMBED_CLI="${BUILD}/pqcap_embed_cli"
PLAIN="${FIXTURES}/udp1.pcapng"
EMBEDDED="${OUT_DIR}/udp1.pqcapng"

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
[[ -f "${PLAIN}" ]] || fail "missing fixture ${PLAIN}"
[[ -f "${FIXTURES}/minimal_metadata.parquet" ]] || fail "missing minimal_metadata.parquet"

mkdir -p "${OUT_DIR}"

tshark_count() {
  local file="$1"
  local filter="${2:-}"
  if [[ -n "${filter}" ]]; then
    tshark -r "${file}" -Y "${filter}" -T fields -e frame.number 2>/dev/null | wc -l | tr -d ' '
  else
    tshark -r "${file}" -T fields -e frame.number 2>/dev/null | wc -l | tr -d ' '
  fi
}

tshark_must_read() {
  local file="$1"
  tshark -r "${file}" -T fields -e frame.number >/dev/null 2>&1 || fail "tshark cannot read ${file}"
}

tshark_must_read "${PLAIN}"

PLAIN_UDP="$(tshark_count "${PLAIN}" "udp")"
[[ "${PLAIN_UDP}" =~ ^[0-9]+$ ]] || fail "invalid UDP packet count on plain capture"
(( PLAIN_UDP > 0 )) || fail "plain capture has no UDP packets decodable by tshark"

PROTO="$(tshark -r "${PLAIN}" -c 1 -T fields -e _ws.col.Protocol | head -n 1 | tr -d '\r')"
[[ -n "${PROTO}" ]] || fail "tshark returned empty protocol on plain capture"

"${EMBED_CLI}" "${PLAIN}" "${FIXTURES}/minimal_metadata.parquet" "${EMBEDDED}"

tshark_must_read "${EMBEDDED}"

EMBED_UDP="$(tshark_count "${EMBEDDED}" "udp")"
[[ "${EMBED_UDP}" == "${PLAIN_UDP}" ]] || {
  echo "FAIL: UDP packet count changed after embed (plain=${PLAIN_UDP}, embedded=${EMBED_UDP})"
  exit 1
}

EMBED_PROTO="$(tshark -r "${EMBEDDED}" -Y "udp" -c 1 -T fields -e _ws.col.Protocol | head -n 1 | tr -d '\r')"
[[ "${EMBED_PROTO}" == "${PROTO}" ]] || fail "protocol changed after embed (${PROTO} -> ${EMBED_PROTO})"

# Capture prefix must be unchanged (packet bytes untouched by libpqcap embed).
python3 - <<'PY' "${PLAIN}" "${EMBEDDED}"
import pathlib, sys
plain = pathlib.Path(sys.argv[1]).read_bytes()
embedded = pathlib.Path(sys.argv[2]).read_bytes()
if embedded[: len(plain)] != plain:
    raise SystemExit("capture prefix bytes changed after embed")
PY

if command -v capinfos >/dev/null 2>&1; then
  capinfos "${EMBEDDED}" >/dev/null 2>&1 || fail "capinfos rejected embedded file"
fi

echo "PASS pcapng tshark compatibility (udp packets=${PLAIN_UDP}, protocol=${PROTO}, prefix preserved)"
