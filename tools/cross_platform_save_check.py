"""Cross-platform save check of M113: saves written by the Windows build load
on the Linux build and the other way round.

Run on Windows with the Linux build in a WSL clone on ext4 (see
docs/dependencies.md, "Linux build under WSL"). Each build writes the same
saves: the four M44 gameplay scenarios (the witness inputs and seeds of
tools/m44_digest_audit.py) and a few developer presets. Then every save is
dumped with both builds' mmodern_save_dump, which must print the same
fields, and reloaded by the other build through mmodern_m44_baseline_restart,
whose F9 re-save must be byte-identical. The witness restarts quiet Clouds
Journey saves only; each other preset must be refused by both builds with the
same expected reason, taken from the witness's own log. The bytes each build
writes for the same save are compared too. Every restart runs in its own
folder, reloaded/<origin>-<save>[-own]/, which keeps its logs.
Exit 0: every check passed; 1: a check failed.

The saves come from the original data, so the evidence stays local: the
output directories must be new directories inside the two build trees.

Build first, on both: mmodern_save_dump mmodern_m44_baseline_witness
mmodern_m43_cli_witness mmodern_m44_baseline_restart mmodern_test_save.

Example:
  python -B tools/cross_platform_save_check.py --windows-build build-rel \\
    --game "F:/Games/gog/Might and Magic 4-5" --linux-repo /home/carbo/mmodern-linux \\
    --linux-build build-linux-rel --linux-game /home/carbo/m113-measure/data
"""
import argparse
import datetime
import hashlib
import json
import os
import re
import shlex
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

from m44_digest_audit import SCENARIOS, clean_environment

# A preset the M44 witness can reload and re-save: a quiet Clouds Journey.
PRESETS = ('m78-playtest',)
# Presets on Darkside or on an Event cell: the M44 witness cannot finish its
# scripted restart there, so they get the dump and byte checks, and both
# builds must refuse their restart with EXPECTED_REFUSAL.
DUMP_ONLY_PRESETS = ('m103-enter-vertigo', 'm103-dragon-cave', 'm103-dwarf-mine',
                     'ellinger-speech', 'm101-castleview', 'm102-playtest', 'm103-fall')
EXPECTED_REFUSAL = ('witness exit 3: Presentation update failed: M44 exact restart before input | '
                    'Gameplay startup failed: M43 witnessed actions did not complete')
# The M44 witness's scenarios, passed to the restart of their own saves (the
# roundtrip scenario overrides the presentation deadline). The temple save
# (M43 witness) and the presets have none there: they use its default.
WITNESS_SCENARIOS = ('mainland', 'services', 'roundtrip')
DEFAULT_SCENARIO = 'mainland'
TIMEOUT = 300


def sha(data):
    return hashlib.sha256(data).hexdigest()


def windows_to_wsl(path):
    """D:/x/y -> /mnt/d/x/y for a path on a Windows drive."""
    path = Path(path).resolve()
    drive = path.drive.rstrip(':').lower()
    return '/mnt/' + drive + '/' + '/'.join(path.parts[1:])


class Windows:
    name = 'windows'

    def __init__(self, build, game, out, toolchain):
        self.build, self.game, self.out = Path(build).resolve(), game, Path(out)
        self.toolchain = toolchain

    def exe(self, target):
        return self.build / (target + '.exe')

    def run(self, args, log, extra=None, cwd=None):
        env = clean_environment(self.build)
        # build-rel uses the UCRT64 runtime DLLs where they are installed.
        env['PATH'] = os.pathsep.join((str(self.build), self.toolchain, env['PATH']))
        env.update({k: str(v) for k, v in (extra or {}).items()})
        with open(log, 'wb') as stream:
            return subprocess.run([str(a) for a in args], cwd=cwd or self.out, env=env, stdout=stream,
                                  stderr=subprocess.STDOUT, timeout=TIMEOUT).returncode

    def path(self, *parts):
        return str(self.out.joinpath(*parts))

    def mkdir(self, directory):
        Path(directory).mkdir(parents=True, exist_ok=True)

    def read(self, path):
        return Path(path).read_bytes()

    def remove(self, directory):
        shutil.rmtree(directory)

    def glob(self, directory, pattern):
        return sorted(str(p) for p in Path(directory).glob(pattern))

    def temporary(self):
        return Path(tempfile.mkdtemp(prefix='mmodern-xplat-')).as_posix()

    def copy_in(self, source_side, source, target):
        data = source_side.read(source)
        Path(target).parent.mkdir(parents=True, exist_ok=True)
        Path(target).write_bytes(data)


