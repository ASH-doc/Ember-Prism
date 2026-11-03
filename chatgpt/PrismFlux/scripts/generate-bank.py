#!/usr/bin/env python3
"""Generate 116 original musical starting recipes plus init, using the native schema."""
import json
from pathlib import Path

CATEGORIES=[
 ('Plucks','Glacial Pin|Crystal Relay|Ion Droplet|Diamond Circuit|Polar Staccato|Prism Needle|Glass Satellite|Quartz Spark|Neon Pebble|Skyline Plink|Azure Keys|Laser Dew|Silver Pulse|Frozen Orbit'),
 ('Leads','Prismatic Flight|Chrome Horizon|Aurora Vector|Photon Rally|Ultraviolet Arc|Diamond Wake|Ion Front|Lucent Blade|Spectral Summit|Mirror Anthem|Violet Overdrive|Digital Comet|Skyline Laser|Radiant Motion'),
 ('Basses','Ion Foundation|Digital Undertow|Chrome Roller|Prism Teeth|Neon Root|Polar Engine|Circuit Depth|Violet Rubber|Laser Current|Quartz Lowline|Glacial Pulse|Spectral Bite|Cobalt Floor|Mirror Sub'),
 ('Pads and atmospheres','Cloud Geometry|Aurora Lagoon|Crystal Expanse|Polar Veil|Skyglass Field|Violet Canopy|Prism Aurora|Airborne Choir|Frozen Cathedral|Neon Nebula|Silver Stratosphere|Lucent Ocean|Diamond Haze|Orbital Light'),
 ('Textures and movement','Liquid Pixels|Spectral Loom|Photon Rain|Crystal Tides|Vowel Mosaic|Mirror Leaves|Polar Fragments|Ion Garden|Violet Network|Air Geometry'),
 ('Arps and sequences','Photon Staircase|Quartz Courier|Prism Switches|Chrome Carousel|Ion Sequence|Digital Raindrops|Crystal Compass|Violet Raceway|Polar Telegraph|Mirror Steps|Skyglass Ratchet|Neon Circuitry|Aurora Numbers|Cobalt Clock'),
 ('Keys bells and mallets','Crystal Tines|Photon Bell|Quartz Mallet|Glass Cathedral|Ion Celesta|Digital Harp|Polar Chimes|Mirror Marimba|Violet Gong|Silver Keys'),
 ('Vocal and formant','Vowel Spectrum|Prism Choir|Digital Whisper|Ion Throat|Aurora Mouth|Chrome Vowels|Polar Voices|Lucent Syllables'),
 ('Chords and stabs','Quartz Chord|Prism Strike|Neon Emblem|Chrome Fan|Polar Stamp|Violet Assembly|Crystal Impact|Aurora Stabs|Ion Polygon|Mirror Chords'),
 ('FX risers and impacts','Photon Launch|Crystal Fracture|Ion Ascent|Polar Vacuum|Spectral Drop|Chrome Meteor|Aurora Surge|Digital Collapse'),
]

