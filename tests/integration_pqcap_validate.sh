#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="${LIBPQCAP_BUILD_DIR:-${ROOT}/build}"
FIXTURES="${ROOT}/tests/fixtures"
OUT_DIR="${LIBPQCAP_TEST_OUT:-${BUILD}/integration}"
CONVERT_CLI="${BUILD}/pqcap_convert_cli"
PLAIN="${FIXTURES}/udp1.pcapng"

mkdir -p "${OUT_DIR}"

if [[ ! -x "${CONVERT_CLI}" ]]; then
  echo "FAIL: ${CONVERT_CLI} not built; run cmake --build build"
  exit 1
fi

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

echo "PASS libpqcap integration (convert + tshark + byte-level parquet checks)"
