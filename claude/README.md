# AETHER DUO — two synths, one instrument (VST3 + Windows app)

A native C++ / [JUCE](https://juce.com) instrument. Two completely separate synthesizers live inside it, side by side:

| | **EMBER** (Synth A) | **PRISM** (Synth B) |
|---|---|---|
| Type | Subtractive virtual analog | Wavetable |
| Character | Warm, dark, gritty, **Moog-ladder warmth** | **Crystal clear**, airy, modern, digital |
| Look | Vintage hardware: walnut wood cheeks, brushed-bronze face, champagne metal knobs, amber LCD, analog needle meter | Crystal software: faceted dark-glass backdrop, chamfered panels, prism-gradient rings, glowing 3D wavetable view |
| Core | 3 oscillators + sub + noise, hypersaw/unison, analog drift, 4 filter models (Moog ladder 24 dB, multimode 12, screamer, state-variable), 2 filters (serial/parallel/split), trance gate, arp, chord stab | 2 wavetable oscillators (30 original tables, 10 warp modes each), 16-voice unison with wavetable spread, 8 filter types, drag-and-drop modulation, drawable LFOs, 16-step sequencer, arp, “Surprise me” randomizer |
| Own effects | drive, chorus/flanger, phaser, delay, dark plate reverb, tone EQ, tempo pump | 8-slot rack (any order): chorus, phaser, delay, reverb, distortion, EQ, compressor, filter, widener, flanger |

The two synths **share nothing**: no code, no presets, no effects. The only things they have in common are the MIDI input and the host's mixer + master section.

## Run it

Everything is in the **`Release`** folder:

* **Standalone app:** double-click `Release\AETHER DUO.exe`.
  Click **Options** (top-left of the window) to pick your audio device (an ASIO driver gives the lowest latency) and your MIDI keyboard.
* **VST3 plugin:** right-click `Release\Install-VST3.ps1` → *Run with PowerShell* (accepts the admin prompt), then rescan plugins in your DAW. It appears as an **instrument** called *AETHER DUO*.
* **Rebuild from source:** `.\build.ps1` (needs Visual Studio Build Tools 2022 and CMake; JUCE is bundled, no downloads). Tests: `.\build.ps1 -Target DuoSelfTest`, then run `build\DuoSelfTest_artefacts\Release\DuoSelfTest.exe`.

## How to use it

**Top bar (grey, always the same)**

* **EMBER | PRISM** – switches which synth you *see*. Both keep playing; nothing changes in the sound.
* **LAYER | SPLIT | CHANNEL** – where your keys go. LAYER = both synths. SPLIT = EMBER below the split note, PRISM above (the **A<B** button swaps them). CHANNEL = MIDI channel 1 → EMBER, 2 → PRISM by default (set 0 = “omni”).
  Changing the mode releases every held note, so nothing can get stuck.
* **BPM / TAP / DAW** – tempo for arps, gates, delays and LFOs. With **DAW** ticked the plugin follows your DAW; in the standalone app the BPM field is used.
* **PANIC** – instant silence on both synths (also clears the reverb tail).
* **SAVE / LOAD** – whole sessions (both synths + mixer + routing + master) as a `.json` file.
* **MASTER** – opens the drawer with the glue compressor, master reverb, safety limiter and master volume.

**Bottom bar:** one strip per synth (volume, pan, **M**ute, **S**olo, level meter) and a playable keyboard. In the standalone app you can also play with the computer keys (A W S E D F T G Y H U J K …). Muting a synth releases its notes; solo works as expected.

**EMBER** – everything is on one face. Turn knobs by dragging up/down (hold **Shift** for fine, **double-click** to reset). The big amber display is the preset browser (click it for the list, ◀ ▶ to step). **AGE** adds analog drift and tolerance, **WARMTH** adds even-harmonic saturation (the “Moog” glow), **FLAVOR** changes the oscillator’s shape/saturation. The **trance gate** LEDs can be drawn by dragging across them. For a chord stab: hold 2+ keys, press **LEARN**, switch on **CHORD STAB**, then play single keys.

**PRISM** – drag a coloured chip (ENV, LFO, macro, velocity…) from the chip bar **onto any knob or the Position / Warp sliders**; a coloured ring shows the range and the white dot moves with the modulation. Right-click a knob to remove an assignment; amounts live in the **MATRIX** tab. The **LFO** tab lets you draw a shape (set the LFO’s shape to *Drawn*). In the FX rack every slot can hold any effect; use the **< >** buttons to reorder. **SURPRISE** makes a new sound inside musical limits; lock sections to keep them.

## What to listen for

See [`LISTENING_CHECKLIST.md`](LISTENING_CHECKLIST.md). In short: EMBER should feel heavy, rounded and slightly alive (nothing perfectly still); PRISM should feel clean, wide and bright without harshness, with smooth sweeps when you scan the wavetable position.

## Files

```
Release/                   ready-to-use app + VST3 + installer + demo WAVs
Source/Host/               mixer, MIDI routing, glue compressor, master reverb, limiter (no UI inside)
Source/SynthA/             EMBER engine, effects, presets, panel   (prefix sa_ / namespace sa, emberui)
Source/SynthB/             PRISM engine, wavetables, effects, presets, panel   (prefix sb_ / namespace sb, prismui)
Source/ui/                 the grey host window
Source/PluginProcessor.*   parameters, sessions, state
Tests/                     SelfTest (DSP + host), PluginTest (sessions, panels, real VST3 load), UiShot (renders the UI to PNG)
screenshots/               what the windows look like
INTEGRATION_REPORT.md      what was built, what was cut, test results, assumptions
```

Names: whole instrument **AETHER DUO**, Synth A **EMBER**, Synth B **PRISM** (chosen by me because none were given; they are easy to change in `Source`).
