#!/usr/bin/env bash
exec "$(cd "$(dirname "$0")/.." && pwd)/tests/pcapng_to_pqcap_convert.sh" "$@"
