"""M44 evidence orchestration. No save-layout knowledge belongs in this module."""
import argparse
import difflib
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
from datetime import datetime, timezone

REPO = Path(__file__).resolve().parents[1]
EXECUTABLES = ('mmodern_m44_baseline_witness.exe', 'mmodern_m43_cli_witness.exe',
               'mmodern_m44_baseline_restart.exe', 'mmodern_save_dump.exe')
SCENARIOS = (('mainland', 'mainland-combat-myra-phirna', 3626689381, 'mainland.mmsave'),
             ('services', 'vertigo-buy-repair-training', 7, 'services-paid.mmsave'),
             ('temple', 'temple-recovery', 3626689381, 'temple.mmsave'),
             ('roundtrip', 'vertigo-f3-roundtrip', 3626689381, 'roundtrip.mmsave'))
# Checkpoints a scenario also resumes from, each in its own resume-<label>
# directory so the checkpoint glob below sees only the uninterrupted run.
RESUMES = {'roundtrip': ('A', 'B')}


def sha(data):
    return hashlib.sha256(data).hexdigest()


def write_json(path, value):
    temporary = path.with_suffix(path.suffix + '.tmp')
    temporary.write_text(json.dumps(value, indent=2) + '\n', encoding='utf-8')
    temporary.replace(path)


def git(*args):
    return subprocess.check_output(['git', '-C', str(REPO), *args], text=True).strip()


def provenance():
    return dict(commit=git('rev-parse', 'HEAD'), branch=git('branch', '--show-current'),
                origin_main=git('rev-parse', 'origin/main'), status=git('status', '--short'),
                tracked_diff_sha256=sha(subprocess.check_output(['git', '-C', str(REPO), 'diff', 'HEAD'])))


def fresh(path):
    # Never reuse directories: stale checkpoints must not masquerade as output.
    path.mkdir(parents=True, exist_ok=False)


def outside_repository(path):
    if path == REPO or REPO in path.parents:
        raise ValueError('PreservedDir must be outside the repository')


def clean_environment(binary_dir):
    env = {k: v for k, v in os.environ.items() if not k.upper().startswith(('MMODERN_', 'SDL_'))}
    windows = Path(os.environ['SystemRoot'])
    env.update(PATH=os.pathsep.join(map(str, (binary_dir, windows / 'System32', windows))),
               SDL_VIDEODRIVER='dummy', SDL_RENDER_DRIVER='software')
    return env


def run(args, log, env, cwd):
    with log.open('wb') as stream:
        result = subprocess.run(list(map(str, args)), cwd=cwd, env=env, stdout=stream,
                                stderr=subprocess.STDOUT, timeout=240)
    if result.returncode:
        raise RuntimeError(f'{args[0]} exited {result.returncode}; see {log}')


def bundle(build, destination, objdump):
    """Recursively resolve PE imports; OS DLLs remain supplied by Windows."""
    fresh(destination)
    search = [build] + [Path(p) for p in os.environ.get('PATH', '').split(os.pathsep) if p]
    system = Path(os.environ['SystemRoot']) / 'System32'
    queue = [build / name for name in EXECUTABLES]
    manifest = {}
    system_imports = set()
    while queue:
        source = queue.pop(0).resolve(strict=True)
        key = source.name.lower()
        digest = sha(source.read_bytes())
        if key in manifest:
            if manifest[key]['sha256'] != digest:
                raise RuntimeError(f'Conflicting DLLs: {source}')
            continue
        shutil.copy2(source, destination / source.name)
        manifest[key] = dict(file=source.name, source=str(source), sha256=digest)
        imports = subprocess.check_output([objdump, '-p', str(source)], text=True)
        for name in re.findall(r'DLL Name:\s*(\S+)', imports):
            if name.lower().startswith(('api-ms-win-', 'ext-ms-win-')):
                system_imports.add(name)
                continue
            # Executable-local DLLs take precedence over the OS and PATH.
            candidates = [build / name, system / name] + [p / name for p in search[1:]]
            dependency = next((p for p in candidates if p.is_file()), None)
            if dependency is None:
                raise FileNotFoundError(f'Unresolved runtime dependency {name} of {source}')
            if dependency.parent == system:
                system_imports.add(name)
            else:
                queue.append(dependency)
    return dict(files=manifest, windows_imports=sorted(system_imports))


def verify_bundle(directory, manifest):
    for entry in manifest['files'].values():
        if sha((directory / entry['file']).read_bytes()) != entry['sha256']:
            raise RuntimeError(f'Preserved binary changed: {entry["file"]}')


