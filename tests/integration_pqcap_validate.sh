#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="${LIBPQCAP_BUILD_DIR:-${ROOT}/build}"
FIXTURES="${ROOT}/tests/fixtures"
OUT_DIR="${LIBPQCAP_TEST_OUT:-${BUILD}/integration}"
EMBED_CLI="${BUILD}/pqcap_embed_cli"
CONVERT_CLI="${BUILD}/pqcap_convert_cli"
PQCAP_BIN="${PQCAP_BIN:-}"
PLAIN="${FIXTURES}/udp1.pcapng"

mkdir -p "${OUT_DIR}"

if [[ ! -x "${EMBED_CLI}" ]]; then
  echo "FAIL: ${EMBED_CLI} not built; run cmake --build build"
  exit 1
fi

# PCAP-NG / tshark compatibility is required for format success.
bash "${ROOT}/tests/pcapng_to_pqcap_convert.sh"

MIN_PQCAP="${OUT_DIR}/minimal.pqcapng"
LARGE_PQCAP="${OUT_DIR}/large.pqcapng"

"${CONVERT_CLI}" "${PLAIN}" "${FIXTURES}/minimal_metadata.parquet" "${MIN_PQCAP}"
"${CONVERT_CLI}" "${PLAIN}" "${FIXTURES}/large_metadata.parquet" "${LARGE_PQCAP}"

python3 - <<'PY' "${MIN_PQCAP}" "${FIXTURES}/minimal_metadata.parquet"
import pathlib, struct, sys
path = pathlib.Path(sys.argv[1])
ref = pathlib.Path(sys.argv[2]).read_bytes()
footer = path.read_bytes()[-44:]
enterprise, magic, version, reserved, off, length = struct.unpack_from("<I8sHHQQ", footer, 8)
assert magic == b"PQCAPFTR"
blob = path.read_bytes()[off:off+length]
assert blob == ref, "embedded parquet slice mismatch"
PY

python3 - <<'PY' "${LARGE_PQCAP}" "${FIXTURES}/large_metadata.parquet"
import pathlib, struct, sys
path = pathlib.Path(sys.argv[1])
ref = pathlib.Path(sys.argv[2]).read_bytes()
footer = path.read_bytes()[-44:]
_, magic, _, _, off, length = struct.unpack_from("<I8sHHQQ", footer, 8)
assert magic == b"PQCAPFTR"
assert length == len(ref)
blob = path.read_bytes()[off:off+length]
assert blob == ref, "large embedded parquet slice mismatch"
assert len(ref) > 10000, "expected non-trivial parquet payload"
PY

if [[ -z "${PQCAP_BIN}" ]]; then
  if [[ -x "${ROOT}/../dist/pqcap" ]]; then
    PQCAP_BIN="${ROOT}/../dist/pqcap"
  fi
fi

if [[ -z "${PQCAP_BIN}" || ! -x "${PQCAP_BIN}" ]]; then
  echo "SKIP pqcap binary validation (set PQCAP_BIN to dist/pqcap)"
  echo "PASS libpqcap integration (tshark + byte-level parquet checks)"
  exit 0
fi

MIN_ROWS="$("${PQCAP_BIN}" query -c "SELECT COUNT(*)::BIGINT AS n FROM read_pqcap('${MIN_PQCAP}');" | grep -E '^[0-9]+$' | tail -n 1)"
[[ "${MIN_ROWS}" == "1" ]] || {
  echo "FAIL: read_pqcap expected 1 row for minimal fixture, got '${MIN_ROWS}'"
  exit 1
}

LARGE_ROWS="$("${PQCAP_BIN}" query -c "SELECT COUNT(*)::BIGINT AS n FROM read_pqcap('${LARGE_PQCAP}');" | grep -E '^[0-9]+$' | tail -n 1)"
[[ "${LARGE_ROWS}" == "2000" ]] || {
  echo "FAIL: read_pqcap expected 2000 rows for large fixture, got '${LARGE_ROWS}'"
  exit 1
}

PACKETS="$("${PQCAP_BIN}" query -c "SELECT COUNT(*)::BIGINT AS n FROM read_pqcap_packets('${MIN_PQCAP}');" | grep -E '^[0-9]+$' | tail -n 1)"
[[ "${PACKETS}" == "1" ]] || {
  echo "FAIL: read_pqcap_packets expected 1 packet, got '${PACKETS}'"
  exit 1
}

HTTP_PQCAP="${OUT_DIR}/http.pqcapng"
"${CONVERT_CLI}" "${FIXTURES}/http.pcapng" "${FIXTURES}/http_metadata.parquet" "${HTTP_PQCAP}"

HTTP_META="$("${PQCAP_BIN}" query -c "SELECT COUNT(*)::BIGINT AS n FROM read_pqcap('${HTTP_PQCAP}');" | grep -E '^[0-9]+$' | tail -n 1)"
[[ "${HTTP_META}" == "43" ]] || {
  echo "FAIL: read_pqcap expected 43 rows for http fixture, got '${HTTP_META}'"
  exit 1
}

HTTP_PACKETS="$("${PQCAP_BIN}" query -c "SELECT COUNT(*)::BIGINT AS n FROM read_pqcap_packets('${HTTP_PQCAP}');" | grep -E '^[0-9]+$' | tail -n 1)"
[[ "${HTTP_PACKETS}" == "43" ]] || {
  echo "FAIL: read_pqcap_packets expected 43 packets for http fixture, got '${HTTP_PACKETS}'"
  exit 1
}

echo "PASS libpqcap integration (tshark + byte checks + pqcap read_pqcap/read_pqcap_packets)"
