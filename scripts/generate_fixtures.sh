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
command -v tshark >/dev/null 2>&1 || {
  echo "tshark is required to regenerate http.pcapng and validate fixtures"
  exit 1
}

HTTP_CAP="${FIXTURES}/http.cap"
HTTP_PCAPNG="${FIXTURES}/http.pcapng"
if [[ ! -f "${HTTP_CAP}" ]]; then
  echo "Downloading Wireshark http.cap sample..."
  curl -fsSL -o "${HTTP_CAP}" \
    "https://wiki.wireshark.org/uploads/27707187aeb30df68e70c8fb9d614981/http.cap"
fi
tshark -r "${HTTP_CAP}" -F pcapng -w "${HTTP_PCAPNG}" >/dev/null 2>&1

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

def scan_pcapng(data: bytes):
    locs = []
    pos = 0
    while pos + 12 <= len(data):
        bt, tl = struct.unpack_from("<II", data, pos)
        if tl < 12 or pos + tl > len(data):
            break
        if bt == 0x00000006 and tl >= 32:
            size = struct.unpack_from("<I", data, pos + 20)[0]
            locs.append((pos + 28, size))
        elif bt == 0x00000003 and tl >= 16:
            size = struct.unpack_from("<I", data, pos + 8)[0]
            locs.append((pos + 12, size))
        pos += tl
    return locs

def write_minimal_parquet(pcapng_path: pathlib.Path, parquet_out: str) -> None:
    locs = scan_pcapng(pcapng_path.read_bytes())
    if not locs:
        raise SystemExit(f"no packet blocks in {pcapng_path}")
    offset, size = locs[0]
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

write_minimal_parquet(pathlib.Path(sys.argv[1]), sys.argv[2])
PY

python3 - <<'PY' "${HTTP_PCAPNG}" "${FIXTURES}/http_metadata.parquet"
import struct, subprocess, sys, pathlib

def scan_pcapng(data: bytes):
    locs = []
    pos = 0
    while pos + 12 <= len(data):
        bt, tl = struct.unpack_from("<II", data, pos)
        if tl < 12 or pos + tl > len(data):
            break
        if bt == 0x00000006 and tl >= 32:
            size = struct.unpack_from("<I", data, pos + 20)[0]
            locs.append((pos + 28, size))
        elif bt == 0x00000003 and tl >= 16:
            size = struct.unpack_from("<I", data, pos + 8)[0]
            locs.append((pos + 12, size))
        pos += tl
    return locs

pcapng = pathlib.Path(sys.argv[1])
parquet_out = sys.argv[2]
locs = scan_pcapng(pcapng.read_bytes())
if not locs:
    raise SystemExit("no packets in http.pcapng")
values = ",\n".join(f"({off}, {size}, {1000 + i}, 1)" for i, (off, size) in enumerate(locs))
sql = f"""
CREATE TABLE http AS
SELECT * FROM (VALUES {values}) AS t("offset", "size", ts_ns, linktype);
COPY http TO '{parquet_out}' (FORMAT PARQUET);
"""
subprocess.check_call(["duckdb", "-unsigned", "-c", sql])
print(f"http metadata rows: {len(locs)}")
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

COUNT="$(tshark -r "${PCAPNG}" -Y udp -T fields -e frame.number | wc -l | tr -d ' ')"
[[ "${COUNT}" -gt 0 ]] || exit 1
HTTP_FRAMES="$(tshark -r "${HTTP_PCAPNG}" -T fields -e frame.number | wc -l | tr -d ' ')"
[[ "${HTTP_FRAMES}" -gt 0 ]] || exit 1

echo "Regenerated fixtures in ${FIXTURES}"