def expected_digests():
    """Recorded digests by label; None marks a scenario whose digest awaits approval."""
    header = (REPO / 'tests/XeenM44BaselineDigests.h').read_text(encoding='utf-8-sig')
    rows = re.findall(r'\{"([^"]+)",\s*(\d+)u,\s*"([0-9a-f]{64})"\}', header)
    result = {}
    for label, key, seed, _ in SCENARIOS:
        matches = [digest for name, value, digest in rows if name == key and int(value) == seed]
        if len(matches) > 1 or (not matches and any(name == key for name, _, _ in rows)):
            raise ValueError(f'Missing or ambiguous baseline/seed for {label}')
        result[label] = matches[0] if matches else None
    if not any(result.values()):
        raise ValueError('No recorded baseline digest')
    return result


def read_dump(path):
    result = {}
    for line in path.read_text(encoding='ascii').splitlines():
        key, value = line.split('\t')
        if key in result:
            raise ValueError(f'Duplicate decoded field {key}')
        result[key] = value
    if not any(k.startswith('snapshot.') for k in result):
        raise ValueError('Empty decoded snapshot')
    return result


def namespace(fields, prefix):
    return {k: v for k, v in fields.items() if k.startswith(prefix)}


def per_map_comparison(a, b):
    # The per-map view (view.perMap) keeps fields comparable across layouts.
    # A build that predates it cannot be compared there; that is not a difference.
    have = ('view.perMap' in a, 'view.perMap' in b)
    if not all(have):
        return dict(unavailable=[side for side, present in zip(('old', 'new'), have) if not present])
    return field_comparison(namespace(a, 'maps['), namespace(b, 'maps['))


def resume(binary_dir, game, directory, name, filename, label, final):
    """Resume from an uninterrupted checkpoint; its final bytes must be identical."""
    target = directory / f'resume-{label}'
    fresh(target)
    shutil.copy2(directory / f'{filename}-{label}.mmsave', target / filename)
    env = clean_environment(binary_dir)
    env.update(MMODERN_M44_SCENARIO=name, MMODERN_M44_STAGE=label, MMODERN_M49_TRACE=str(target / 'trace.txt'))
    run([binary_dir / EXECUTABLES[0], '--load-game', game, target / filename], target / 'generate.log', env, target)
    resumed = (target / (filename + '-final.mmsave')).read_bytes()
    if resumed != final:
        raise RuntimeError(f'{name}: final resumed from {label} differs from the uninterrupted final')
    return dict(checkpoint=label, sha256=sha(resumed), byte_equal=True)


def generate(binary_dir, game, output, expected=None, names=None):
    fresh(output)
    results = []
    for name, _, seed, filename in SCENARIOS:
        if names is not None and name not in names:
            continue
        directory = output / name
        fresh(directory)
        env = clean_environment(binary_dir)
        env['MMODERN_M49_TRACE'] = str(directory / 'trace.txt')
        if name != 'temple':
            env['MMODERN_M44_SCENARIO'] = name
        executable = EXECUTABLES[1 if name == 'temple' else 0]
        print(f'Running {name}: {binary_dir}', flush=True)
        run([binary_dir / executable, '--journey-region', '--combat-seed', seed, game,
             '--save-file', directory / filename], directory / 'generate.log', env, directory)
        final = directory / (filename if name == 'temple' else filename + '-final.mmsave')
        digest = sha(final.read_bytes())
        if expected and expected.get(name) and digest != expected[name]:
            raise RuntimeError(f'{name}: preserved hash {digest} != baseline {expected[name]}')
        trace = (directory / 'trace.txt').read_bytes()
        if not trace:
            raise RuntimeError(f'{name}: empty trace')
        checkpoints = sorted(directory.glob('*.mmsave'))
        for checkpoint in checkpoints:
            log = checkpoint.with_suffix(checkpoint.suffix + '.fields.tsv')
            run([binary_dir / EXECUTABLES[3], checkpoint], log, clean_environment(binary_dir), directory)
            read_dump(log)
        run([binary_dir / EXECUTABLES[2], binary_dir / EXECUTABLES[0], game, name, final],
            directory / 'restart.log', clean_environment(binary_dir), directory)
        if 'Baseline restart and final bytes exact' not in (directory / 'restart.log').read_text():
            raise RuntimeError(f'{name}: missing reload/re-save confirmation')
        resumes = [resume(binary_dir, game, directory, name, filename, label, final.read_bytes())
                   for label in RESUMES.get(name, ())]
        results.append(dict(name=name, seed=seed, final=final.name, sha256=digest,
                            recorded=bool(expected and expected.get(name)),
                            trace_lines=len(trace.splitlines()), trace_sha256=sha(trace),
                            checkpoints=[p.name for p in checkpoints], native_reload_resave_exact=True,
                            resumes=resumes))
    return results