class Linux:
    name = 'linux'

    def __init__(self, repo, build, game, out):
        self.build = f'{repo}/{build}' if not build.startswith('/') else build
        self.game, self.out = game, out

    def exe(self, target):
        return f'{self.build}/{target}'

    def shell(self, command, timeout=TIMEOUT):
        return subprocess.run(['wsl.exe', '-e', 'bash', '-c', command], stdout=subprocess.PIPE,
                              stderr=subprocess.STDOUT, timeout=timeout)

    def run(self, args, log, extra=None, cwd=None):
        env = {'PATH': '/usr/bin:/bin', 'HOME': self.out, 'XDG_DATA_HOME': f'{self.out}/xdg',
               'SDL_VIDEODRIVER': 'dummy', 'SDL_RENDER_DRIVER': 'software', 'SDL_AUDIODRIVER': 'dummy'}
        env.update(extra or {})
        command = ('cd ' + shlex.quote(cwd or self.out) + ' && exec env -i ' +
                   ' '.join(shlex.quote(f'{k}={v}') for k, v in env.items()) + ' ' +
                   ' '.join(shlex.quote(str(a)) for a in args) + ' > ' + shlex.quote(str(log)) + ' 2>&1')
        return self.shell(command).returncode

    def path(self, *parts):
        return '/'.join((self.out,) + parts)

    def mkdir(self, directory):
        result = self.shell('mkdir -p ' + shlex.quote(directory))
        if result.returncode:
            raise RuntimeError(f'mkdir {directory}: {result.stdout.decode(errors="replace")}')

    def read(self, path):
        result = subprocess.run(['wsl.exe', '-e', 'cat', path], stdout=subprocess.PIPE, timeout=TIMEOUT)
        if result.returncode:
            raise RuntimeError(f'Cannot read {path}')
        return result.stdout

    def remove(self, directory):
        self.shell('rm -rf ' + shlex.quote(directory))

    def glob(self, directory, pattern):
        result = self.shell('cd ' + shlex.quote(directory) + ' && for f in ' + pattern +
                            '; do [ -e "$f" ] && echo "$PWD/$f"; done; true')
        return sorted(line for line in result.stdout.decode(errors='replace').splitlines() if line)

    def temporary(self):
        result = self.shell('mktemp -d /tmp/mmodern-xplat-XXXXXX')
        if result.returncode:
            raise RuntimeError('mktemp failed')
        return result.stdout.decode().strip()

    def copy_in(self, source_side, source, target):
        source_path = windows_to_wsl(source) if isinstance(source_side, Windows) else source
        result = self.shell('mkdir -p ' + shlex.quote(target.rsplit('/', 1)[0]) + ' && cp ' +
                            shlex.quote(source_path) + ' ' + shlex.quote(target))
        if result.returncode:
            raise RuntimeError(f'copy {source} -> {target}: {result.stdout.decode(errors="replace")}')


def generate(side):
    """Writes the scenario finals and presets; returns {label: path}."""
    saves = {}
    for name, _, seed, filename in SCENARIOS:
        directory = side.path('written', name)
        side.mkdir(directory)
        extra = {'MMODERN_M49_TRACE': side.path('written', name, 'trace.txt')}
        if name != 'temple':
            extra['MMODERN_M44_SCENARIO'] = name
        target = 'mmodern_m43_cli_witness' if name == 'temple' else 'mmodern_m44_baseline_witness'
        save = side.path('written', name, filename)
        code = side.run([side.exe(target), '--journey-region', '--combat-seed', seed, side.game, '--save-file', save],
                        side.path('written', name, 'generate.log'), extra, directory)
        if code:
            raise RuntimeError(f'{side.name}: scenario {name} failed ({code})')
        saves[name] = save if name == 'temple' else save + '-final.mmsave'
    side.mkdir(side.path('written', 'presets'))
    # mmodern_test_save writes only outside the repository, so each preset is
    # written to a temporary folder first and then kept with the evidence.
    scratch = side.temporary()
    for preset in PRESETS + DUMP_ONLY_PRESETS:
        save = side.path('written', 'presets', preset + '.mmsave')
        written = scratch + '/' + preset + '.mmsave'
        code = side.run([side.exe('mmodern_test_save'), preset, side.game, written],
                        side.path('written', 'presets', preset + '.log'))
        if code:
            raise RuntimeError(f'{side.name}: preset {preset} failed ({code})')
        side.copy_in(side, written, save)
        saves[preset] = save
    side.remove(scratch)
    return saves


def dump(side, save, log):
    code = side.run([side.exe('mmodern_save_dump'), save], log)
    if code:
        raise RuntimeError(f'{side.name}: mmodern_save_dump {save} failed ({code})')
    # The Windows dump writes text-mode CRLF line ends; the fields are compared.
    return side.read(log).replace(b'\r\n', b'\n')


def refusal(text):
    """The failure lines of a witness log, without bracketed paths."""
    lines = [re.sub(r'\s*\[[^\]]*\]', '', line).strip() for line in text.splitlines()
             if re.search(r'fail|error|exception|abort|assert|terminate', line, re.I)]
    return ' | '.join(lines) or '(no failure line)'


