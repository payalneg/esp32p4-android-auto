"""Host regression tests executing the control functions from main.adc-pas.lisp.

The small evaluator implements only the Lisp subset used by these functions;
it does NOT validate LispBM scheduling, heap/stack usage or physical hardware.
Run: python scripts/test_adc_pas.py
"""
import math
from pathlib import Path
import re
import unittest


class Symbol(str):
    pass


def parse(source):
    tokens = iter(t for t in re.findall(
        r';[^\n]*|"(?:\\.|[^"\\])*"|[(){}\']|[^\s(){}\';]+', source)
        if not t.startswith(';'))

    def item(token):
        if token in ('(', '{'):
            result = [] if token == '(' else [Symbol('progn')]
            end = ')' if token == '(' else '}'
            for token in tokens:
                if token == end:
                    return result
                result.append(item(token))
            raise ValueError('Unclosed expression')
        if token in (')', '}'):
            raise ValueError('Unexpected closing delimiter')
        if token == "'":
            return [Symbol('quote'), item(next(tokens))]
        if token.startswith(';'):
            return None
        if token.startswith('"'):
            return token[1:-1]
        try:
            return float(token) if '.' in token else int(token, 0)
        except ValueError:
            return Symbol(token)

    return [item(t) for t in tokens]


class Env(dict):
    def __init__(self, parent=None, **values):
        super().__init__(values)
        self.parent = parent

    def owner(self, key):
        if key in self:
            return self
        if self.parent is not None:
            return self.parent.owner(key)
        raise KeyError(key)


def curve(x, accel, brake, mode):
    if mode == 0:
        return 1 - (1-x)**(1+accel) if accel >= 0 else x**(1-accel)
    if mode == 1 and abs(accel) > 1e-10:
        return (1-math.expm1(accel*(1-x))/math.expm1(accel) if accel >= 0
                else math.expm1(-accel*x)/math.expm1(-accel))
    if mode == 2:
        return 1-(1-x)/(1+accel*x) if accel >= 0 else x/(1-accel*(1-x))
    return x


