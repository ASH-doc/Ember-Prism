#!/usr/bin/env python3
"""Reproducible original starting bank: 114 designed variations plus init.
These are synthesis recipes, not copied or ear-approved factory content.
"""
import json
from pathlib import Path

BANK = [
 ("Supersaw leads", "Copper Horizon|Amber Armada|Molten Avenue|Afterglow Engine|Night Convoy|Brass Meridian|Solar Furnace|Redline Halo|Ember Rally|Velvet Voltage|Saffron Signal|Rust Skyline|Dusk Anthem|Cinder Crown"),
 ("Trance plucks", "Brass Droplet|Soot Spark|Copper Pin|Lantern Step|Velvet Arrow|Warm Relay|Tungsten Rain|Burnished Echo|Ochre Current|Lowlight Pinch|Midnight Pulse|Glowing Wire|Rust Bell|Apricot Staccato"),
 ("Gated pads and stabs", "Warehouse Lanterns|Brass Shutters|Dusk Chopper|Furnace Steps|Amber Strobe|Copper Lattice|Night Aperture|Saffron Grid|Molten Stabs|Velvet Guillotine|Ember Tiles|Warm Transit"),
 ("Basses", "Copper Foundation|Rust Roller|Warm Concrete|Furnace Floor|Soot Acid|Tungsten Hook|Ember Underpass|Subterranean Brass|Amber Offbeat|Midnight Rubber|Grit Circuit|Lowlight Anchor|Cinder Pulse|Velvet Sub|Dark Dynamo|Saffron Drive"),
 ("Pads and strings", "Walnut Sky|Fading Lantern|Copper Fog|Warm Observatory|Velvet Reservoir|Ochre Strings|Ember Cloud|Night Bloom|Brass Veil|Dusk Memory|Soot Aurora|Amber Drift|Lowlight Choir|Furnace Snow"),
 ("Arps and sequences", "Copper Clockwork|Lantern Ladder|Saffron Run|Dusk Courier|Brass Spiral|Velvet Transit|Night Telegraph|Ember Cascade|Rust Compass|Warm Ratchet|Ochre Orbit|Furnace Cipher"),
 ("Analog polys and brass", "Walnut Poly|Copper Ensemble|Amber Horns|Dusk Clavinet|Velvet Fifths|Brass Workshop|Ember Orchestra|Saffron Chords|Warm Parliament|Night Satin"),
 ("Keys and organs", "Lantern Keys|Copper Tines|Velvet Drawbars|Ember Reed|Dusk Console|Warm Chapel"),
 ("FX risers and drops", "Furnace Ignition|Copper Ascent|Dusk Pressure|Ember Exhaust|Soot Meteor|Brass Descent|Warm Vacuum|Lantern Impact"),
 ("Mono leads and solos", "Copper Solo|Velvet Glide|Lantern Ribbon|Dusk Whistle|Warm Messenger|Saffron Soloist|Rust Needle|Ember Monologue"),
]