def restart(side, folder, save, scenario):
    """Loads a save on this build through the M44 witness in its own folder,
    presses F9 and requires identical bytes. Returns (exact, outcome): the
    outcome is 'exact', or the witness's exit code and failure lines."""
    log = side.path('reloaded', folder, 'restart-wrapper.log')
    code = side.run([side.exe('mmodern_m44_baseline_restart'), side.exe('mmodern_m44_baseline_witness'),
                     side.game, scenario, save], log, cwd=side.path('reloaded', folder))
    wrapper = side.read(log).decode(errors='replace')
    if code == 0 and 'Baseline restart and final bytes exact' in wrapper:
        return True, 'exact'
    children = side.glob(side.path('reloaded', folder), 'm44-restart-*/restart.log')
    last = (wrapper.strip().splitlines() or [''])[-1]
    if len(children) != 1:
        return False, f'restart exit {code}, {len(children)} witness logs: {last}'
    exit_code = re.search(r'^PID \d+ exit (-?\d+)', wrapper, re.M)
    return False, (f'witness exit {exit_code.group(1) if exit_code else "?"}: '
                   + refusal(side.read(children[0]).decode(errors='replace')))


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('--windows-build', required=True)
    parser.add_argument('--game', required=True, help='Windows path of the game data')
    parser.add_argument('--linux-repo', required=True, help='WSL path of the Linux clone')
    parser.add_argument('--linux-build', default='build-linux-rel')
    parser.add_argument('--linux-game', required=True, help='WSL path of the game data on ext4')
    parser.add_argument('--windows-toolchain-bin', default='C:/msys64/ucrt64/bin')
    args = parser.parse_args()
    # Git Bash rewrites /home/... arguments into Windows paths; refuse them.
    for name in ('linux_repo', 'linux_game'):
        value = getattr(args, name)
        if not value.startswith('/') or ':' in value:
            parser.error(f'--{name.replace("_", "-")} must be an absolute WSL path, not {value!r} '
                         '(from Git Bash, set MSYS_NO_PATHCONV=1)')

    stamp = datetime.datetime.now().strftime('%Y%m%d-%H%M%S')
    win_out = Path(args.windows_build).resolve() / 'xplat-save-check' / stamp
    win_out.mkdir(parents=True)
    linux = Linux(args.linux_repo, args.linux_build, args.linux_game, '')
    linux.out = f'{linux.build}/xplat-save-check/{stamp}'
    linux.mkdir(linux.out)
    linux.mkdir(linux.path('xdg'))
    windows = Windows(args.windows_build, args.game, win_out, args.windows_toolchain_bin)
    sides = {'windows': windows, 'linux': linux}

    report = dict(windows_output=str(win_out), linux_output=linux.out, saves=[])
    written = {name: generate(side) for name, side in sides.items()}
    failures = 0
    for side in sides.values():
        side.mkdir(side.path('reloaded'))
        side.mkdir(side.path('dumps'))
    for origin, other in (('windows', 'linux'), ('linux', 'windows')):
        src, dst = sides[origin], sides[other]
        for label, save in written[origin].items():
            tag = f'{origin}-{label}'
            scenario = label if label in WITNESS_SCENARIOS else DEFAULT_SCENARIO
            data = src.read(save)
            dst.mkdir(dst.path('reloaded', tag))
            copy = dst.path('reloaded', tag, 'save.mmsave')
            dst.copy_in(src, save, copy)
            own = dump(src, save, src.path('dumps', tag + f'.{origin}.tsv'))
            foreign = dump(dst, copy, dst.path('dumps', tag + f'.{other}.tsv'))
            resaved, outcome = restart(dst, tag, copy, scenario)
            entry = dict(save=tag, sha256=sha(data), size=len(data), dump_equal=own == foreign,
                         dump_lines=own.count(b'\n'), scenario=scenario, resave_on=other,
                         resave_exact=resaved, restart=outcome)
            if label in DUMP_ONLY_PRESETS:
                src.mkdir(src.path('reloaded', tag + '-own'))
                own_copy = src.path('reloaded', tag + '-own', 'save.mmsave')
                src.copy_in(src, save, own_copy)
                _, own_outcome = restart(src, tag + '-own', own_copy, scenario)
                entry.update(dump_only=True, own_restart=own_outcome)
                resave_failed = not (outcome == own_outcome == EXPECTED_REFUSAL)
                resave_text = ('refused with the expected reason on both' if not resave_failed else
                               f'refused DIFFERENTLY ({origin}: {own_outcome!r}; {other}: {outcome!r})')
            else:
                resave_failed = not resaved
                resave_text = f"re-save on {other} {'exact' if resaved else 'FAILED: ' + outcome}"
            if origin == 'windows':
                twin = sides['linux'].read(written['linux'][label])
                entry['same_bytes_as_linux_write'] = twin == data
            failures += (not entry['dump_equal']) + resave_failed + \
                (entry.get('same_bytes_as_linux_write') is False)
            report['saves'].append(entry)
            print(f"{tag}: {len(data)} bytes, dump {'equal' if entry['dump_equal'] else 'DIFFERS'} "
                  f"({entry['dump_lines']} lines), {resave_text}"
                  + ('' if origin != 'windows' else
                     f", Linux wrote {'the same bytes' if entry['same_bytes_as_linux_write'] else 'DIFFERENT bytes'}"),
                  flush=True)
    report['failures'] = failures
    (win_out / 'report.json').write_text(json.dumps(report, indent=2))
    print(f'{len(report["saves"])} saves checked, {failures} failed checks. Report: {win_out / "report.json"}')
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
