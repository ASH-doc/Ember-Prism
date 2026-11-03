#!/bin/bash
# Install the supplied Apple Silicon binaries for this user only.
set -euo pipefail
package_dir="$(cd "$(dirname "$0")" && pwd)"
app_directory="$HOME/Applications"
plugin_directory="$HOME/Library/Audio/Plug-Ins/VST3"
for source in 'Dual Synth.app' 'Dual Synth.vst3'; do
  [[ -d "$package_dir/$source" ]] || { echo "Missing $source beside this installer."; exit 1; }
done
if [[ -e "$app_directory/Dual Synth.app" || -e "$plugin_directory/Dual Synth.vst3" ]]; then
  read -r -p 'Replace the installed Dual Synth app/plugin? Type YES: ' reply
  [[ "$reply" == YES ]] || exit 0
fi
mkdir -p "$app_directory" "$plugin_directory"
/usr/bin/ditto "$package_dir/Dual Synth.app" "$app_directory/Dual Synth.app"
/usr/bin/ditto "$package_dir/Dual Synth.vst3" "$plugin_directory/Dual Synth.vst3"
printf 'Installed in ~/Applications and ~/Library/Audio/Plug-Ins/VST3.\nOpen Dual Synth and select your audio/MIDI device from Options. Rescan VST3 in your DAW.\n'
read -r -p 'Press Return to close. ' reply
