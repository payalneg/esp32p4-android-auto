/// Verification verdicts against the fake VESC.
///
/// The point of every case here is the same: the protocol acks say "fine".
/// These are the ways the hardware tells you it isn't.
library;

import 'package:aa_bridge/agent/verify.dart';
import 'package:flutter_test/flutter_test.dart';

import 'fake_lisp_device.dart';

/// Short timings — the production defaults spend 3.5 s per flash.
const _fast = [
  Duration(milliseconds: 1),
  Duration(milliseconds: 2),
  Duration(milliseconds: 3),
];

Future<VerifyReport> flash(FakeLispDevice dev,
        {bool run = true,
        List<String> expect = const [],
        List<String> moving = const []}) =>
    flashAndVerify(dev, '(def x 0)\n',
        run: run,
        expectPrints: expect,
        movingGlobals: moving,
        settle: Duration.zero,
        sampleAt: _fast);

void main() {
  test('a healthy script verifies', () async {
    final dev = FakeLispDevice()..movingGlobals.add('x');
    final r = await flash(dev, moving: ['x']);
    expect(r.verdict, Verdict.ok);
    expect(r.pass, isTrue);
    expect(dev.running, isTrue);
  });

  test('stops the running script before erasing', () async {
    final dev = FakeLispDevice()..running = true;
    await flash(dev);
    expect(dev.calls, contains('upload(run=true,stopFirst=true)'));
  });

  test('flash without run is verified by the acks alone', () async {
    final dev = FakeLispDevice();
    final r = await flash(dev, run: false);
    expect(r.verdict, Verdict.flashedNotRun);
    expect(r.pass, isTrue);
    expect(dev.running, isFalse);
    // No stats sampling needed when nothing was started.
    expect(r.samples, isEmpty);
  });

  test('doneCtx carrying an eval error fails and is quoted verbatim',
      () async {
    final dev = FakeLispDevice()
      ..doneCtx = 'out_of_memory'
      ..heap = 98.7;
    final r = await flash(dev);
    expect(r.verdict, Verdict.evalError);
    expect(r.pass, isFalse);
    expect(r.reasons.join(), contains('out_of_memory'));
    // …and the project's hard-won explanation is attached where it helps.
    expect(r.hint, contains('@const'));
  });

  test('variable_not_bound gets the load-order hint', () async {
    final dev = FakeLispDevice()..doneCtx = 'variable_not_bound';
    final r = await flash(dev);
    expect(r.hint, contains('before it was defined'));
  });

  test('a failed script is stopped automatically', () async {
    final dev = FakeLispDevice()..doneCtx = 'eval_error';
    final r = await flash(dev);
    expect(r.pass, isFalse);
    expect(r.stoppedAfterFailure, isTrue);
    expect(dev.running, isFalse);
    expect(dev.calls.last, 'setRunning(false)');
  });

  test('benign doneCtx values are not treated as errors', () async {
    final dev = FakeLispDevice()..doneCtx = 'done';
    expect((await flash(dev)).verdict, Verdict.ok);
  });

  test('no bindings at all means the script is not loaded', () async {
    final dev = FakeLispDevice();
    // Script "starts" but reports nothing — bindings only appear when running.
    dev.globals.clear();
    final r = await flash(dev);
    expect(r.verdict, Verdict.notRunning);
    expect(r.pass, isFalse);
  });

  test('a watched global that never moves means execution stopped', () async {
    final dev = FakeLispDevice(); // 'x' is NOT in movingGlobals
    final r = await flash(dev, moving: ['x']);
    expect(r.verdict, Verdict.notRunning);
    expect(r.reasons.join(), contains("'x' did not change"));
    expect(r.movingGlobals['x'], hasLength(3));
  });

  test('a climbing heap is flagged as a possible OOM', () async {
    final dev = FakeLispDevice()
      ..movingGlobals.add('x')
      ..heap = 80
      ..heapClimbPerSample = 4;
    final r = await flash(dev, moving: ['x']);
    expect(r.verdict, Verdict.oomSuspect);
    expect(r.pass, isFalse);
  });

  test('expected prints that do arrive pass', () async {
    final dev = FakeLispDevice()
      ..movingGlobals.add('x')
      ..printsPerSample.add('agent-probe');
    final r = await flash(dev, expect: ['agent-probe'], moving: ['x']);
    expect(r.verdict, Verdict.ok);
    expect(r.console, contains('agent-probe'));
  });

  test('missing prints fail when the console works', () async {
    final dev = FakeLispDevice()
      ..movingGlobals.add('x')
      ..printsPerSample.add('something else');
    final r = await flash(dev, expect: ['agent-probe'], moving: ['x']);
    expect(r.verdict, Verdict.expectMissed);
    expect(r.pass, isFalse);
  });

  test('missing prints are inconclusive when the console is dead', () async {
    // The direct-adapter case: output may be going to the head unit instead.
    final dev = FakeLispDevice(consoleAlive: false)..movingGlobals.add('x');
    final r = await flash(dev, expect: ['agent-probe'], moving: ['x']);
    expect(r.verdict, Verdict.inconclusive);
    // Not a failure: the model must not be punished for a dead channel.
    expect(r.pass, isTrue);
    expect(r.hint, contains('debug global'));
  });

  test('a pinned CPU is flagged but still passes', () async {
    final dev = FakeLispDevice()
      ..movingGlobals.add('x')
      ..cpu = 99;
    final r = await flash(dev, moving: ['x']);
    expect(r.verdict, Verdict.cpuSaturated);
    expect(r.pass, isTrue);
  });

  test('losing the link after flashing is reported as such', () async {
    final dev = FakeLispDevice()..failStats = StateError('link gone');
    final r = await flash(dev);
    expect(r.verdict, Verdict.linkLost);
    expect(r.pass, isFalse);
  });

  test('the report serialises everything the model needs', () async {
    final dev = FakeLispDevice()
      ..movingGlobals.add('x')
      ..printsPerSample.add('hello');
    final json = (await flash(dev, moving: ['x'])).toJson();
    expect(json['verdict'], 'ok');
    expect(json['pass'], isTrue);
    expect(json['samples'], hasLength(3));
    expect(json['moving_globals'], contains('x'));
    expect(json['console'], contains('hello'));
    expect(json['console_alive'], isTrue);
  });
}
