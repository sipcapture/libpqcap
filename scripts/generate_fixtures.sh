#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
FIXTURES="${ROOT}/tests/fixtures"
TMP_DIR="$(mktemp -d)"
trap 'rm -rf "${TMP_DIR}"' EXIT
mkdir -p "${FIXTURES}"

command -v duckdb >/dev/null 2>&1 || {
  echo "duckdb is required to regenerate fixtures"
  exit 1
}
command -v text2pcap >/dev/null 2>&1 || {
  echo "text2pcap is required to regenerate PCAP-NG fixtures"
  exit 1
}

HEX_FILE="${TMP_DIR}/udp1.hex"
PCAPNG="${FIXTURES}/udp1.pcapng"

cat > "${HEX_FILE}" <<'EOF'
0000  ff ff ff ff ff ff 00 11 22 33 44 55 08 00 45 00
0010  00 1c 00 01 00 00 40 11 6a ce 7f 00 00 01 7f 00
0020  00 01 04 d2 16 2e 00 08 00 00
EOF

text2pcap "${HEX_FILE}" "${PCAPNG}" >/dev/null

python3 - <<'PY' "${PCAPNG}" "${FIXTURES}/minimal_metadata.parquet"
import struct, subprocess, sys, pathlib

pcapng = pathlib.Path(sys.argv[1]).read_bytes()
parquet_out = sys.argv[2]
pos = 0
offset = 0
size = 0
while pos + 12 <= len(pcapng):
    bt, tl = struct.unpack_from("<II", pcapng, pos)
    if tl < 12 or pos + tl > len(pcapng):
        break
    if bt == 0x00000006 and tl >= 32:
        size = struct.unpack_from("<I", pcapng, pos + 20)[0]
        offset = pos + 28
        break
    if bt == 0x00000003 and tl >= 16:
        size = struct.unpack_from("<I", pcapng, pos + 8)[0]
        offset = pos + 12
        break
    pos += tl

if size <= 0:
    raise SystemExit("no packet block found in generated pcapng")

sql = f"""
CREATE TABLE minimal AS
SELECT
  CAST({offset} AS UBIGINT) AS "offset",
  CAST({size} AS UBIGINT) AS "size",
  CAST(1000 AS UBIGINT) AS ts_ns,
  CAST(1 AS USMALLINT) AS linktype;
COPY minimal TO '{parquet_out}' (FORMAT PARQUET);
"""
subprocess.check_call(["duckdb", "-unsigned", "-c", sql])
print(f"minimal metadata: offset={offset} size={size}")
PY

duckdb -unsigned <<SQL
CREATE TABLE large AS
SELECT
  CAST(i * 42 AS UBIGINT) AS "offset",
  CAST(42 AS UBIGINT) AS "size",
  CAST(1716800000000000000 + i AS UBIGINT) AS ts_ns,
  CAST(1 AS USMALLINT) AS linktype
FROM range(2000) r(i);
COPY large TO '${FIXTURES}/large_metadata.parquet' (FORMAT PARQUET);
SQL

if command -v tshark >/dev/null 2>&1; then
  COUNT="$(tshark -r "${PCAPNG}" -Y udp -T fields -e frame.number | wc -l | tr -d ' ')"
  [[ "${COUNT}" -gt 0 ]] || {
    echo "generated pcapng is not readable by tshark"
    exit 1
  }
fi

echo "Regenerated fixtures in ${FIXTURES} (udp1.pcapng is tshark-valid PCAP-NG)"
