"""Synthetic audit regression tests; no game resources or saves are retained."""
import contextlib
import io
import json
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch
import m44_digest_audit as audit


class AuditTests(unittest.TestCase):
    def test_byte_ranges_reconstruct_candidate(self):
        for old, new in [(b'abc', b'abc'), (b'abcd', b'abXYcd'),
                         (b'abcde', b'abe'), (b'abc', b'aXc'),
                         (b'', b'new'), (b'old', b''),
                         (b'\0'*80+b'A'+b'\0'*20, b'\0'*80+b'BCD'+b'\0'*20)]:
            with self.subTest(old=old, new=new):
                comparison = audit.byte_comparison(old, new)
                rebuilt = bytearray(old)
                for change in reversed(comparison['ranges']):
                    i,j = change['old']; k,l = change['new']
                    rebuilt[i:j] = new[k:l]
                self.assertEqual(bytes(rebuilt), new)
                self.assertEqual(comparison['equal'], old == new)
                self.assertEqual(comparison['first_difference'] is None, old == new)

    def test_fields_include_changes_additions_and_removals(self):
        result = audit.field_comparison({'same':'1', 'changed':'2', 'gone':'3'},
                                        {'same':'1', 'changed':'9', 'new':'4'})
        self.assertEqual(result['common_count'], 2)
        self.assertEqual(result['changed'], [{'field':'changed','old':'2','new':'9'}])
        self.assertEqual(result['removed'], {'gone':'3'})
        self.assertEqual(result['added'], {'new':'4'})

    def test_four_scenarios_and_resumes(self):
        self.assertEqual([s[0] for s in audit.SCENARIOS], ['mainland', 'services', 'temple', 'roundtrip'])
        self.assertEqual(audit.RESUMES, {'roundtrip': ('A', 'B')})

    def test_unrecorded_scenario_digest_awaits_approval(self):
        rows = '\n'.join(f'{{"{key}",{seed}u,"{"0"*64}"}},' for _, key, seed, _ in audit.SCENARIOS[:3])
        with patch.object(audit.Path, 'read_text', return_value=rows):
            expected = audit.expected_digests()
        self.assertEqual(expected['roundtrip'], None)
        self.assertEqual(expected['mainland'], '0'*64)
        with patch.object(audit.Path, 'read_text', return_value=''):
            with self.assertRaises(ValueError):
                audit.expected_digests()

    def test_per_map_view_compared_only_when_both_builds_write_it(self):
        old = {'snapshot.food':'1', 'maps[0:23].monsters[0].hp':'5'}
        new = {'snapshot.food':'1', 'view.perMap':'1', 'maps[0:23].monsters[0].hp':'6'}
        self.assertEqual(audit.per_map_comparison(old, new), {'unavailable': ['old']})
        old['view.perMap'] = '1'
        result = audit.per_map_comparison(old, new)
        self.assertEqual(result['changed'], [{'field':'maps[0:23].monsters[0].hp','old':'5','new':'6'}])

    def test_new_scenario_is_reported_not_compared(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for side in ('old','new'):
                (root/side/'mainland').mkdir(parents=True)
                (root/side/'mainland'/'trace.txt').write_bytes(b'one\n')
            old = [dict(name='mainland', sha256='a', checkpoints=[])]
            new = [dict(name='roundtrip', sha256='c', checkpoints=[], native_reload_resave_exact=True),
                   dict(name='mainland', sha256='a', checkpoints=[], native_reload_resave_exact=True)]
            result = audit.compare_runs(root/'old',root/'new',old,new)
            self.assertEqual([s['name'] for s in result], ['mainland'])
            self.assertFalse(audit.has_differences(result))
            result[0]['checkpoints'] = [dict(file='x', bytes=dict(equal=True),
                decoded=dict(changed=[],added={},removed={}),
                per_map=dict(changed=[{'field':'maps[0:28].cells[3].mask'}],added={},removed={}))]
            self.assertTrue(audit.has_differences(result))
            result[0]['checkpoints'][0]['per_map'] = {'unavailable': ['old']}
            self.assertFalse(audit.has_differences(result))

    def test_complete_trace_and_missing_checkpoint(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for side in ('old','new'):
                (root/side/'mainland').mkdir(parents=True)
            (root/'old/mainland/trace.txt').write_bytes(b'one\ntwo\n')
            (root/'new/mainland/trace.txt').write_bytes(b'one\ntwo\nextra\n')
            old = dict(name='mainland', sha256='a', checkpoints=['missing.mmsave'])
            new = dict(name='mainland', sha256='b', checkpoints=[], native_reload_resave_exact=True)
            result = audit.compare_runs(root/'old',root/'new',[old],[new])
            self.assertEqual(result[0]['trace']['line_differences'], [{'line':3,'old':None,'new':'extra'}])
            self.assertEqual(result[0]['checkpoints'][0]['missing_from'], 'new')
            self.assertTrue(audit.has_differences(result))
            (root/'new/mainland/trace.txt').write_bytes(b'one\ntwo\n')
            old['checkpoints'] = []
            self.assertFalse(audit.has_differences(audit.compare_runs(root/'old',root/'new',[old],[new])))

    def test_safety_and_bundle_tampering(self):
        with self.assertRaises(ValueError):
            audit.outside_repository(audit.REPO/'build-rel'/'baseline')
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            with self.assertRaises(FileExistsError):
                audit.fresh(root)
            (root/'binary.exe').write_bytes(b'old')
            manifest = {'files':{'binary.exe':{'file':'binary.exe','sha256':audit.sha(b'old')}}}
            audit.verify_bundle(root,manifest)
            (root/'binary.exe').write_bytes(b'changed')
            with self.assertRaises(RuntimeError):
                audit.verify_bundle(root,manifest)

    def test_failed_preserve_writes_error_report(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory)/'preserved'
            args = ['audit', 'Preserve', '--build-dir', directory,
                    '--preserved-dir', str(output), '--game-dir', directory]
            with patch.object(audit.sys, 'argv', args), \
                 patch.object(audit, 'provenance', return_value={}), \
                 patch.object(audit, 'expected_digests', return_value={}), \
                 patch.object(audit, 'bundle', side_effect=RuntimeError('missing dependency')), \
                 contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(audit.main(), 1)
            report = json.loads((output/'digest-audit.json').read_text())
            self.assertEqual(report['status'], 'error')
            self.assertEqual(report['error'], 'missing dependency')
            self.assertIn('missing dependency', (output/'summary.txt').read_text())

    def test_environment_and_dump_rejection(self):
        with patch.dict(audit.os.environ, {'MMODERN_M44_SCENARIO':'stale','MMODERN_M49_TRACE':'stale','SDL_AUDIODRIVER':'wasapi'}):
            env = audit.clean_environment(Path('bundle'))
            self.assertNotIn('MMODERN_M44_SCENARIO',env)
            self.assertNotIn('MMODERN_M49_TRACE',env)
            self.assertEqual(env['SDL_AUDIODRIVER'],'dummy')
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)/'fields.tsv'
            path.write_text('snapshot.food\t5\nsnapshot.food\t6\n')
            with self.assertRaises(ValueError):
                audit.read_dump(path)
            path.write_text('envelope.version\t9\n')
            with self.assertRaises(ValueError):
                audit.read_dump(path)


if __name__ == '__main__':
    unittest.main()