def design(family,i):
    p={'osc1.table':5+i%5,'osc1.pos':.12+(i%7)*.1,'osc2.table':i%5,
       'osc2.pos':.2+(i%5)*.11,'osc2.fine':[-7,5,-11,9][i%4],
       'osc1.warp1.type':2,'osc1.warp1.amount':0,'osc1.uni.voices':4,'osc1.uni.detune':.14+(i%5)*.035,'osc2.level':.24,
       'macro.euphoria':.12+(i%4)*.1,'macro.space':.14+(i%3)*.09,
       'fx4.mix':.16,'fx5.mix':.18,'fx10.mix':.35,'master.gain':-9}
    routes=[('Macro 1','osc1.pos',.5),('Macro 1','osc2.pos',.35),
            ('Macro 2','osc1.warp1.amount',.5),('Macro 3','flt1.cutoff',.3),
            ('Macro 4','lfo1.division',-.4),('Macro 5','osc2.level',.4),
            ('Macro 6','osc1.uni.spread',.4),('Macro 7','fx4.mix',.5),
            ('Macro 8','fx5.mix',.5),('Wheel','osc1.pos',.15)]
    if family==0:
        p.update({'env1.attack':.002,'env1.decay':.16+i*.018,'env1.sustain':0,
          'env1.release':.12,'env2.attack':.001,'env2.decay':.12+i*.02,'env2.sustain':0,
          'flt1.cutoff':900+i*150,'flt1.env':.65,'flt1.res':.15+i*.02,
          'osc1.warp1.type':2 if i%3 else 6,'osc1.warp1.amount':.1+i*.012,
          'fx4.mix':.27,'fx4.b':.38,'fx5.mix':.15})
    elif family==1:
        p.update({'osc1.uni.voices':8,'osc2.uni.voices':5,'osc2.level':.35,
          'osc1.uni.detune':.18+i*.019,'osc1.uni.wtspread':.25,'osc2.uni.wtspread':.12,
          'osc1.warp1.type':[1,2,5,6][i%4],'osc1.warp1.amount':.08+i*.012,
          'flt1.cutoff':6000+i*650,'env1.attack':.005,'env1.release':.24+i*.03,
          'fx1.on':1,'fx1.mix':.18,'fx4.mix':.22,'fx5.mix':.2,'macro.euphoria':.35})
    elif family==2:
        p.update({'osc1.octave':-1,'osc2.octave':-1,'osc1.uni.voices':1+i%3,
          'osc1.table':5+i%10,'osc2.level':.16,'mix.sub':.35,'voice.mode':1,
          'voice.glide':.025,'flt1.cutoff':350+i*90,'flt1.res':.2+i*.02,
          'flt1.env':.6,'env1.attack':.002,'env1.decay':.22,'env1.sustain':.12,
          'env1.release':.07,'env2.decay':.18+i*.01,'env2.sustain':0,
          'osc1.warp1.type':6 if i%2 else 2,'osc1.warp1.amount':.2,
          'fx6.on':1,'fx6.mode':1 if i%3==0 else 0,'fx6.mix':.2,
          'fx4.on':0,'fx5.on':0,'fx10.on':0,'macro.space':0})
        routes.append(('LFO 1','osc1.pos',.08+i*.01))
    elif family==3:
        p.update({'osc1.table':15+i%15,'osc2.table':5+i%10,'osc1.uni.voices':6,
          'osc2.uni.voices':4,'osc1.uni.wtspread':.55,'osc2.uni.wtspread':.4,
          'env1.attack':.7+i*.12,'env1.release':2+i*.17,'env2.attack':1.4,
          'flt1.cutoff':7000+i*500,'env1.sustain':.85,'lfo1.division':9,
          'lfo2.division':8,'fx1.on':1,'fx1.mix':.38,'fx5.mode':2,
          'fx5.a':.8,'fx5.b':.18,'fx5.c':.3,'fx5.mix':.38,'macro.space':.5})
        routes.extend([('LFO 1','osc1.pos',.2),('LFO 2','osc2.pos',.25),('LFO 3','flt1.cutoff',.08)])
    elif family==4:
        p.update({'osc1.table':10+i%20,'osc2.table':15+i%15,'osc1.uni.voices':5,
          'osc1.uni.wtspread':.7,'env1.attack':.4,'env1.release':1.8,
          'osc1.warp1.type':5,'osc1.warp1.amount':.2,'flt1.type':5 if i%2 else 6,
          'flt1.cutoff':700+i*190,'flt1.res':.4,'fx3.on':1,'fx3.mix':.25,
          'fx5.mode':2,'fx5.mix':.35,'macro.space':.4})
        routes.extend([('LFO 1','osc1.pos',.3),('LFO 2','flt1.vowel',.3),('Sample hold','osc2.pos',.2)])
    elif family==5:
        p.update(design(0,i)[0]);p.update({'arp.on':1 if i<7 else 0,'seq.on':0 if i<7 else 1,
          'arp.mode':i%6,'arp.octaves':1+i%3,'arp.rate':2 if i%3 else 3,
          'seq.rate':2,'seq.mode':i%4,'seq.length':.5,'fx4.division':5,'fx4.mix':.3})
        pattern=[0,7,12,3,0,10,7,12,0,3,7,15,12,7,3,10]
        for n in range(16):
            p[f'seq.step{n+1}.pitch']=pattern[(n+i)%16]
            p[f'seq.step{n+1}.velocity']=[1,.65,.85,.7][n%4]
            p[f'seq.step{n+1}.gate']=0 if (n+i)%7==0 else .8
    elif family==6:
        p.update({'osc1.table':20+i%5,'osc2.table':5+i%5,'osc2.level':.12,
          'osc1.uni.voices':1+i%2,'osc1.uni.detune':.06,'env1.attack':.001,
          'env1.decay':.6+i*.12,'env1.sustain':0,'env1.release':.35,
          'flt1.cutoff':14000,'osc1.warp1.type':6,'osc1.warp1.amount':.04+i*.01,
          'fx4.mix':.16,'fx5.mix':.32,'macro.space':.3})
    elif family==7:
        p.update({'osc1.table':10+i%5,'osc2.table':10+(i+2)%5,'osc1.uni.voices':4,
          'osc2.level':.3,'flt1.type':6,'flt1.vowel':i/8,'flt1.res':.4,
          'env1.attack':.08 if i%2 else .5,'env1.release':.8,'fx5.mode':2,
          'fx5.mix':.3,'macro.space':.35,'lfo1.division':8})
        routes.extend([('LFO 1','flt1.vowel',.3),('LFO 2','osc1.pos',.2)])
    elif family==8:
        p.update({'osc1.table':i%5,'osc2.table':5+i%5,'osc1.uni.voices':8,
          'osc2.uni.voices':4,'osc2.tune':7 if i%3==0 else 0,'env1.attack':.003,
          'env1.decay':.28+i*.025,'env1.sustain':.1,'env1.release':.18,
          'flt1.env':.4,'flt1.cutoff':1800+i*450,'fx4.mix':.22,'fx5.mix':.23,
          'macro.euphoria':.35,'macro.space':.22})
    else:
        p.update({'osc1.table':25+i%5,'osc2.level':.08,'mix.noise':.25,'mix.sub':0,
          'osc1.uni.voices':4,'env1.attack':3 if i<4 else .001,'env1.decay':2,
          'env1.sustain':0,'env1.release':1.5,'env2.attack':3 if i<4 else .001,
          'env2.decay':2,'env2.sustain':0,'flt1.cutoff':350 if i<4 else 14000,
          'flt1.env':.85 if i<4 else -.85,'flt1.res':.6,'fx2.on':1,'fx2.mix':.4,
          'fx5.mode':2,'fx5.mix':.45,'macro.space':.5})
        routes.append(('Env 2','osc1.pos',.65))
    return p,routes

