"""Convert the pinned Claude Prism preset dump into the Dual Synth compatibility bank."""
import json
import math
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BANK = ROOT / "Engines/B/Presets"
SOURCE = "07e1d7d14dce7551b4732e98fe63ee281a22a604"
RATE_DIVISIONS = [6, 4, 2, 1, 0]
LFO_DIVISIONS = [0, 8, 7, 6, 4, 2, 3, 1]
MOD_SOURCES = [0, 4, 5, 6, 7, 1, 2, 3, *range(14, 22), 8, 9, 10, 11, 12, 24]
FX_TYPES = [0, 1, 3, 4, 5, 6, 7, 8, 9, 10, 2]

def read_source():
    defaults = {}
    presets = []
    for line in (BANK / "claude-prism-source.tsv").read_text().splitlines():
        kind, *fields = line.split("\t")
        if kind == "D":
            defaults[fields[0][3:]] = float(fields[1])
        elif kind == "P":
            presets.append({"index": int(fields[0]), "name": fields[1], "category": fields[2],
                            "values": {}, "shapes": {}})
        elif kind == "V":
            presets[-1]["values"][fields[0][3:]] = float(fields[1])
        elif kind == "L":
            presets[-1]["shapes"][int(fields[0])] = list(map(float, fields[1:]))
        else:
            raise ValueError(kind)
    return defaults, presets

def fx_values(kind, a, b, c):
    if kind == 1:
        return .1 + 2.5*a, b, .15
    if kind == 2:
        return .05 + 1.5*a, b, c*.85
    if kind == 3:
        return .05 + 3*a, b, c*.85
    if kind == 4:
        return .375, b*.9, 1200 + 16800*c
    if kind == 5:
        return a, 1-b, 0
    if kind == 6:
        return a*14/24, 200 + 19800*c, 3 + (1-b)*9
    if kind == 7:
        return tuple((v-.5)*24 for v in (a,b,c))
    if kind == 8:
        return -40 + 40*a, 1.5 + 8.5*b, .12
    if kind == 9:
        return 40*400**a, b, 0
    if kind == 10:
        return 2*a, b*30, 0
    raise ValueError(kind)