class Controller:
    def __init__(self):
        self.conf = {'app-to-use': 2, 'adc-ctrl-type': 8,
                     'adc-v1-start': .9, 'adc-v1-end': 3.,
                     'adc-v1-min': .5, 'adc-v1-max': 3.2,
                     'adc-thr-hyst': .05, 'timeout-msec': 500,
                     'l-current-max': 100., 'l-current-max-scale': 1.,
                     'max-speed': 60/3.6}
        self.raw, self.brake, self.speed, self.current = .9, 0., 10., 30.
        self.time, self.fault, self.disabled = 10., 0, False
        self.writes = []
        self.env = Env()
        self.env.update({
            'nil': False, 't': True,
            '+': lambda *a: sum(a), '*': lambda *a: math.prod(a),
            '-': lambda a, *b: a-sum(b) if b else -a,
            '/': lambda a, b: a/b, 'abs': abs,
            '=': lambda a, b: a == b, '>': lambda a, b: a > b,
            '<': lambda a, b: a < b, '>=': lambda a, b: a >= b,
            '<=': lambda a, b: a <= b, 'not': lambda a: not a,
            'conf-get': self.conf.__getitem__,
            'conf-set': self.conf.__setitem__,
            'get-adc': lambda ch: self.raw,
            'get-adc-decoded': lambda ch: self.brake,
            'get-speed': lambda: self.speed,
            'get-current': lambda: self.current,
            'get-fault': lambda: self.fault,
            'app-is-output-disabled': lambda: self.disabled,
            'secs-since': lambda seen: self.time-seen,
            'app-adc-override': lambda ch, volts: self.writes.append((ch, volts)),
            'throttle-curve': curve, 'print': lambda *a: None,
            'str-merge': lambda *a: ''.join(a), 'to-str': str,
            'eeprom-read-i': lambda addr: False, 'bufcreate': bytearray,
        })
        self.source = (Path(__file__).resolve().parents[1]/'lisp/main.adc-pas.lisp').read_text(encoding='utf-8')
        self.forms = parse(self.source)
        for form in self.forms:
            if isinstance(form, list) and form and form[0] in ('def', 'defun'):
                self.eval(form, self.env)
        self.env.update({'throttle-armed': 1, 'rpm-per-ms': 100.})

    def eval(self, x, env):
        if isinstance(x, Symbol):
            return env.owner(x)[x]
        if not isinstance(x, list):
            return x
        if not x:
            return False
        op, *args = x
        if op == 'quote':
            return args[0]
        if op == 'progn':
            result = False
            for arg in args:
                result = self.eval(arg, env)
            return result
        if op == 'if':
            return self.eval(args[1] if self.eval(args[0], env) else
                             args[2] if len(args) > 2 else False, env)
        if op == 'and':
            return all(self.eval(a, env) for a in args)
        if op == 'or':
            return any(self.eval(a, env) for a in args)
        if op in ('def', 'setq'):
            target = self.env if op == 'def' else env.owner(args[0])
            target[args[0]] = self.eval(args[1], env)
            return target[args[0]]
        if op == 'defun':
            name, params, body = args
            def fn(*values):
                local = Env(env)
                local.update(zip(params, values, strict=True))
                return self.eval(body, local)
            self.env[name] = fn
            return fn
        if op == 'let':
            local = Env(env)
            for name, value in args[0]:
                local[name] = self.eval(value, local)
            return self.eval(args[1], local)
        if op == 'cond':
            for condition, body in args:
                if self.eval(condition, env):
                    return self.eval(body, env)
            return False
        if op == 'loopwhile':
            count = 0
            while self.eval(args[0], env):
                self.eval(args[1], env)
                count += 1
                if count > 10000:
                    raise RuntimeError('Unexpected unbounded loop')
            return False
        return self.eval(op, env)(*[self.eval(a, env) for a in args])

    def call(self, name, *args):
        return self.env[name](*args)

    def tick(self, dt=.01):
        self.call('motor-tick', dt)
        return self.writes[-1][1] if self.writes else None

    def pas(self, amps):
        self.env.update({'pas-amps': amps, 'pas-seen': self.time})

    def fraction(self, volts):
        x = self.call('physical-throttle', volts)
        h = self.conf['adc-thr-hyst']
        return curve(max(0, (x-h)/(1-h)), self.env['adc-curve-accel'], 0,
                     self.env['adc-curve-mode'])