def byte_comparison(a, b):
    # Sequence alignment is evidence, not a semantic explanation. Repeated bytes
    # can admit several equally valid alignments. Report all half-open ranges.
    matcher = difflib.SequenceMatcher(None, a, b, autojunk=False)
    ranges = [dict(kind=tag, old=[i,j], new=[k,l])
              for tag,i,j,k,l in matcher.get_opcodes() if tag != 'equal'] if a != b else []
    first = next((i for i in range(min(len(a),len(b))) if a[i] != b[i]),
                 min(len(a),len(b)) if len(a) != len(b) else None)
    return dict(equal=a == b, first_difference=first, ranges=ranges,
                old_sha256=sha(a), new_sha256=sha(b), old_length=len(a), new_length=len(b))


def field_comparison(a, b):
    common = sorted(a.keys() & b.keys())
    return dict(common_count=len(common), changed=[dict(field=k, old=a[k], new=b[k])
                for k in common if a[k] != b[k]],
                removed={k:a[k] for k in sorted(a.keys()-b.keys())},
                added={k:b[k] for k in sorted(b.keys()-a.keys())})


def compare_runs(old, new, old_results, new_results):
    result = []
    candidates = {b['name']: b for b in new_results}
    for a in old_results:
        name = a['name']
        b = candidates[name]
        old_dir, new_dir = old / name, new / name
        x, y = (old_dir / 'trace.txt').read_bytes(), (new_dir / 'trace.txt').read_bytes()
        lines_x, lines_y = x.splitlines(), y.splitlines()
        differences = [dict(line=i+1, old=lines_x[i].decode('utf-8', 'backslashreplace') if i < len(lines_x) else None,
                            new=lines_y[i].decode('utf-8', 'backslashreplace') if i < len(lines_y) else None)
                       for i in range(max(len(lines_x),len(lines_y)))
                       if (lines_x[i] if i < len(lines_x) else None) != (lines_y[i] if i < len(lines_y) else None)]
        trace = dict(byte_equal=x == y, old_lines=len(lines_x), new_lines=len(lines_y),
                     old_sha256=sha(x), new_sha256=sha(y), line_differences=differences)
        checkpoints = []
        for filename in sorted(set(a['checkpoints']) | set(b['checkpoints'])):
            if filename not in a['checkpoints'] or filename not in b['checkpoints']:
                checkpoints.append(dict(file=filename, missing_from='old' if filename not in a['checkpoints'] else 'new'))
                continue
            p, q = old_dir / filename, new_dir / filename
            u, v = read_dump(p.with_suffix(p.suffix+'.fields.tsv')), read_dump(q.with_suffix(q.suffix+'.fields.tsv'))
            checkpoints.append(dict(file=filename, bytes=byte_comparison(p.read_bytes(),q.read_bytes()),
                                    old_envelope=namespace(u, 'envelope.'), new_envelope=namespace(v, 'envelope.'),
                                    decoded=field_comparison(namespace(u, 'snapshot.'), namespace(v, 'snapshot.')),
                                    per_map=per_map_comparison(u, v)))
        result.append(dict(name=name, old_sha256=a['sha256'], candidate_sha256=b['sha256'],
                           trace=trace, checkpoints=checkpoints, native_reload_resave_exact=b['native_reload_resave_exact']))
    return result


