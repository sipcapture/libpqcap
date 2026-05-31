#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
FIXTURES="${ROOT}/tests/fixtures"
mkdir -p "${FIXTURES}"

command -v duckdb >/dev/null 2>&1 || {
  echo "duckdb is required to regenerate fixtures"
  exit 1
}

duckdb -unsigned <<SQL
CREATE TABLE minimal AS
SELECT
  CAST(60 AS UBIGINT) AS "offset",
  CAST(4 AS UBIGINT) AS "size",
  CAST(1000 AS UBIGINT) AS ts_ns,
  CAST(1 AS USMALLINT) AS linktype;
COPY minimal TO '${FIXTURES}/minimal_metadata.parquet' (FORMAT PARQUET);

CREATE TABLE large AS
SELECT
  CAST(i * 42 AS UBIGINT) AS "offset",
  CAST(42 AS UBIGINT) AS "size",
  CAST(1716800000000000000 + i AS UBIGINT) AS ts_ns,
  CAST(1 AS USMALLINT) AS linktype
FROM range(2000) r(i);
COPY large TO '${FIXTURES}/large_metadata.parquet' (FORMAT PARQUET);
SQL

python3 - <<'PY' "${FIXTURES}/minimal_capture.pcapng"
import pathlib, sys
pcapng = bytes([
    0x0a, 0x0d, 0x0d, 0x0a, 0x20, 0x00, 0x00, 0x00, 0x4d, 0x3c, 0x2b, 0x1a, 0x01, 0x00,
    0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
    0x20, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0xde, 0xad, 0xbe, 0xef, 0x24, 0x00, 0x00, 0x00,
])
pathlib.Path(sys.argv[1]).write_bytes(pcapng)
PY

echo "Regenerated fixtures in ${FIXTURES}"