class AdcPasTests(unittest.TestCase):
    def setUp(self):
        self.c = Controller()

    def test_manual_has_priority_over_pas_and_cruise(self):
        c = self.c
        c.call('activate-cruise-control')
        c.pas(100)
        target = c.env['cruise-rpm']
        for raw in (1.1, 1.8, 3.):
            c.raw = raw
            self.assertEqual(c.tick(), raw)
            self.assertEqual(c.env['cruise-active'], 1)
            self.assertEqual(c.env['cruise-rpm'], target)
        c.raw = .9
        self.assertLess(c.fraction(c.tick()), .5)  # cruise, not full PAS

    def test_full_brake_removes_full_gas_and_cancels_cruise(self):
        c = self.c
        c.call('activate-cruise-control')
        c.pas(100)
        c.raw, c.brake = 3., 1.
        self.assertEqual(c.tick(), .9)
        self.assertEqual(c.env['cruise-active'], 0)
        self.assertEqual(c.env['pas-amps'], 0)
        self.assertTrue(all(ch == 0 for ch, v in c.writes))

    def test_small_brake_also_removes_full_gas(self):
        self.c.raw, self.c.brake = 3., .01
        self.assertEqual(self.c.tick(), .9)

    def test_pas_fraction_tracks_profile_limit(self):
        c = self.c
        c.pas(20)
        for scale in (.5, .67, 1.):
            c.conf['l-current-max-scale'] = scale
            self.assertAlmostEqual(c.fraction(c.tick()), 20/(100*scale), places=4)

    def test_pas_expiry_and_clamping(self):
        c = self.c
        c.pas(200)
        self.assertAlmostEqual(c.fraction(c.tick()), 1., places=4)
        c.time += .41
        self.assertEqual(c.tick(), .9)

    def test_inverse_curve_deadband_and_inversion(self):
        c = self.c
        for inverted in (0, 1):
            c.env['adc1-inverted'] = inverted
            for mode in range(3):
                c.env['adc-curve-mode'] = mode
                for gain in (-1., -.4, 0., .4, 1.):
                    c.env['adc-curve-accel'] = gain
                    for target in (.001, .1, .3, .7, .999):
                        volts = c.call('current-to-volts', target)
                        self.assertAlmostEqual(c.fraction(volts), target, delta=2e-5)

    def test_master_off_preserves_brake_and_requires_release(self):
        c = self.c
        c.raw, c.brake = 3., 1.
        c.call('panel-set-throttle', 0)
        self.assertEqual(c.tick(), .9)
        c.call('panel-set-throttle', 1)
        c.brake = 0
        self.assertEqual(c.tick(), .9)
        c.raw = .9
        c.tick()
        c.raw = 2.
        self.assertEqual(c.tick(), 2.)

    def test_raw_wire_fault_is_not_hidden_by_override(self):
        c = self.c
        c.pas(100)
        c.raw = .1
        self.assertEqual(c.tick(), .9)
        self.assertEqual(c.env['throttle-armed'], 0)
        c.raw = 3.
        self.assertEqual(c.tick(), .9)

    def test_incompatible_configuration_stops_keepalive(self):
        c = self.c
        for key, value in [('adc-ctrl-type', 1), ('app-to-use', 10),
                           ('timeout-msec', 0), ('adc-v1-end', .9)]:
            saved = c.conf[key]
            c.conf[key] = value
            c.writes.clear()
            c.tick()
            self.assertEqual(c.writes, [])
            c.conf[key] = saved

    def test_detection_fault_and_scheduler_stall_inhibit(self):
        c = self.c
        c.call('activate-cruise-control')
        c.raw, c.disabled = 2., True
        self.assertEqual(c.tick(), .9)
        self.assertEqual(c.env['cruise-active'], 0)
        c.disabled, c.fault = False, 1
        self.assertEqual(c.tick(), .9)
        c.fault = 0
        self.assertEqual(c.tick(.5), .9)

    def test_pid_engagement_is_seeded_and_derivative_has_no_target_kick(self):
        c = self.c
        c.call('activate-cruise-control')
        self.assertAlmostEqual(c.fraction(c.tick()), .3, places=4)
        c.call('increase-cruise-speed')
        c.tick()
        self.assertAlmostEqual(c.env['cruise-d'], 0.)
        self.assertEqual(c.env['cruise-active'], 1)
        for _ in range(100):
            c.call('decrease-cruise-speed')
        self.assertEqual(c.env['cruise-active'], 1)
        self.assertAlmostEqual(c.env['cruise-rpm']/c.env['rpm-per-ms'], .5)

    def test_pid_bounds_windup_and_simple_load_recovery(self):
        c = self.c
        c.call('activate-cruise-control')
        c.speed = 0
        for _ in range(500):
            self.assertTrue(0 <= c.fraction(c.tick()) <= 1)
        self.assertTrue(0 <= c.env['cruise-i'] <= 1)
        # Restore speed and simulate a simple motor/load with downstream ADC ramp.
        c.speed, c.env['cruise-last-speed'], c.env['cruise-d'] = 10., 10., 0.
        c.env['cruise-i'] = .3
        output = .3
        for n in range(3000):
            requested = c.fraction(c.tick())
            output += max(-.01/.3, min(.01/.3, requested-output))
            load = .3 if n < 500 else .4
            c.speed += .01*(4*output-4*load-.05*(c.speed-10))
            c.current = 100*output
        self.assertAlmostEqual(c.speed, 10., delta=.15)

    def test_no_direct_motor_or_adc2_commands(self):
        def calls(x):
            if not isinstance(x, list) or not x:
                return
            if isinstance(x[0], Symbol):
                yield x[0], x[1:]
            for child in x:
                yield from calls(child)
        found = list(calls(self.c.forms))
        self.assertFalse(any(name in {'set-current', 'set-current-rel', 'set-rpm',
                                      'set-brake', 'set-brake-rel', 'app-disable-output'}
                             for name, args in found))
        self.assertFalse(any(name == 'app-adc-override' and args[0] == 1
                             for name, args in found))
        self.assertIn((Symbol('app-adc-detach'), [3, 2]), found)


if __name__ == '__main__':
    unittest.main(verbosity=2)
