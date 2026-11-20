// Physical phone -> production navigator -> BLE -> head-unit replay.
// Install only under com.aabridge.aa_bridge.navtest to preserve user data.
import 'dart:async';
import 'dart:convert';
import 'dart:io';

import 'package:flutter/material.dart';
import 'package:path_provider/path_provider.dart';
import 'package:package_info_plus/package_info_plus.dart';
import 'package:aa_bridge/main.dart' show launchAaBridgeApp;
import 'package:aa_bridge/ble/ble_proxy.dart';
import 'package:aa_bridge/nav/geo.dart';
import 'package:aa_bridge/nav/location_service.dart';
import 'package:aa_bridge/nav/map_data.dart';
import 'package:aa_bridge/nav/nav_controller.dart';
import 'package:aa_bridge/nav/ride_simulator.dart';
import 'package:aa_bridge/nav/route_guide.dart';
import 'package:aa_bridge/nav/router.dart';
import 'package:aa_bridge/nav/way_classes.dart';
import 'package:aa_bridge/ui/navigator_screen.dart';
import 'nav_phone_fixture.dart';

Future<void> main() async {
  WidgetsFlutterBinding.ensureInitialized();
  final info = await PackageInfo.fromPlatform();
  if (!info.packageName.endsWith('.navtest')) {
    throw StateError('Physical replay requires the isolated .navtest package');
  }
  await launchAaBridgeApp(appOverride: const PhoneDrive());
}

class PhoneDrive extends StatefulWidget {
  const PhoneDrive({super.key});
  @override
  State<PhoneDrive> createState() => _PhoneDriveState();
}

class _PhoneDriveState extends State<PhoneDrive> {
  final controller = NavController();
  final fixes = StreamController<GeoFix>();
  final clock = Stopwatch()..start();
  File? log;
  Timer? status;

  void record(String event, [Map<String, Object?> fields = const {}]) {
    final line = jsonEncode({
      'ms': clock.elapsedMilliseconds,
      'utc': DateTime.now().toUtc().toIso8601String(),
      'event': event,
      ...fields,
    });
    log?.writeAsStringSync('$line\n', mode: FileMode.append);
    // Also retain events in adb logcat if the external file is unavailable.
    debugPrint('PHONE_DRIVE $line');
  }

  @override
  void initState() {
    super.initState();
    final route = RouteResult(
      points: phoneDrivePoints,
      lengthM: cumulativeM(phoneDrivePoints).last,
      timeS: 353,
      legs: const [],
      profile: RideProfile.scooter,
      segmentTypes: phoneDriveTypes,
    );
    controller.start = route.points.first;
    controller.finish = route.points.last;
    controller.route = route;
    controller.guide = RouteGuide(route);
    controller.simulating = true;
    controller.attachFixes(fixes.stream);
    controller.addListener(() {
      final fix = controller.lastFix;
      if (fix != null) {
        record('consumed', {
          'lat': fix.position.lat,
          'lon': fix.position.lon,
          'speedMs': fix.speedMs,
          'simulating': controller.simulating,
        });
      }
    });
    WidgetsBinding.instance.addPostFrameCallback((_) => unawaited(drive()));
  }

  Future<void> drive() async {
    try {
      final dir = await getExternalStorageDirectory();
      if (dir == null) throw StateError('No evidence directory');
      log = File(
        '${dir.path}/phone-drive-${DateTime.now().millisecondsSinceEpoch}.jsonl',
      );
      record('init', {
        'hz': 1,
        'speedMs': 5,
        'speedup': 1,
        'points': phoneDrivePoints.length,
        'fixture': 'Rynek-Kazimierz-20261119',
      });
      await Future<void>.delayed(const Duration(seconds: 8));
      await BleProxy.instance.syncStatus();
      await BleProxy.instance.connect('58:E6:C5:D2:AA:2A');
      status = Timer.periodic(const Duration(seconds: 1), (_) {
        final ble = BleProxy.instance;
        record('status', {
          'ble': ble.currentState.name,
          'visible': ble.navState.visible,
          'navMode': ble.navState.navMode,
          'tilesReady': MapData.instance.tiles != null,
        });
      });
      final deadline = DateTime.now().add(const Duration(seconds: 90));
      while (BleProxy.instance.currentState != BleConnState.connected ||
          !BleProxy.instance.navState.visible ||
          MapData.instance.tiles == null) {
        if (DateTime.now().isAfter(deadline)) {
          throw StateError('Head not ready');
        }
        await Future<void>.delayed(const Duration(seconds: 1));
      }
      controller.startNavigation();
      final guide = controller.guide!;
      final first = GeoFix(
        position: guide.positionAt(0),
        speedMs: 0,
        headingDeg: bearingDeg(guide.positionAt(0), guide.positionAt(5)),
      );
      fixes.add(first);
      record('warmup');
      await Future<void>.delayed(const Duration(seconds: 20));
      record('ride-start');
      const shortSeconds = int.fromEnvironment('PHONE_DRIVE_SECONDS');
      final riding = Stopwatch()..start();
      await for (final fix in simulateRide(
        guide,
        speedMs: 5,
        speedup: 1,
        tick: const Duration(seconds: 1),
      )) {
        fixes.add(fix);
        record('fix', {
          'lat': fix.position.lat,
          'lon': fix.position.lon,
          'speedMs': fix.speedMs,
          'headingDeg': fix.headingDeg,
        });
        if (shortSeconds > 0 && riding.elapsed.inSeconds >= shortSeconds) {
          record('arrival-injection', {'afterSeconds': shortSeconds});
          fixes.add(
            GeoFix(
              position: guide.positionAt(guide.totalM - 8),
              speedMs: 5,
              headingDeg: fix.headingDeg,
            ),
          );
          break;
        }
      }
      record('ride-end');
      await Future<void>.delayed(const Duration(seconds: 10));
      record('complete');
    } catch (error, stack) {
      record('error', {'error': '$error', 'stack': '$stack'});
    }
  }

  @override
  void dispose() {
    status?.cancel();
    unawaited(fixes.close());
    super.dispose();
  }

  @override
  Widget build(BuildContext context) =>
      MaterialApp(home: NavigatorScreen(controller: controller));
}