def recipe(category, i):
    # Each family uses a deliberate voice, articulation, filter and effects recipe.
    p = {"analog.age": .1 + (i % 5) * .075, "macro.warmth": .28 + (i % 4)*.12,
         "macro.saturation": .08 + (i % 5)*.045, "master.volume": -9,
         "osc1.flavor": i % 4, "flt1.model": (i//4) % 4,
         "flt1.cutoff": [2500, 3800, 1600, 5800, 900][i%5],
         "osc2.fine": [-7, 7, -4, 11][i%4], "fx.delay.tone": 2400 + i*130,
         "macro.1": 0, "macro.2": 0, "macro.3": 0, "macro.4": 0}
    routes = [("Mod wheel", "flt1.cutoff", .22), ("Macro 1", "flt1.cutoff", .38),
              ("Macro 2", "osc1.uni.detune", .42), ("Macro 2", "osc2.uni.detune", .35),
              ("Macro 3", "fx.delay.mix", .45), ("Macro 4", "flt1.drive", .5)]
    if category == 0:
        p.update({"osc1.wave":4,"osc2.wave":0,"osc2.uni.voices":4,"osc2.level":.38,
          "osc1.uni.detune":.24+i*.018,"osc2.uni.detune":.18+i*.012,"osc1.uni.spread":.65+i*.02,
          "osc3.level":.08,"osc3.octave":-1,"env1.attack":.006,"env1.release":.25+i*.03,
          "flt1.env":.2,"fx.chorus.on":1,"fx.chorus.mix":.16,"fx.delay.on":1,
          "fx.delay.mix":.15+i*.008,"fx.reverb.on":1,"fx.reverb.mix":.12,
          "voice.glide":.06 if i%4==0 else 0})
    elif category == 1:
        p.update({"osc1.wave":i%2,"osc1.uni.voices":3,"osc1.uni.detune":.16+i*.018,
          "osc2.wave":2,"osc2.level":.24,"osc2.coarse":12 if i%3==0 else 0,
          "flt1.cutoff":500+i*120,"flt1.res":.16+i*.025,"flt1.env":.55,
          "env1.attack":.002,"env1.decay":.18+i*.018,"env1.sustain":0,"env1.release":.12,
          "env2.attack":.001,"env2.decay":.12+i*.023,"env2.sustain":0,"fx.delay.on":1,
          "fx.delay.mix":.28,"fx.delay.feedback":.35,"fx.reverb.on":1,"fx.reverb.mix":.12})
    elif category == 2:
        p.update({"osc1.wave":4,"osc1.uni.detune":.36,"osc2.wave":1,"osc2.pw":.3,
          "osc2.uni.voices":3,"osc2.level":.25,"env1.attack":.08 if i%2==0 else .004,
          "env1.sustain":.85,"env1.release":.5,"seq.gate.on":1,"seq.gate.depth":.85,
          "seq.gate.shape":.35,"seq.gate.rate":2 if i%3 else 4,
          "fx.pump.on":1,"fx.pump.mix":.5,"fx.pump.depth":.6,"fx.delay.on":1,
          "fx.delay.mix":.17,"fx.reverb.on":1,"fx.reverb.mix":.22})
        patterns = [[1,0,.7,0,1,.4,0,.7,1,0,.5,0,1,.7,.3,0],
                    [1,1,0,.3,1,0,.8,0,1,1,0,.7,1,0,.5,0],
                    [1,.4,.2,0,1,.2,.7,0,1,.5,0,.4,1,0,.8,.2]]
        for n,v in enumerate(patterns[i%3]): p[f"seq.gate.step{n+1}"]=v
    elif category == 3:
        p.update({"osc1.wave":i%2,"osc1.octave":-1,"osc2.level":.22,"osc2.octave":-1,
          "mix.sub":.35,"voice.mode":1,"voice.glide":.03 if i%3 else .09,
          "flt1.cutoff":180+i*48,"flt1.res":.75 if i%4==0 else .2,
          "flt1.model":2 if i%4==0 else 0,"flt1.env":.65,"flt1.drive":.3,
          "env1.decay":.2+i*.014,"env1.sustain":.2 if i%2 else 0,"env1.release":.09,
          "env2.decay":.14+i*.013,"env2.sustain":0,"macro.saturation":.3,
          "fx.drive.on":1,"fx.drive.amount":.15,"fx.drive.mix":.3})
    elif category == 4:
        p.update({"osc1.wave":i%2,"osc1.uni.voices":4,"osc1.uni.detune":.24,
          "osc2.wave":2,"osc2.level":.4,"osc2.uni.voices":3,"osc3.wave":3,"osc3.level":.15,
          "env1.attack":.7+i*.15,"env1.release":2+i*.2,"env2.attack":1.2,"env2.release":3,
          "flt1.cutoff":1200+i*180,"flt1.env":.2,"lfo1.rate":.08+i*.012,
          "fx.chorus.on":1,"fx.chorus.mix":.4,"fx.reverb.on":1,"fx.reverb.mix":.4,
          "fx.reverb.mode":1,"fx.reverb.size":.8,"fx.delay.on":1,"fx.delay.mix":.12})
        routes.extend([("LFO 1","flt1.cutoff",.08),("LFO 2","osc1.pw",.2)])
    elif category == 5:
        p.update(recipe(1,i)[0]);p.update({"arp.on":1,"arp.mode":i%6,"arp.octaves":1+i%3,
          "arp.rate":2 if i%2 else 3,"arp.swing":.12 if i%3==0 else 0,"arp.gate":.45+i*.025,
          "fx.delay.division":5,"fx.delay.mix":.24,"fx.delay.feedback":.42})
        for n in range(16):p[f"arp.velocity{n+1}"]=[1,.6,.8,.55][(n+i)%4]
    elif category == 6:
        p.update({"osc1.wave":i%2,"osc2.wave":1,"osc2.pw":.35,"osc2.level":.5,
          "osc1.uni.voices":2,"osc1.uni.detune":.15,"env1.attack":.025+i*.009,
          "env1.release":.25,"flt1.model":1,"flt1.env":.3,"env2.attack":.06,
          "env2.decay":.5,"env2.sustain":.3,"fx.chorus.on":1,"fx.chorus.mix":.35,
          "fx.reverb.on":1,"fx.reverb.mix":.18})
    elif category == 7:
        p.update({"osc1.wave":3,"osc2.wave":3,"osc2.coarse":12,"osc2.level":.4,
          "osc3.wave":3,"osc3.coarse":19,"osc3.level":.15,"mix.sub":.12,
          "env1.attack":.003,"env1.decay":.5+i*.12,"env1.sustain":.7 if i>1 else .05,
          "env1.release":.18,"flt1.cutoff":4800,"fx.chorus.on":1,"fx.chorus.mix":.2,
          "fx.reverb.on":1,"fx.reverb.mix":.25,"macro.saturation":.15})
    elif category == 8:
        p.update({"osc1.level":.15,"osc2.level":0,"mix.noise":.4,"mix.sub":.1,
          "flt1.res":.8,"flt1.env":.8 if i<4 else -.8,"flt1.cutoff":400 if i<4 else 8000,
          "env1.attack":2+i*.2 if i<4 else .002,"env1.decay":2,"env1.sustain":0,
          "env1.release":1.5,"env2.attack":4 if i<4 else .001,"env2.decay":2,
          "env2.sustain":0,"fx.flanger.on":1,"fx.flanger.mix":.4,
          "fx.reverb.on":1,"fx.reverb.mix":.4,"fx.reverb.size":.85,
          "fx.delay.on":1,"fx.delay.mix":.25})
    else:
        p.update({"voice.mode":2,"voice.glide":.06+i*.025,"osc1.wave":i%4,
          "osc2.wave":1,"osc2.pw":.3,"osc2.level":.24,"osc1.uni.voices":2,
          "osc1.uni.detune":.12,"flt1.cutoff":1600+i*450,"flt1.res":.3,
          "env1.sustain":.85,"env1.release":.18,"fx.delay.on":1,"fx.delay.mix":.24,
          "fx.reverb.on":1,"fx.reverb.mix":.15,"lfo1.rate":5.2})
        routes.append(("Mod wheel","osc1.fine",.035))
    return p,routes

def main():
    root=Path(__file__).resolve().parents[1]
    schema=json.loads((root/"docs/parameters.json").read_text())
    ids=[p['id'] for p in schema]
    defaults={p['id']:p['default'] for p in schema}
    sources=["Off","Velocity","Key","Mod wheel","Pressure","Env 1","Env 2","Env 3","LFO 1","LFO 2","LFO 3","Random","Macro 1","Macro 2","Macro 3","Macro 4"]
    def patch(name,category,p,routes):
        values=dict(defaults); values.update(p)
        for n,(src,dst,amount) in enumerate(routes,1):
            values[f'mod.{n}.source']=sources.index(src); values[f'mod.{n}.dest']=ids.index(dst)+1;values[f'mod.{n}.amount']=amount
        for d in schema:
            assert d['min']<=values[d['id']]<=d['max'],(name,d['id'],values[d['id']])
        return dict(format="dualsynth-patch",version=1,engine="A",name=name,category=category,
                    tags=["analog","warm","original"],params=values,
                    macros=["Open","Width","Echo","Bite"],mod=[dict(src=s,dst=d,amount=a) for s,d,a in routes],
                    fx=[dict(type=k,on=bool(values[f'fx.{k}.on']),params={key.split('.')[-1]:v for key,v in values.items() if key.startswith(f'fx.{k}.') and not key.endswith('.on')}) for k in ['drive','chorus','flanger','phaser','delay','reverb','eq','pump']])
    bank=[patch("Init - Warm Saw","Init",{},[])]
    for category,(label,names) in enumerate(BANK):
        for i,name in enumerate(names.split('|')):
            p,routes=recipe(category,i);bank.append(patch(name,label,p,routes))
    assert len(bank)==115
    (root/'Presets/factory.json').write_text(json.dumps(bank,indent=2)+'\n')
    (root/'docs/PRESET-LIST.md').write_text('# Ember VA preset bank\n\n114 original synthesis recipes plus Init. These need musical audition; they are not claimed to have been reviewed by ear.\n\n'+'\n'.join(f"- {n:03d} · {p['category']} · {p['name']}" for n,p in enumerate(bank))+'\n')
    print(f"Wrote {len(bank)} patches with {len(ids)} parameters each.")
if __name__=='__main__':main()
