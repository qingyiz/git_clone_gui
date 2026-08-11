#!/usr/bin/env bash

set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "用法：$0 <GitCloneGui.app> <输出.dmg>" >&2
  exit 2
fi

app_path="$1"
output_path="$2"
executable="$app_path/Contents/MacOS/GitCloneGui"
cocoa_plugin="$app_path/Contents/PlugIns/platforms/libqcocoa.dylib"

for required_path in \
  "$executable" \
  "$app_path/Contents/Frameworks/QtCore.framework" \
  "$app_path/Contents/Frameworks/QtGui.framework" \
  "$app_path/Contents/Frameworks/QtWidgets.framework" \
  "$cocoa_plugin" \
  "$app_path/Contents/Resources/GitCloneGui.icns"; do
  if [[ ! -e "$required_path" ]]; then
    echo "macOS 安装树缺少：$required_path" >&2
    exit 1
  fi
done

external_dependencies="$(otool -L "$executable" \
  | awk 'NR > 1 { print $1 }' \
  | grep -Ev '^(@|/System/Library/|/usr/lib/)' || true)"
if [[ -n "$external_dependencies" ]]; then
  echo "macOS 主程序仍引用应用外部的非系统绝对路径：" >&2
  echo "$external_dependencies" >&2
  exit 1
fi

codesign --verify --deep --strict --verbose=2 "$app_path"

stage_dir="$(mktemp -d "${RUNNER_TEMP:-/tmp}/git-clone-gui-dmg.XXXXXX")"
trap 'rm -rf "$stage_dir"' EXIT

ditto "$app_path" "$stage_dir/GitCloneGui.app"
ln -s /Applications "$stage_dir/Applications"
mkdir -p "$(dirname "$output_path")"
rm -f "$output_path"
hdiutil create \
  -volname "GitCloneGui" \
  -srcfolder "$stage_dir" \
  -format UDZO \
  -ov \
  "$output_path"

if [[ "${MACOS_SIGNING_ENABLED:-false}" == "true" ]]; then
  codesign \
    --force \
    --timestamp \
    --sign "$GIT_CLONE_GUI_MACOS_SIGNING_IDENTITY" \
    "$output_path"
  codesign --verify --strict --verbose=2 "$output_path"
fi

notary_values=0
[[ -n "${APPLE_ID:-}" ]] && ((notary_values += 1))
[[ -n "${APPLE_APP_PASSWORD:-}" ]] && ((notary_values += 1))
[[ -n "${APPLE_TEAM_ID:-}" ]] && ((notary_values += 1))

if [[ "${MACOS_SIGNING_ENABLED:-false}" == "true" && "$notary_values" -eq 3 ]]; then
  xcrun notarytool submit "$output_path" \
    --apple-id "$APPLE_ID" \
    --password "$APPLE_APP_PASSWORD" \
    --team-id "$APPLE_TEAM_ID" \
    --wait
  xcrun stapler staple "$output_path"
  xcrun stapler validate "$output_path"
  spctl --assess --type open --context context:primary-signature --verbose=2 "$output_path"
  if [[ -n "${GITHUB_STEP_SUMMARY:-}" ]]; then
    echo "- macOS：Developer ID 签名、公证与 stapling 验证通过。" >> "$GITHUB_STEP_SUMMARY"
  fi
elif [[ -n "${GITHUB_STEP_SUMMARY:-}" ]]; then
  if [[ "${MACOS_SIGNING_ENABLED:-false}" == "true" ]]; then
    echo "- macOS：应用和 DMG 已签名，但公证 Secrets 不完整，本次未公证。" >> "$GITHUB_STEP_SUMMARY"
  else
    echo "- macOS：DMG 为 unsigned 测试包，未执行公证。" >> "$GITHUB_STEP_SUMMARY"
  fi
fi

echo "macOS 发布包：$output_path"
