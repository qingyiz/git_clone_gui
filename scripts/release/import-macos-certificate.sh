#!/usr/bin/env bash

set -euo pipefail

if [[ -z "${GITHUB_ENV:-}" || -z "${RUNNER_TEMP:-}" ]]; then
  echo "此脚本只能在 GitHub Actions runner 中执行。" >&2
  exit 2
fi

certificate="${MACOS_CERTIFICATE:-}"
password="${MACOS_CERTIFICATE_PASSWORD:-}"

if [[ -z "$certificate" || -z "$password" ]]; then
  {
    echo "MACOS_SIGNING_ENABLED=false"
    echo "GIT_CLONE_GUI_MACOS_SIGNING_IDENTITY="
  } >> "$GITHUB_ENV"
  if [[ -n "${GITHUB_STEP_SUMMARY:-}" ]]; then
    echo "- macOS：未配置完整 Developer ID P12 Secrets，本次生成 unsigned 测试包。" >> "$GITHUB_STEP_SUMMARY"
  fi
  exit 0
fi

certificate_path="$RUNNER_TEMP/git-clone-gui-developer-id.p12"
keychain_path="$RUNNER_TEMP/git-clone-gui-signing.keychain-db"
keychain_password="$(uuidgen)"

printf '%s' "$certificate" | base64 -D > "$certificate_path"
security create-keychain -p "$keychain_password" "$keychain_path"
security set-keychain-settings -lut 21600 "$keychain_path"
security unlock-keychain -p "$keychain_password" "$keychain_path"
security import "$certificate_path" \
  -k "$keychain_path" \
  -P "$password" \
  -A \
  -t cert \
  -f pkcs12
security set-key-partition-list \
  -S apple-tool:,apple:,codesign: \
  -s \
  -k "$keychain_password" \
  "$keychain_path" >/dev/null
security list-keychains -d user -s "$keychain_path" login.keychain-db
rm -f "$certificate_path"

identity="$(security find-identity -v -p codesigning "$keychain_path" \
  | sed -n 's/.*"\(Developer ID Application:.*\)"/\1/p' \
  | head -n 1)"

if [[ -z "$identity" ]]; then
  echo "P12 中没有 Developer ID Application 身份，不能用于站外分发和公证。" >&2
  exit 1
fi

{
  echo "MACOS_SIGNING_ENABLED=true"
  echo "MACOS_KEYCHAIN_PATH=$keychain_path"
  echo "GIT_CLONE_GUI_MACOS_SIGNING_IDENTITY=$identity"
} >> "$GITHUB_ENV"

if [[ -n "${GITHUB_STEP_SUMMARY:-}" ]]; then
  echo "- macOS：已导入 Developer ID Application，后续执行 Hardened Runtime 签名。" >> "$GITHUB_STEP_SUMMARY"
fi
