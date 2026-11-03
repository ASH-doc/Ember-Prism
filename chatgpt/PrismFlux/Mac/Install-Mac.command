#!/bin/bash
# Install the supplied Apple Silicon binaries for this user only.
set -euo pipefail
package_dir="$(cd "$(dirname "$0")" && pwd)"
app_directory="$HOME/Applications"
plugin_directory="$HOME/Library/Audio/Plug-Ins/VST3"
for source in 'Prism Flux.app' 'Prism Flux.vst3'; do
  [[ -d "$package_dir/$source" ]] || { echo "Missing $source beside this installer."; exit 1; }
done
if [[ -e "$app_directory/Prism Flux.app" || -e "$plugin_directory/Prism Flux.vst3" ]]; then
  read -r -p 'Replace the installed Prism Flux app/plugin? Type YES: ' reply
  [[ "$reply" == YES ]] || exit 0
fi
mkdir -p "$app_directory" "$plugin_directory"
/usr/bin/ditto "$package_dir/Prism Flux.app" "$app_directory/Prism Flux.app"
/usr/bin/ditto "$package_dir/Prism Flux.vst3" "$plugin_directory/Prism Flux.vst3"
printf 'Installed in ~/Applications and ~/Library/Audio/Plug-Ins/VST3.\nOpen Prism Flux and select your audio/MIDI device from Options. Rescan VST3 in your DAW.\n'
read -r -p 'Press Return to close. ' reply
