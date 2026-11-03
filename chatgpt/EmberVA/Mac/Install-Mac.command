#!/bin/bash
set -euo pipefail
SOURCE_DIR="$(cd "$(dirname "$0")" && pwd)"
APP_DIR="$HOME/Applications"
VST_DIR="$HOME/Library/Audio/Plug-Ins/VST3"
if [[ -e "$APP_DIR/Ember VA.app" || -e "$VST_DIR/Ember VA.vst3" ]]; then
  read -r -p "Replace the existing Ember VA installation? [y/N] " answer
  [[ "$answer" == "y" || "$answer" == "Y" ]] || exit 0
fi
mkdir -p "$APP_DIR" "$VST_DIR"
ditto "$SOURCE_DIR/Ember VA.app" "$APP_DIR/Ember VA.app"
ditto "$SOURCE_DIR/Ember VA.vst3" "$VST_DIR/Ember VA.vst3"
echo "Installed the standalone app in $APP_DIR"
echo "Installed the plugin in $VST_DIR"
echo "Rescan plugins in your VST3 host."
read -r -p "Press Return to close." answer
