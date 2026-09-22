"""Opt-in original-resource/process acceptance. Never edits game data or injects state.
Usage: python tests/RunConsequenceWitnesses.py BUILD_DIRECTORY INSTALLATION
Legacy saves and transcripts go to BUILD_DIRECTORY/m33-evidence.
Append --m34 for bounded lifecycle acceptance under BUILD_DIRECTORY/m34-evidence.
"""
import json
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

build, game = map(lambda x: Path(x).resolve(), sys.argv[1:3])
m34 = "--m34" in sys.argv[3:]
output = build / ("m34-evidence" if m34 else "m33-evidence")
output.mkdir(exist_ok=True)
exe = build / "mmodern_consequence_cli_witness.exe"
env = os.environ.copy()
for key in ("MMODERN_M33_EXPECT", "MMODERN_M33_ORACLE", "MMODERN_M33_ROUTE", "MMODERN_M33_FAULT", "MMODERN_M33_FAIL_DRAW", "MMODERN_M34_POLICY", "MMODERN_M34_STOP_AFTER_EXIT", "MMODERN_M34_CHECKPOINT_FILE", "MMODERN_M34_CHECKPOINT_ROUTE"):
    env.pop(key, None)
env["MMODERN_M33_SUMMARY"] = "1"
replay_selected = "--replay-selected" in sys.argv[3:]
retained_recipes = json.loads((output / "selected-recipes.json").read_text()) if replay_selected else {}
index = (output / ("processes-replay.log" if replay_selected else "processes.log")).open("w")

def run(name, route, seed=3, source=None, expected=None, oracle=None, failure=False, fault=None, fail_draw=None, policy=None, stop=False, checkpoint=None, checkpoint_route=None):
    if replay_selected:
        name = "final-" + name
    save = output / (name + ".mmsave")
    local = env.copy()
    local["MMODERN_M33_ROUTE"] = route
    if policy is not None:
        local["MMODERN_M34_POLICY"] = policy
    if stop:
        local["MMODERN_M34_STOP_AFTER_EXIT"] = "1"
    if checkpoint is not None:
        local["MMODERN_M34_CHECKPOINT_FILE"] = str(checkpoint)
        local["MMODERN_M34_CHECKPOINT_ROUTE"] = str(checkpoint_route)
    if source:
        shutil.copyfile(source, save)
        args = ["--load-game", str(game), str(save)]
    else:
        args = ["--journey-region", "--combat-seed", str(seed), str(game), "--save-file", str(save)]
    if fail_draw:
        local["MMODERN_M33_FAIL_DRAW"] = str(fail_draw)
    if fault:
        local["MMODERN_M33_FAULT"] = fault
    if expected:
        local["MMODERN_M33_EXPECT"] = str(expected)
    if oracle:
        local["MMODERN_M33_ORACLE"] = oracle
    with (output / (name + ".log")).open("w") as log:
        child = subprocess.Popen([str(exe), *args], env=local, stdout=log, stderr=subprocess.STDOUT)
        code = child.wait()
    text = (output / (name + ".log")).read_text()
    index.write(f"{name} PID={child.pid} exit={code} seed={seed} route={route} policy={policy} stop={stop} checkpoint={checkpoint} checkpoint_route={checkpoint_route} source={source} expected={expected} args={args}\n")
    index.flush()
    if fail_draw:
        assert code == 4 and f"ARTIFICIAL RAW-DRAW FAILURE ATOMICITY PASS {fail_draw}" in text, text[-1500:]
        return save, text, code
    if code and not failure:
        raise RuntimeError(f"{name} failed: {text[-1500:]}")
    terminal = re.search(r'TERMINAL phase=\d+ failure=(\d+)', text)
    if code and m34 and failure and terminal and int(terminal[1]) not in (0, 3):
        raise RuntimeError(f"{name}: integrity/preparation/observation/overflow failure is not a seed-selection outcome: {text[-2000:]}")
    if code and m34 and failure and not any(reason in text for reason in (
            'TERMINAL phase=', 'Exploration support stop', 'M34 128 resource-attack search bound',
            'M34 600 player-input search bound', 'M34 64 charged return-move bound',
            'No living original Orc remains', 'Return target must remain alive')):
        raise RuntimeError(f"{name}: unexpected integration failure during bounded selection: {text[-2000:]}")
    if code and not (m34 and failure) and ("TERMINAL phase=" not in text or "Combat terminal witness" not in text):
        raise RuntimeError(f"{name}: unexpected integration failure: {text[-1500:]}")
    if not code:
        assert ("M34 GAMEPLAY PASS" if policy is not None else "M33 GAMEPLAY PASS") in text
        if source:
            assert "FRESH PROCESS FULL STATE AND BYTES PASS" in text
        if expected:
            assert "CONTROL FULL SEMANTIC STATE AND BYTES PASS" in text
            assert save.read_bytes() == Path(expected).read_bytes(), name + ": exact on-disk bytes (including header/CRC) differ"
    return save, text, code

