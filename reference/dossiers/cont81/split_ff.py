import re, os, json
S = '/tmp/claude-1000/-home-stevenlafl-Projects-goldsrc-net-mod-ts/d03aef4b-20d4-42e9-a274-7611e5e053d1/scratchpad'
addrmap = {
0x8020c:'CalcSpread',0x86ea0:'DropKickback',0x7f0a4:'GetBodyByItems',0x86e2c:'GetCloseCombatVectors',
0x8705c:'GetWeaponType',0x801d4:'IsOnlyAkimbo',0x870c8:'IsRestrictedWeapon',0x8019c:'IsWeaponAkimbo',
0x7f2cc:'Set_ammo_count',0x7f51c:'compile_ammo_info',0x7f1f4:'compile_range_damage',0x7f1c4:'get_ammo_info',
0x7f290:'get_range_multipler',0x9ea3c:'AdjustSpecificWeapon',0x9eb14:'InformClientsOnWeaponsParams',
0x9fae0:'PlayerIsCheating',0x9fb4c:'RemoveEntityFromSlowedArray',0x9eb98:'SendWeaponParams',
0x9f238:'TSCountPlayers',0x9fa64:'TSFoundRandomEntity',0x9f94c:'TSPlayerDisconnect',0x9f320:'TSPrecache',
0x9e944:'UTIL_EntInArray',0x9e9c0:'WeaponIdFromString',0x9eca4:'adjust_weapon_value',0x9f044:'save_weapon_value',
0x767ac:'DecideGamePlay',0x76b48:'DoSpectacularNess',0x76c4c:'GetAccelBySlots',0x75ff4:'SetPlayerRespawnTime',
0x76244:'TSSelectSpawnSpot',0x76c8c:'VictimKatanaHold',0x76de4:'VictimKnifeHold',0x76f50:'VictimKungfuHold',
0x76ad8:'WeaponRestrictionByID',0xf97cc:'CanJumpHigh',0xf8fc4:'FixVelocity',0xf8fc8:'PseudoClimb',
0x6f4f4:'UTIL_TraceLine_Expanded',0x7c850:'m61_grenade',0x89140:'TSGetPointsForFrag',
0x95bac:'FindHullIntersectionplayer',0x974f4:'GetRandomPosition',
}
results = {}
for b in ['ff_batch1.txt','ff_batch2.txt','ff_batch3.txt']:
    text = open(os.path.join(S,b)).read()
    parts = re.split(r'(?m)^(// ===== FUN @ ([0-9a-fA-F]+) size=\d+ =====)$', text)
    # parts: [pre, header, addr, body, header, addr, body, ...]
    i = 1
    while i+2 < len(parts)+1 and i < len(parts):
        header, addr, body = parts[i], parts[i+1], parts[i+2]
        a = int(addr,16)
        name = addrmap.get(a)
        if name:
            results[name] = (a, header + body)
        i += 3
report = {}
for a,name in addrmap.items():
    if name in results:
        addr, content = results[name]
        path = os.path.join(S, 'ff_%s.c' % name)
        open(path,'w').write(content.rstrip()+'\n')
        # crude emptiness check: must contain a brace block
        ok = '{' in content and '}' in content and len(content.strip()) > 60
        report[name] = {'addr': hex(a), 'file': path, 'ok': ok, 'lines': content.count('\n')}
    else:
        report[name] = {'addr': hex(a), 'file': None, 'ok': False, 'lines': 0}
print(json.dumps(report, indent=1))