def convert(defaults, preset, schema):
    old = defaults | preset["values"]
    p = {}
    unmapped = {}
    used = set()
    def get(key):
        used.add(key)
        return old[key]
    def put(key, value):
        spec = schema[key]
        value = min(spec["max"], max(spec["min"], value))
        p[key] = round(value) if spec["integer"] else round(value, 8)
    def copy(prefix, source, pairs):
        for old_suffix, new_suffix in pairs.items():
            put(prefix+new_suffix, get(source+old_suffix))

    for osc in (1,2):
        prefix = f"osc{osc}."
        source = f"o{osc}_"
        put(prefix+"claude.table", get(source+"table")+1)
        copy(prefix, source, {"pos":"pos","oct":"octave","semi":"tune","fine":"fine",
                             "lvl":"level","pan":"pan","phase":"phase","rand":"random",
                             "w1a":"warp1.amount","w2a":"warp2.amount"})
        for lane in (1,2):
            # Claude combines AM/ring; Prism keeps AM and Ring as separate choices.
            warp = int(get(source+f"w{lane}"))
            put(prefix+f"warp{lane}.type", warp + (warp >= 8))
        copy(prefix+"uni.", "uni_", {"voices":"voices","detune":"detune","curve":"curve",
                                      "spread":"spread","blend":"blend","wtspread":"wtspread"})
    copy("mix.", "", {"sub_lvl":"sub","sub_wave":"sub.wave","sub_oct":"sub.octave",
                      "noise_lvl":"noise","noise_col":"noise.color"})
    for filt in (1,2):
        prefix = f"flt{filt}."
        put(prefix+"on", 1)
        copy(prefix, f"f{filt}_", {"type":"type","cut":"cutoff","res":"res","drive":"drive",
                                   "mix":"mix","key":"key","env":"env","vel":"velocity"})
    put("filter.route", get("f_route"))
    for env in (1,2,3):
        copy(f"env{env}.", f"e{env}_", {"a":"attack","d":"decay","s":"sustain","r":"release"})
    for lfo in range(1,5):
        prefix = f"lfo{lfo}."
        shape = int(get(f"l{lfo}_shape"))
        sync = int(get(f"l{lfo}_sync"))
        put(prefix+"rate", get(f"l{lfo}_rate"))
        put(prefix+"sync", bool(sync))
        if sync:
            put(prefix+"division", LFO_DIVISIONS[sync])
        # Sample and hold routes use the engine's available S&H source below.
        points = preset["shapes"].get(lfo-1) if shape == 5 else None
        shapes = (lambda x: math.sin(2*math.pi*x), lambda x: 4*abs(x-.5)-1,
                  lambda x: 2*x-1, lambda x: 1 if x < .5 else -1)
        for i in range(16):
            put(prefix+f"point{i+1}", points[i] if points else shapes[min(shape,3)](i/16))
        put(prefix+"interpolation", 2 if shape in (3,4) else 1)
    for i in range(1,9):
        put(f"macro{i}.value", get(f"mac{i}"))
    put("macro.euphoria", 0)
    put("macro.space", 0)
    put("voice.mode", get("play_mode"))
    put("voice.glide", get("glide"))
    put("voice.bend", get("bend_range"))
    put("master.gain", 20*math.log10(max(get("master"), .001)))
    copy("arp.", "arp_", {"on":"on","mode":"mode","oct":"octaves","gate":"length",
                         "swing":"swing","hold":"hold"})
    put("arp.rate", RATE_DIVISIONS[int(get("arp_rate"))])
    for step in range(1,17):
        put(f"arp.step{step}.gate", 1)
        put(f"arp.step{step}.velocity", 1)
    put("seq.on", get("seq_on"))
    put("seq.rate", [4,2,0][int(get("seq_rate"))])
    put("seq.length", .7)
    sequence_length = int(get("seq_len"))
    for step in range(1,17):
        original = 1+(step-1)%sequence_length
        for key, suffix in (("sp","pitch"),("sg","gate"),("sv","velocity")):
            put(f"seq.step{step}.{suffix}", get(f"{key}{original}"))
    # Duplicate destinations cover Claude's shared controls using Prism's independent oscillators.
    destinations = [[], ["osc1.tune","osc2.tune"], ["osc1.tune"], ["osc2.tune"],
                    ["osc1.pos"], ["osc2.pos"], ["osc1.warp1.amount"], ["osc1.warp2.amount"],
                    ["osc2.warp1.amount"], ["osc2.warp2.amount"], ["osc1.level"], ["osc2.level"],
                    ["mix.sub"], ["mix.noise"], ["flt1.cutoff"], ["flt1.res"], ["flt2.cutoff"],
                    ["flt2.res"], ["osc1.uni.detune","osc2.uni.detune"],
                    ["osc1.uni.wtspread","osc2.uni.wtspread"], ["master.gain"],
                    ["osc1.pan","osc2.pan"], ["lfo1.rate"], ["lfo2.rate"]]
    routes = []
    for slot in range(1,11):
        source, dest = (int(get(f"m{slot}_{key}")) for key in ("src","dst"))
        amount = get(f"m{slot}_amt")
        if not source or not dest:
            continue
        mapped_source = MOD_SOURCES[source]
        if 1 <= source <= 4 and int(old[f"l{source}_shape"]) == 4:
            mapped_source = 13
        for target in destinations[dest]:
            routes.append({"src": schema["mod1.source"]["choices"][mapped_source]
                             if mapped_source < 24 else "Pitch Bend",
                           "dst": target, "amount": amount, "bipolar": True, "curve": 1})
    for kind in range(1,11):
        put(f"fx{kind}.on", 0)
    order = []
    for slot in range(1,9):
        prefix = f"r{slot}_"
        kind = FX_TYPES[int(get(prefix+"type"))]
        enabled = get(prefix+"on")
        a,b,c,mix = (get(prefix+k) for k in ("a","b","c","mix"))
        if not kind or not enabled:
            continue
        if kind in order:
            unmapped[f"rack.slot{slot}"] = {key:old[prefix+key] for key in ("type","on","a","b","c","mix")}
            continue
        order.append(kind)
        key = f"fx{kind}."
        put(key+"on", 1)
        put(key+"mix", mix)
        for suffix,value in zip("abc", fx_values(kind,a,b,c)):
            put(key+suffix, value)
        if kind == 4:
            put(key+"sync", 1)
            put(key+"division", [2,4,5,6,6,7][min(5,int(a*6))])
        if kind == 6:
            put(key+"mode", 0 if b < .4 else 1 if b < .75 else 2)
    order.extend(k for k in range(1,11) if k not in order)
    for slot,kind in enumerate(order,1):
        put(f"rack.slot{slot}", kind-1)
    unmapped.update({key: old[key] for key in sorted(old.keys()-used) if key in preset["values"]})
    return {"format":"dualsynth-patch","version":1,"engine":"B",
            "name":f"Claude / {preset['name']}", "category":"Claude Compatibility",
            "tags":["Claude", "Compatibility", preset["category"]],
            "params":p, "mod":routes,
            "compatibility":{"source_commit":SOURCE,"source_index":preset["index"],
                             "unmapped":unmapped}}

def main():
    schema = {row["key"]:row for row in json.loads(Path(sys.argv[1]).read_text())}
    for osc in (1,2):
        schema[f"osc{osc}.claude.table"] = {"min":0,"max":30,"integer":True}
    defaults,presets = read_source()
    result = [convert(defaults,preset,schema) for preset in presets]
    (BANK/"claude-prism.json").write_text(json.dumps(result,separators=(",",":"),ensure_ascii=True)+"\n")

if __name__ == "__main__":
    main()