def branch(name, prefix, suffix, seed=3, oracle=None):
    checkpoint, _, _ = run(name + "-checkpoint", prefix, seed)
    control, _, _ = run(name + "-control", prefix + suffix, seed, oracle=oracle)
    resumed, _, _ = run(name + "-resume", suffix, seed, source=checkpoint, expected=control, oracle=oracle)
    return resumed

def legacy():
    wound, _, _ = run("fixed-wound", "UF", oracle="wound")
    for name, key in (("contact", "U"), ("ranged", "F")):
        control, _, _ = run("fixed-" + name, "UF" + key, oracle=name)
        run(name + "-resume", key, source=wound, expected=control, oracle=name)
        branch(name + "-further", "UF" + key, "RRF")
    branch("generated-armor", "UFUA", "ERRF")
    run("shoot-owed-prefix", "Q", expected=wound, oracle="wound")
    run("shoot-discarded-at-contact", "Q", source=wound, expected=output / "fixed-contact.mmsave", oracle="contact")
    for fault in ("shoot", "ranged", "lethal", "delivery", "credit"):
        run("fault-" + fault, "UFU", expected=output / "fixed-contact.mmsave", fault=fault)

    for route, counts in (("UFU", range(1, 23)), ("UFF", range(9, 15))):
        for count in counts:
            run(f"raw-fault-{route}-{count}", route, fail_draw=count)

    routes = {
        "snake": "LUUURUULURUULUUUUUUU",
        "toad": "LUUURUULURUULUULUURUURULUU",
        "undead": "LUUURUULURUULUUUUURUUUULU",
    }
    selected = {}
    negative = False
    for family, route in routes.items():
        for seed in range(1, 257):
            save, text, code = run(f"{family}-seed-{seed}", route, seed, failure=True)
            if code:
                if family == "toad":
                    negative = True
                continue
            obs = re.search(r"OBSERVATIONS poison=(\d+) sleep=(\d+) disease=(\d+) contacts=(\d+) grouped=(\d+)", text)
            poison, sleep, disease, contacts, grouped = map(int, obs.groups())
            conditions = re.search(r"CONDITIONS wake=(\d+) reapply=(\d+) allParty=(\d+) poisonDerived=(\d+) sleepSkipped=(\d+)", text)
            wake, reapply, all_party, changed, skipped = map(int, conditions.groups())
            living_poison = int(re.search(r"LIVING POISON (\d+)", text)[1])
            good = ((family == "snake" and poison and changed and living_poison and contacts & (1 << 12)) or
                    (family == "toad" and sleep and wake and reapply and all_party and skipped and contacts & ((1 << 14) | (1 << 15))) or
                    (family == "undead" and disease and contacts & (1 << 17) and contacts & (1 << 18)))
            if good:
                assert grouped >= 2
                selected[family] = (seed, save, text)
                run(family + "-repeat", route, seed, expected=save)
                branch(family + "-further", route, "R", seed)
                print("SELECTED", family, seed, flush=True)
                break
        else:
            raise RuntimeError(f"Acceptance failure: no {family} witness in seeds 1..256")
    assert negative, "Distinct Toad terminal control required"

    seed, saved, transcript = selected["snake"]
    route = routes["snake"]
    ends = list(map(int, re.findall(r"EPISODE QUIET route=(\d+)", transcript)))
    assert len(ends) >= 2 and ends[0] < ends[-1]
    branch("between-episodes", route[:ends[0]], route[ends[0]:], seed)
    # The exact reverse path returns to (9,11), using Right for the 180-degree tie.
    reverse = "RRUUUUUUURUULURUULUUU"
    base = route + reverse
    before = None
    for waits in range(48):
        save, text, _ = run("time-search-" + str(waits), base + "W" * waits, seed)
        minute = int(re.findall(r"STATE minute=(\d+)", text)[-1])
        if 950 <= minute < 960:
            before = base + "W" * waits
            break
        assert minute < 960, "Time route skipped the required pre-crossing Quiet"
    assert before, "Time witness did not reach pre-crossing Quiet"
    branch("time-crossing", before, "W", seed)
    branch("time-further", before + "W", "W", seed)
    # A genuine selected-survivor purse is retained by this same route.
    pending, text, _ = run("pending-checkpoint", before, seed)
    assert int(re.findall(r"STATE minute=\d+ pendingGold=(\d+)", text)[-1]) > 0
    for count in range(331, 341):
        run("tick-raw-fault-" + str(count), "W", seed, source=pending, fail_draw=count)
    branch("pending-collection", before, "RR", seed)
    branch("pending-once-only", before + "RR", "RRF", seed)

    loot = {}
    for seed in range(1, 4097):
        save, text, code = run("loot-seed-" + str(seed), "UFU", seed, failure=True)
        if code:
            continue
        for category, item in re.findall(r"LOOT (armor|weapon) id=(\d+)", text):
            item = int(item)
            loot.setdefault(category, (seed, item))
            if category == "weapon" and 30 <= item <= 33:
                loot.setdefault("missile", (seed, item))
        if len(loot) == 3:
            break
    assert len(loot) == 3, "Acceptance failure: no complete loot witness in seeds 1..4096"
    assert loot == {"armor": (3, 2), "weapon": (64, 32), "missile": (64, 32)}, loot
    branch("generated-weapon", "UFUGUF", "F", 64)
    sign, _, _ = run("regional-sign", "LUUURUUUURU", 3)
    run("regional-sign-no-replay", "", source=sign, expected=sign)
    branch("regional-sign-further", "LUUURUUUURU", "R", 3)
    # Explicitly artificial selected-survivor/item fixture, separate from genuine routes.
    fixture = output / "ARTIFICIAL-pending-item.mmsave"
    with (output / "original-controls.log").open("w") as log:
        subprocess.run([str(build / "mmodern_consequence_original.exe"), str(game), str(fixture)], env=env, stdout=log, stderr=subprocess.STDOUT, check=True)
    checkpoint, _, _ = run("ARTIFICIAL-item-restored", "", source=fixture, expected=fixture)
    control, text, _ = run("ARTIFICIAL-item-collection-control", "RR", source=fixture)
    assert "LOOT armor id=2" in text and "gold=810" in text
    run("ARTIFICIAL-item-collection-resume", "RR", source=checkpoint, expected=control)
    further, _, _ = run("ARTIFICIAL-item-further-control", "RRRRF", source=fixture)
    run("ARTIFICIAL-item-further-resume", "RRF", source=control, expected=further)
    print("All M33 fixed/bounded gameplay and fresh-process comparisons passed", selected.keys(), loot, flush=True)

