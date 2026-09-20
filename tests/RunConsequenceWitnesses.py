"""Opt-in original-resource/process acceptance. Never edits game data or injects state.
Usage: python tests/RunConsequenceWitnesses.py BUILD_DIRECTORY INSTALLATION
All saves, literal draw transcripts and process output go to BUILD_DIRECTORY/m33-evidence.
"""
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

build, game = map(lambda x: Path(x).resolve(), sys.argv[1:3])
output = build / "m33-evidence"
output.mkdir(exist_ok=True)
exe = build / "mmodern_consequence_cli_witness.exe"
env = os.environ.copy()
for key in ("MMODERN_M33_EXPECT", "MMODERN_M33_ORACLE", "MMODERN_M33_ROUTE", "MMODERN_M33_FAULT", "MMODERN_M33_FAIL_DRAW"):
    env.pop(key, None)
env["MMODERN_M33_SUMMARY"] = "1"
index = (output / "processes.log").open("w")

def run(name, route, seed=3, source=None, expected=None, oracle=None, failure=False, fault=None, fail_draw=None):
    save = output / (name + ".mmsave")
    local = env.copy()
    local["MMODERN_M33_ROUTE"] = route
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
    index.write(f"{name} PID={child.pid} exit={code} seed={seed} route={route} source={source} expected={expected} args={args}\n")
    index.flush()
    if fail_draw:
        assert code == 4 and f"ARTIFICIAL RAW-DRAW FAILURE ATOMICITY PASS {fail_draw}" in text, text[-1500:]
        return save, text, code
    if code and not failure:
        raise RuntimeError(f"{name} failed: {text[-1500:]}")
    if code and ("TERMINAL phase=" not in text or "Combat terminal witness" not in text):
        raise RuntimeError(f"{name}: unexpected integration failure: {text[-1500:]}")
    if not code:
        assert "M33 GAMEPLAY PASS" in text
        if source:
            assert "FRESH PROCESS FULL STATE AND BYTES PASS" in text
        if expected:
            assert "CONTROL FULL SEMANTIC STATE AND BYTES PASS" in text
    return save, text, code

def branch(name, prefix, suffix, seed=3, oracle=None):
    checkpoint, _, _ = run(name + "-checkpoint", prefix, seed)
    control, _, _ = run(name + "-control", prefix + suffix, seed, oracle=oracle)
    resumed, _, _ = run(name + "-resume", suffix, seed, source=checkpoint, expected=control, oracle=oracle)
    return resumed

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
