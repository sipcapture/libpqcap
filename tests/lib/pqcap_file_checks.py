#!/usr/bin/env python3
"""File-level pqcap format checks used by shell integration tests."""
from __future__ import annotations

import struct
import sys
from pathlib import Path

FOOTER_BLOCK_TYPE = 0x00000BEE
FOOTER_BLOCK_SIZE = 44
FOOTER_MAGIC = b"PQCAPFTR"


def has_pqcap_footer(path: Path) -> bool:
    data = path.read_bytes()
    if len(data) < FOOTER_BLOCK_SIZE:
        return False
    footer = data[-FOOTER_BLOCK_SIZE:]
    block_type, total_len = struct.unpack_from("<II", footer, 0)
    if block_type != FOOTER_BLOCK_TYPE or total_len != FOOTER_BLOCK_SIZE:
        return False
    magic = footer[12:20]
    return magic == FOOTER_MAGIC


def assert_plain_capture(path: Path) -> None:
    if has_pqcap_footer(path):
        raise SystemExit(f"{path} is already a pqcap-indexed capture")


def assert_pqcap_indexed(path: Path) -> None:
    if not has_pqcap_footer(path):
        raise SystemExit(f"{path} is not a pqcap-indexed capture")


def assert_prefix_unchanged(plain: Path, converted: Path) -> None:
    plain_bytes = plain.read_bytes()
    converted_bytes = converted.read_bytes()
    if converted_bytes[: len(plain_bytes)] != plain_bytes:
        raise SystemExit("capture prefix bytes changed after convert")


def assert_parquet_extract(path: Path, expected_parquet: Path, out_parquet: Path) -> None:
    data = path.read_bytes()
    footer = data[-FOOTER_BLOCK_SIZE:]
    _, magic, _, _, off, length = struct.unpack_from("<I8sHHQQ", footer, 8)
    if magic != FOOTER_MAGIC:
        raise SystemExit("missing PQCAPFTR footer")
    blob = data[off : off + length]
    ref = expected_parquet.read_bytes()
    if blob != ref:
        raise SystemExit("embedded parquet bytes do not match metadata fixture")
    out_parquet.write_bytes(blob)


def main() -> int:
    if len(sys.argv) < 3:
        print("usage: pqcap_file_checks.py <command> <path> [args...]", file=sys.stderr)
        return 2

    cmd = sys.argv[1]
    if cmd == "assert-plain":
        assert_plain_capture(Path(sys.argv[2]))
        return 0
    if cmd == "assert-indexed":
        assert_pqcap_indexed(Path(sys.argv[2]))
        return 0
    if cmd == "assert-prefix":
        assert_prefix_unchanged(Path(sys.argv[2]), Path(sys.argv[3]))
        return 0
    if cmd == "assert-parquet":
        assert_parquet_extract(Path(sys.argv[2]), Path(sys.argv[3]), Path(sys.argv[4]))
        return 0

    print(f"unknown command: {cmd}", file=sys.stderr)
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