def has_differences(scenarios):
    for s in scenarios:
        if not s['trace']['byte_equal']:
            return True
        for c in s['checkpoints']:
            if 'missing_from' in c or not c['bytes']['equal']:
                return True
            if any(c['decoded'][key] for key in ('changed', 'added', 'removed')):
                return True
            if 'unavailable' not in c.get('per_map', {'unavailable': True}) and \
                    any(c['per_map'][key] for key in ('changed', 'added', 'removed')):
                return True
    return False


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('step', choices=('Preserve', 'Compare'))
    for name in ('build-dir', 'preserved-dir', 'game-dir'):
        parser.add_argument('--'+name, type=Path, required=True)
    parser.add_argument('--output-dir', type=Path)
    parser.add_argument('--objdump', default='objdump')
    args = parser.parse_args()
    build, preserved, game = (p.resolve() for p in (args.build_dir,args.preserved_dir,args.game_dir))
    outside_repository(preserved)
    output = preserved if args.step == 'Preserve' else (args.output_dir.resolve() if args.output_dir else None)
    if output is None:
        parser.error('Compare requires --output-dir (a new directory)')
    fresh(output)
    report = dict(report_schema=1, step=args.step, started_utc=datetime.now(timezone.utc).isoformat(),
                  build=str(build), preserved=str(preserved), game=str(game), provenance=provenance(), status='running')
    try:
        if args.step == 'Preserve':
            report['expected'] = expected_digests()
            report['bundle'] = bundle(build, preserved / 'bin', args.objdump)
            report['scenarios'] = generate(preserved / 'bin', game, preserved / 'validation', report['expected'])
            report['status'] = 'baseline_verified'
        else:
            baseline = json.loads((preserved / 'digest-audit.json').read_text())
            if baseline['status'] != 'baseline_verified':
                raise ValueError('Preserve did not complete successfully')
            report['baseline_manifest_sha256'] = sha((preserved / 'digest-audit.json').read_bytes())
            report['expected'] = baseline['expected']
            verify_bundle(preserved / 'bin', baseline['bundle'])
            # Copy the candidate too: run with only bundled dependencies on PATH.
            report['bundle'] = bundle(build, output / 'candidate-bin', args.objdump)
            # The preserved build runs only the scenarios it was preserved with.
            preserved_names = [s['name'] for s in baseline['scenarios']]
            old = generate(preserved / 'bin', game, output / 'old', baseline['expected'], preserved_names)
            new = generate(output / 'candidate-bin', game, output / 'new')
            report['scenarios'] = compare_runs(output / 'old', output / 'new', old, new)
            # A scenario without a preserved run has no comparison; its digest
            # is a candidate fresh baseline for the maintainer to approve.
            report['new_scenarios'] = [b for b in new if b['name'] not in preserved_names]
            report['status'] = 'differences_require_review' if has_differences(report['scenarios']) else 'zero_differences'
    except Exception as error:
        report['status'] = 'error'
        report['error'] = str(error)
    report['finished_utc'] = datetime.now(timezone.utc).isoformat()
    write_json(output / 'digest-audit.json', report)
    summary = [f'M44 digest audit: {report["status"]}', f'Report: {output / "digest-audit.json"}']
    if 'error' in report:
        summary.append(report['error'])
    for scenario in report.get('scenarios', []):
        summary.append(f'{scenario["name"]}: {scenario.get("candidate_sha256", scenario.get("sha256"))}')
        for item in scenario.get('resumes', []):
            summary.append(f'  resumed from {item["checkpoint"]}: final byte-identical {item["sha256"]}')
        if 'trace' in scenario:
            checkpoints = scenario['checkpoints']
            changed_saves = sum('missing_from' in c or not c['bytes']['equal'] for c in checkpoints)
            fields = [c['decoded'] for c in checkpoints if 'decoded' in c]
            changed_fields = sum(len(c['changed']) for c in fields)
            added_fields = sum(len(c['added']) for c in fields)
            removed_fields = sum(len(c['removed']) for c in fields)
            summary.append(f'  trace equal: {scenario["trace"]["byte_equal"]}; checkpoints: {len(checkpoints)}; changed/missing saves: {changed_saves}')
            summary.append(f'  decoded fields changed/added/removed: {changed_fields}/{added_fields}/{removed_fields}; reload/re-save exact: {scenario["native_reload_resave_exact"]}')
            maps = [c['per_map'] for c in checkpoints if 'per_map' in c]
            compared = [m for m in maps if 'unavailable' not in m]
            if compared:
                summary.append('  per-map fields changed/added/removed: ' + '/'.join(
                    str(sum(len(m[key]) for m in compared)) for key in ('changed', 'added', 'removed')))
            elif maps:
                summary.append('  per-map view: not comparable (absent from the ' +
                               ' and '.join(sorted({s for m in maps for s in m['unavailable']})) + ' build)')
    for scenario in report.get('new_scenarios', []):
        summary.append(f'{scenario["name"]}: new scenario without a preserved run; candidate digest {scenario["sha256"]}')
        summary.append(f'  checkpoints: {len(scenario["checkpoints"])}; trace lines: {scenario["trace_lines"]}; '
                       f'reload/re-save exact: {scenario["native_reload_resave_exact"]}')
        for item in scenario.get('resumes', []):
            summary.append(f'  resumed from {item["checkpoint"]}: final byte-identical {item["sha256"]}')
    summary.append('Digests are never edited. Differences require maintainer review; byte alignment does not explain them.')
    (output / 'summary.txt').write_text('\n'.join(summary)+'\n', encoding='utf-8')
    print('\n'.join(summary), flush=True)
    return 1 if report['status'] == 'error' else 2 if report['status'] == 'differences_require_review' else 0


if __name__ == '__main__':
    sys.exit(main())
