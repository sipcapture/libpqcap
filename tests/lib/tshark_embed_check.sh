#!/usr/bin/env bash
# Shared helpers for validating libpqcap embed output with tshark.
validate_embed_tshark() {
  local plain="$1"
  local metadata="$2"
  local embedded="$3"
  local app_filter="$4"
  local label="$5"

  [[ -f "${plain}" ]] || {
    echo "FAIL: missing plain capture ${plain}"
    return 1
  }
  [[ -f "${metadata}" ]] || {
    echo "FAIL: missing metadata ${metadata}"
    return 1
  }

  tshark_must_read "${plain}"

  local plain_app embed_app
  plain_app="$(tshark_count "${plain}" "${app_filter}")"
  [[ "${plain_app}" =~ ^[0-9]+$ ]] || {
    echo "FAIL: invalid ${label} count on plain capture"
    return 1
  }
  (( plain_app > 0 )) || {
    echo "FAIL: plain ${label} capture has no decodable ${app_filter} packets"
    return 1
  }

  "${EMBED_CLI}" "${plain}" "${metadata}" "${embedded}"

  tshark_must_read "${embedded}"

  embed_app="$(tshark_count "${embedded}" "${app_filter}")"
  [[ "${embed_app}" == "${plain_app}" ]] || {
    echo "FAIL: ${label} ${app_filter} packet count changed after embed (plain=${plain_app}, embedded=${embed_app})"
    return 1
  }

  python3 - <<'PY' "${plain}" "${embedded}"
import pathlib, sys
plain = pathlib.Path(sys.argv[1]).read_bytes()
embedded = pathlib.Path(sys.argv[2]).read_bytes()
if embedded[: len(plain)] != plain:
    raise SystemExit("capture prefix bytes changed after embed")
PY

  if command -v capinfos >/dev/null 2>&1; then
    capinfos "${embedded}" >/dev/null 2>&1 || {
      echo "FAIL: capinfos rejected embedded ${label} file"
      return 1
    }
  fi

  echo "PASS ${label} tshark (${app_filter} packets=${plain_app}, prefix preserved)"
}

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
  tshark -r "${file}" -T fields -e frame.number >/dev/null 2>&1 || {
    echo "FAIL: tshark cannot read ${file}"
    return 1
  }
}