def main():
    root=Path(__file__).resolve().parents[1]
    schema=json.loads((root/'docs/parameters.json').read_text())
    defaults={p['key']:p['default'] for p in schema}
    ids=list(defaults)
    source_list=next(p['choices'] for p in schema if p['key']=='mod1.source')
    def patch(name,category,changes,routes):
        values=dict(defaults);values.update(changes)
        for n,(source,dest,amount) in enumerate(routes,1):
            values[f'mod{n}.source']=source_list.index(source)
            values[f'mod{n}.destination']=ids.index(dest)+1
            values[f'mod{n}.depth']=amount
        for spec in schema:
            assert spec['min']<=values[spec['key']]<=spec['max'],(name,spec['key'])
        return {'format':'dualsynth-patch','version':1,'engine':'B','name':name,'category':category,
          'tags':['crystal','modern','138bpm']+category.lower().replace(' and ',' ').split()+(['motion'] if routes else ['init']),
          'macros':['Scan','Edge','Lift','Motion','Density','Width','Echo','Bloom'],
          'params':values,'mod':[{'src':s,'dst':d,'amount':a,'bipolar':True,'curve':1} for s,d,a in routes]}
    bank=[patch('Init / Crystal','Init',{},[])]
    for family,(category,names) in enumerate(CATEGORIES):
        for i,name in enumerate(names.split('|')):
            changes,routes=design(family,i);bank.append(patch(name,category,changes,routes))
    assert len(bank)==117
    (root/'Presets/factory.json').write_text(json.dumps(bank,indent=2)+'\n')
    (root/'docs/PRESETS.md').write_text('# Prism Flux bank\n\n116 original synthesis recipes + Init. Render-tested; producer audition remains outstanding.\n\n'+'\n'.join(f"- {i:03}: {p['category']} / {p['name']}" for i,p in enumerate(bank))+'\n')
    print(len(bank),'patches;',len(schema),'parameters')
if __name__=='__main__':main()