def lifecycle():
    """Bounded M34 preparation; every selected case is replayed without search."""
    recipes = {}
    def seeds(name, default):
        return [retained_recipes[name]["seed"]] if name in retained_recipes else default
    def record(name, seed, route, policy, save, text, stop=False, source=None):
        (output / (name + '.log')).write_text(text)
        recipes[name] = dict(seed=seed, route=route, policy=policy, stop=stop,
                             save=str(save), source=str(source) if source else None, transcript=str(output / (name + '.log')))
        (output / 'selected-recipes.json').write_text(json.dumps(recipes, indent=2) + '\n')
        _, replay, _ = run(name + '-literal-replay', route, seed, expected=save,
                           policy=policy, stop=stop, source=source)
        # Literal accepted/rejected conversions and publication observations are
        # compared separately from exact complete save-state comparisons.
        def observations(log):
            return [line for line in log.splitlines() if line.startswith(
                ('DRAW ', 'RUN ', 'COMBAT INPUT ', 'PUBLICATION ', 'NAV INPUT ', 'RETURN INPUT ', 'RETIRED ', 'M34 STATE ',
                 'CONSEQUENCE ', 'INJURY ', 'XP ', 'ARMOR ', 'DROP ', 'RANGED ', 'ACTOR STATE ',
                 'GOLD CONSERVATION ', 'FORFEIT ', 'ITEM DELIVERY ', 'ITEM LOSS '))]
        assert observations(text) == observations(replay), name + ': literal transcript differs'
        print('SELECTED M34', name, seed, route, policy, flush=True)
        return save

    def state(text):
        pattern = (r'M34 STATE camera=(-?\d+),(-?\d+) facing=(\d+) '
                   r'actor9=(-?\d+),(-?\d+),(-?\d+),(\d+) gold=(\d+) '
                   r'pendingGold=(\d+) pendingMask=(\d+) storedItems=(\d+) dead=(\d+)')
        found = re.findall(pattern, text)
        assert found, text[-2000:]
        return dict(zip(('x', 'y', 'facing', 'actorX', 'actorY', 'actorHp', 'actorLifecycle',
                         'gold', 'pendingGold', 'pendingMask', 'items', 'dead'), map(int, found[-1])))

    def suffix_observations(text):
        prefixes = ('DRAW ', 'RUN ', 'COMBAT INPUT ', 'NAV INPUT ', 'RETURN INPUT ',
                    'CONSEQUENCE ', 'INJURY ', 'XP ', 'ARMOR ', 'DROP ', 'RANGED ',
                    'ACTOR STATE ', 'ITEM DELIVERY ', 'ITEM LOSS ', 'FORFEIT ', 'CONTACT ACTOR ')
        return [re.sub(r'\b(?:episode|revision)=\d+ ?', '', line).rstrip()
                for line in text.splitlines() if line.startswith(prefixes)]

    def compare_suffix(control, resumed, name):
        assert 'PROCESS CHECKPOINT\n' in control, name + ': missing exact checkpoint'
        uninterrupted = suffix_observations(control.split('PROCESS CHECKPOINT\n', 1)[1])
        restored = suffix_observations(resumed)
        (output / (name + '-suffix-comparison.json')).write_text(json.dumps({
            'uninterrupted': uninterrupted, 'restored': restored,
            'equal': uninterrupted == restored,
        }, indent=2) + '\n')
        assert uninterrupted == restored, name + ': semantic publication observations differ after fresh restart'

    def process_chain(name, route, policy, seed, suffix, suffix_policy, further, further_policy):
        checkpoint, checkpoint_text, _ = run(name + '-checkpoint', route, seed, policy=policy)
        run(name + '-restart-exact', '', seed, source=checkpoint, expected=checkpoint, policy=suffix_policy)
        control, control_text, _ = run(name + '-control', route + suffix, seed, policy=policy + ',' + suffix_policy,
                                     checkpoint=checkpoint, checkpoint_route=len(route))
        resumed, resumed_text, _ = run(name + '-resumed', suffix, seed, source=checkpoint, expected=control, policy=suffix_policy)
        compare_suffix(control_text, resumed_text, name)
        complete, complete_text, _ = run(name + '-further-control', route + suffix + further, seed,
                             policy=policy + ',' + suffix_policy + ',' + further_policy,
                             checkpoint=resumed, checkpoint_route=len(route + suffix))
        _, further_text, _ = run(name + '-further-resumed', further, seed, source=resumed, expected=complete, policy=further_policy)
        compare_suffix(complete_text, further_text, name + '-further')
        run(name + '-final-restart', '', seed, source=complete, expected=complete, policy='attack')
        return checkpoint, checkpoint_text

    wounded, text, _ = run('wounded-exit', 'UFU', 3, policy='run')
    initial = state(text)
    assert (initial['x'], initial['y'], initial['facing'], initial['actorHp']) == (10, 12, 3, 16), initial
    assert initial['actorLifecycle'] == 0 and initial['gold'] == 800
    record('wounded-exit', 3, 'UFU', 'run', wounded, text)
    process_chain('wounded-return', 'UFU', 'run', 3, 'H', 'run', 'H', 'attack')

    for seed in seeds('mixed-run', range(1, 4097)):
        save, text, code = run('mixed-search-' + str(seed), 'UFU', seed, policy='mixed', failure=True)
        if code:
            continue
        successes = re.findall(r'RUN .* success=1 ', text)
        failures = re.findall(r'RUN .* success=0 ', text)
        if successes and failures:
            record('mixed-run', seed, 'UFU', 'mixed', save, text)
            branch_save, branch_text, _ = run('mixed-victory', 'UFU', seed, policy='first-run-attack')
            assert 'RUN ' in branch_text and state(branch_text)['actorHp'] == 0
            record('mixed-victory', seed, 'UFU', 'first-run-attack', branch_save, branch_text)
            break
    else:
        raise RuntimeError('Acceptance failure: no failed and successful Run witness in seeds 1..4096')

    routes = {
        'snake': ('LUUURUULURUULUUUUUUU', 7),
        'toad': ('LUUURUULURUULUULUURUURULUU', 226),
        'undead': ('LUUURUULURUULUUUUURUUUULU', 18),
    }
    casualty = None
    for family, (route, retained) in routes.items():
        for seed in seeds('casualty-escape', [retained] + [n for n in range(1, 4097) if n != retained]):
            save, text, code = run(f'casualty-{family}-{seed}', route, seed,
                                   policy='first-run-block', stop=True, failure=True)
            if code:
                continue
            result = state(text)
            if result['dead'] and re.search(r'DISENGAGEMENT cause=2 casualties=[1-9]', text):
                casualty = (seed, route, save, text)
                record('casualty-escape', seed, route, 'first-run-block', save, text, True)
                break
        if casualty:
            break
    if casualty is None:
        raise RuntimeError('Acceptance failure: no genuine casualty escape in retained families/seeds 1..4096')
    seed, route, casualty_save, text = casualty
    run('casualty-restart-exact', '', seed, source=casualty_save, expected=casualty_save, policy='attack')
    checkpoint_route = route[:int(re.findall(r'EPISODE QUIET route=(\d+)', text)[-1])]
    episodes = int(re.findall(r'EPISODE QUIET route=\d+ episodes=(\d+)', text)[-1])
    process_chain('casualty-continuation', checkpoint_route, ','.join(['first-run-block'] * episodes), seed,
                  'R', 'attack', 'H', 'attack')

    treasure_routes = {'sign-group': ('LUUURUUUURUURRU', 3),
                       'snake-return': (routes['snake'][0] + 'RRUUUUUUURUULURUULUUU', 7)}
    treasure = None
    for seed in seeds('direct-run-dormant', range(1, 4097)):
        for family, (route, _) in treasure_routes.items():
            save, text, code = run(f'treasure-{family}-{seed}', route, seed,
                                   policy='item-run', stop=True, failure=True)
            if code:
                continue
            result = state(text)
            if result['items'] and not result['pendingMask'] and not result['pendingGold'] and 'DISENGAGEMENT cause=1' in text:
                treasure = (seed, route, save, text)
                record('direct-run-dormant', seed, route, 'item-run', save, text, True)
                break
        if treasure:
            break
    if treasure is None:
        raise RuntimeError('Acceptance failure: no genuine grouped Orc dormant-item Run in seeds 1..4096')
    seed, route, dormant, text = treasure
    run('dormant-restart-exact', '', seed, source=dormant, expected=dormant, policy='attack')
    before_state = state(text)
    continuation = None
    for suffix in ('O', 'OL', 'OR', 'ORR', 'OU', 'OLU', 'ORU', 'ORRU'):
        delivered, delivery_text, code = run('dormant-delivery-search-' + suffix, suffix, seed,
                                            source=dormant, policy='attack', failure=True)
        if code:
            continue
        after_state = state(delivery_text)
        if not after_state['items'] and not after_state['pendingMask'] and after_state['gold'] > before_state['gold']:
            continuation = suffix
            record('dormant-reactivated-delivered', seed, suffix, 'attack', delivered, delivery_text, source=dormant)
            break
    if continuation is None:
        raise RuntimeError('Acceptance failure: genuine dormant queue did not reactivate/deliver on bounded original Orc return')
    # Recover the literal prefix ending at the selected retirement, so the
    # uninterrupted branch does not accidentally execute the unused route tail.
    checkpoint_route = route[:int(re.findall(r'EPISODE QUIET route=(\d+)', text)[-1])]
    episodes = int(re.findall(r'EPISODE QUIET route=\d+ episodes=(\d+)', text)[-1])
    process_chain('dormant-delivery', checkpoint_route, ','.join(['item-run'] * episodes), seed,
                  'RUU', 'attack', continuation, 'attack')

    ready = None
    ready_routes = {'snake-return-contact': (routes['snake'][0] + 'RRUUUUUUURUULURUULUUUUUD', 7),
                    'sign-group': treasure_routes['sign-group']}
    for seed in seeds('ready-attrition', [7] + [n for n in range(1, 4097) if n != 7]):
        for family, (route, _) in ready_routes.items():
            save, text, code = run(f'ready-attrition-{family}-{seed}', route, seed,
                                   policy='pending-first-run-block', stop=True, failure=True)
            if code:
                continue
            result = state(text)
            if result['pendingMask'] and result['pendingGold'] and 'DISENGAGEMENT cause=2' in text:
                ready = (seed, route, save, text)
                record('ready-attrition', seed, route, 'pending-first-run-block', save, text, True)
                break
        if ready:
            break
    if ready is None:
        raise RuntimeError('Acceptance failure: no genuine ready-treasure attrition continuation in seeds 1..4096')
    seed, route, ready_save, text = ready
    run('ready-restart-exact', '', seed, source=ready_save, expected=ready_save, policy='attack')
    ready_wait, _, _ = run('ready-before-settlement', 'W', seed, source=ready_save, policy='attack')
    settlement = None
    for suffix in ('L', 'R', 'RR', 'U', 'LU', 'RU', 'RRU'):
        settled, settle_text, code = run('ready-settlement-search-' + suffix, suffix, seed,
                                        source=ready_wait, policy='attack', failure=True)
        if not code and not state(settle_text)['pendingMask']:
            settlement = suffix
            break
    if settlement is None:
        raise RuntimeError('Acceptance failure: selected surviving threat did not permit bounded ready-treasure settlement')
    checkpoint_route = route[:int(re.findall(r'EPISODE QUIET route=(\d+)', text)[-1])]
    episodes = int(re.findall(r'EPISODE QUIET route=\d+ episodes=(\d+)', text)[-1])
    process_chain('ready-settlement', checkpoint_route, ','.join(['pending-first-run-block'] * episodes), seed,
                  'W', 'attack', settlement, 'attack')
    native = build / 'mmodern.exe'
    def launch(save):
        return f'& "{native}" --load-game "{game}" "{save}"\n'
    (output / 'native-sdl-acceptance.txt').write_text(
        'PHYSICAL ACCEPTANCE PENDING: only the maintainer can attest native SDL visual/input behavior.\n'
        'Prepared saves contain genuine production Quiet states; no combat snapshot or debug teleport.\n'
        'PowerShell, process-local dependency path (does not change machine settings):\n'
        '$env:PATH = "C:\\msys64\\ucrt64\\bin;$env:PATH"\n\n'
        '1. Mixed Run feedback and input handoff (required).\n'
        f'& "{native}" --journey-region --combat-seed 24 "{game}" --save-file "{output / "physical.mmsave"}"\n'
        'Press Up, F, Up, allowing each automatic animation/boundary to settle. At named ready turns, press R seven times, separately.\n'
        'The fourth attempt (Zippo) fails; the other six succeed. Judge named-member feedback and stable escaped portraits.\n'
        'Judge relocation to (10,12) West and surviving threats. Holding R/Space/F9 across handoff must not trigger an unintended action.\n'
        'Fresh arrows/combat input must respond. F9 must distinguish unavailable versus saved.\n\n'
        '2. Wounded survivor restart and re-engagement (required).\n' + launch(wounded) +
        'Press Left, Up, Right, Up, settling each boundary. Recontact is actor 9 at (9,11), still HP16.\n'
        'Judge responsive controls and correct actor/effect identity. Attack with Space or Run with R at the named ready turn.\n\n'
        '3. Dormant original item (required inspection; optional delivery interaction).\n' + launch(dormant) +
        'Judge clear dormant-item ownership with forfeited gold absent. For delivery, press Right, Up, Up, Up, Up.\n'
        'In combat select the lowest original actor ID with keys 1-3 and press Space at each named ready turn; acknowledge receipts with Space.\n'
        'Judge actual recipient/item text and clear loss/delivery wording; no manual arithmetic or save-byte comparison is required.\n\n'
        '4. Abandoned casualties and ready treasure (required).\n' + launch(ready_save) +
        'Judge the five Dead members versus the surviving member and ready +10 gold. Press Right, then acknowledge the receipt with Space.\n'
        'Judge clear settlement without any suggestion that casualties healed or forfeited gold returned.\n\n'
        'Optional separate casualty-only inspection:\n' + launch(casualty_save) +
        'Outside combat use arrow Right to turn; letter R is Run only in contract-5 combat. Period is Wait; Escape/window close exits.\n')
    (output / 'acceptance-summary.json').write_text(json.dumps({
        'automated_original_process_result': 'PASS', 'physical_acceptance': 'PENDING',
        'process_log': str(index.name), 'recipes': recipes,
        'comparisons': ['all semantic snapshot fields', 'exact on-disk save bytes including header and CRC',
                        'literal draw and publication transcripts', 'second restart with further combat/death or settlement',
                        'independent gold/source conservation and exact item delivery bytes'],
    }, indent=2) + '\n')
    print('All M34 bounded genuine gameplay and complete fresh-process comparisons passed', flush=True)

if m34:
    lifecycle()
else:
    legacy()
