/// Tiny hand-rolled i18n. Two locales, no codegen, no ARB pipeline —
/// the app is small enough that a flat per-language map pays for itself
/// in build simplicity.
///
/// Usage:
///   t(context, 'home.title')
///
/// The active locale is held by [LocaleScope] above MaterialApp and
/// persisted via SharedPreferences. Default is English.
library;

import 'package:flutter/material.dart';
import 'package:shared_preferences/shared_preferences.dart';

const _en = <String, String>{
  'app.title': 'VESC Display Tool',

  'home.status.connected': 'Connected',
  'home.status.disconnected': 'Not connected',
  'home.state.idle': 'BLE idle',
  'home.state.scanning': 'Scanning…',
  'home.state.connecting': 'Connecting…',
  'home.state.connected': 'Link established',
  'home.state.disconnected':
      'Link lost — waiting for device to come back on air…',
  'home.forget': 'Forget device',
  'home.restart': 'Restart BLE',
  'home.restart.toast': 'Restarting the Bluetooth link…',
  'home.forget.confirm.title': 'Forget device?',
  'home.forget.confirm.body':
      'Auto-reconnect will stop. To use it again you\'ll have to pair from scratch.',
  'home.forget.cancel': 'Cancel',
  'home.forget.ok': 'Forget',

  'home.notif.title': 'Notification access',
  'home.notif.granted': 'Granted',
  'home.notif.denied': 'Allow it in system settings',
  'home.batt.title': 'No battery restrictions',
  'home.batt.granted': 'OS won\'t throttle the BLE link',
  'home.batt.denied':
      'Without this the OS kills BLE in the background after ~30 min',
  'home.pairing.title': 'Pair with head unit',
  'home.pairing.none': 'No device selected',
  'home.pairing.saved': 'Saved: ',
  'home.filter.title': 'Which apps to forward',
  'home.ios.title': 'iOS mode',
  'home.ios.body':
      'iOS sandbox blocks reading third-party notifications. Only pairing '
      'and the app\'s own media metadata work.',
  'home.lang.title': 'Language',
  'home.test.title': 'Test panel',

  'pairing.title': 'Find head unit',
  'pairing.empty':
      'Tap "refresh" to start scanning. The head unit must be nearby and '
      'advertising.',
  'pairing.error': 'Error: ',
  'pairing.unnamed': 'Unnamed',

  'filter.title': 'Apps',
  'filter.search': 'Search…',

  'test.notif.section': 'Notification test',
  'test.media.section': 'Media test',
  'test.notif.whatsapp': 'WhatsApp-style',
  'test.notif.telegram': 'Telegram-style',
  'test.notif.long': 'Long text',
  'test.not_connected': 'Not connected to head unit',
  'test.sent': 'Sent: ',
  'test.seek': '+30s',

  'lang.choose': 'Choose language',
  'lang.en': 'English',
  'lang.ru': 'Русский',

  'home.fw.title': 'Update head unit firmware',
  'home.about.title': 'About',
  'about.title': 'About',
  'about.app': 'App version',
  'about.fw': 'Bundled firmware',
  'about.desc': 'Bridges phone notifications, media and time to an ESP32-P4 head unit over BLE, and flashes its firmware over WiFi.',
  'about.source': 'Source code',
  'fw.title': 'Firmware update',
  'fw.device': 'Head unit version',
  'fw.detected': 'Detected device',
  'fw.detected.unknown': 'Unknown',
  'fw.select': 'Firmware to flash',
  'fw.warn.undetected':
      'Couldn\'t read the head unit\'s board — pick the right firmware manually.',
  'fw.warn.mismatch':
      'Selected firmware is for a different board than detected — flash only if you\'re sure.',
  'fw.bundled': 'Bundled in app',
  'fw.unsupported': 'This head unit does not support over-the-air updates from the app.',
  'fw.disconnected': 'Connect to the head unit over Bluetooth first.',
  'fw.reading': 'Reading device info…',
  'fw.uptodate': 'Head unit is already on this version.',
  'fw.flash': 'Flash firmware',
  'fw.flashing': 'Updating…',
  'fw.warn.title': 'Flash firmware?',
  'fw.warn.body': 'The firmware bundled in the app will be uploaded to the head unit. Keep it powered the whole time — it reboots when done.',
  'fw.host': 'Device address',
  'fw.host.note': 'Make sure the phone is on the head unit\'s WiFi.',
  'fw.method': 'Update method',
  'fw.method.ble': 'Bluetooth',
  'fw.method.wifi': 'WiFi',
  'fw.method.ble.note':
      'Flashes over the existing Bluetooth link — no WiFi needed. Slower (a few minutes); keep the app open.',
  'fw.method.ble.unsupported':
      'This head unit\'s firmware can\'t flash over Bluetooth — update over WiFi.',
  'fw.warn.go': 'Flash',
  'fw.cancel': 'Cancel',
  'fw.done': 'Firmware uploaded — the head unit is rebooting.',
  // Firmware-update status/error messages (emitted as keys by the updater so
  // they localise; {size}/{host}/{err} are substituted at display time).
  'fw.ota.uploading.wifi': 'Uploading firmware ({size} KB)…',
  'fw.ota.uploading.ble': 'Sending firmware over Bluetooth ({size} KB)…',
  'fw.ota.verifying': 'Verifying image…',
  'fw.ota.verifying.device': 'Verifying image on the device…',
  'fw.ota.rejected': 'The device rejected the firmware',
  'fw.ota.connfail': 'Couldn\'t connect to {host}: {err}',
  'fw.ota.blefail': 'Bluetooth update failed',
  'fw.ota.bleerror': 'Bluetooth update error: {err}',
  'fw.ota.err.noconn': 'No Bluetooth connection to the head unit',
  'fw.ota.err.badsha': 'Invalid image checksum',
  'fw.ota.err.linklost': 'Bluetooth connection lost',
  'fw.ota.err.timeout': 'The device didn\'t respond in time',
  'fw.ota.err.nopart': 'No spare update partition on the device',
  'fw.ota.err.size': 'The image doesn\'t fit the device partition',
  'fw.ota.err.alloc': 'The device ran out of memory',
  'fw.ota.err.sha': 'Checksum error — the transfer was corrupted, retry',
  'fw.ota.err.begin': 'Couldn\'t prepare the device flash',
  'fw.ota.err.write': 'Device flash write error',
  'fw.ota.err.end': 'The image failed verification on the device',
  'fw.ota.err.boot': 'Couldn\'t switch the boot partition',
  'fw.ota.err.proto': 'Transfer failed — retry the update',
  'fw.ota.err.unknown': 'Device error',
  'fw.manual.hint': 'Couldn\'t read WiFi credentials from the head unit — enter them manually.',
  'fw.manual.ssid': 'Head unit WiFi name (SSID)',
  'fw.manual.password': 'Head unit WiFi password',
  'fw.manual.needfields': 'Enter the WiFi name and password.',
  'fw.manual.current': 'Phone is on WiFi: ',
  'fw.manual.current.none': 'not connected to WiFi',
  'fw.manual.scan': 'Scan',
  'fw.manual.pick': 'Select head unit WiFi',
  'fw.manual.noscan': 'No networks found — is location enabled?',

  'home.files.title': 'Device files',
  'files.title': 'Device files',
  'files.unsupported': 'This head unit firmware does not support file browsing.',
  'files.empty': 'Empty folder',
  'files.loading': 'Loading…',
  'files.dir': 'Folder',
  'files.notready': 'Storage is being prepared (first boot formats it, ~1 min).',
  'files.notready.retry': 'Retry',
  'files.upload': 'Upload file',
  'files.uploading': 'Uploading…',
  'files.mkdir': 'New folder',
  'files.mkdir.name': 'Folder name',
  'files.download': 'Download',
  'files.downloading': 'Downloading…',
  'files.saved': 'Saved to: ',
  'files.rename': 'Rename',
  'files.rename.to': 'New name',
  'files.delete': 'Delete',
  'files.delete.confirm': 'Permanently delete?',
  'files.cancel': 'Cancel',
  'files.ok': 'OK',
  'files.truncated': 'Listing truncated — too many entries.',
  'files.err.noconn': 'Not connected to head unit.',
  'files.err.notready': 'Storage is still mounting — try again shortly.',
  'files.err.badpath': 'Invalid path.',
  'files.err.noent': 'No such file or folder.',
  'files.err.notdir': 'Not a folder.',
  'files.err.isdir': 'That is a folder.',
  'files.err.nospc': 'Not enough free space on the device.',
  'files.err.exist': 'Already exists.',
  'files.err.io': 'I/O error.',
  'files.err.sha': 'Checksum mismatch — transfer corrupted.',
  'files.err.proto': 'Protocol error.',
  'files.err.alloc': 'Out of memory on the device.',
  'files.err.busy': 'Device is busy with another operation.',
  'files.err.toobig': 'File is too large for the device storage.',
  'files.err.unknown': 'Unknown error.',

  'home.splash.title': 'Boot splash',
  'splash.title': 'Boot splash',
  'splash.intro':
      'Pick a GIF: the app slices it into JPEG frames the head unit plays at '
          'boot through its hardware decoder. A simple GIF can be used as a '
          'fallback.',
  'splash.set_animated': 'Set animated splash (from GIF)',
  'splash.set_animated.sub': 'Slice a GIF into frames and upload (recommended)',
  'splash.set_gif': 'Set simple GIF (fallback)',
  'splash.set_gif.sub': 'Upload a raw GIF played by the slower decoder',
  'splash.building': 'Slicing GIF into frames…',
  'splash.clearing': 'Preparing folder…',
  'splash.uploading': 'Uploading frames…',
  'splash.done': 'Boot splash updated — reboot the head unit to see it.',
  'splash.reboot_hint': 'Reboot the head unit to see the new splash.',
  'splash.err.decode': 'Could not read that GIF.',

  'home.display.title': 'Display settings',
  'home.helper.title': 'BLE helper settings',
  'home.helper.subtitle': 'ESP32-C3 bridge: buttons, cadence sensor, PAS',
  'home.lisp.title': 'LISP script editor',

  // VESC BLE Helper (ESP32-C3).
  'helper.title': 'BLE helper',
  'helper.tab.status': 'Status',
  'helper.tab.params': 'Parameters',
  'helper.tab.bind': 'Pairing',
  'helper.tab.fw': 'Firmware',
  'helper.connect': 'Connect',
  'helper.disconnect': 'Disconnect',
  'helper.searching': 'Searching for the helper…',
  'helper.connecting': 'Connecting…',
  'helper.notFound': 'Helper not found. Power it up and try again.',
  'helper.disconnected': 'Not connected',
  'helper.connected': 'Connected',
  'helper.fw': 'Firmware {v}',
  'helper.log': 'Log',
  'helper.intro': 'The helper links BLE buttons and a cadence sensor to the '
      'VESC over CAN. Connect to configure it.',

  // Status tab.
  'helper.status.cadence': 'Cadence',
  'helper.status.rpm': 'RPM',
  'helper.status.offline': 'sensor offline',
  'helper.status.assist': 'Assist',
  'helper.status.level': 'Level',
  'helper.status.battery': 'Battery',
  'helper.status.links': 'Links',
  'helper.status.sensor': 'Sensor',
  'helper.status.remote': 'Remote',
  'helper.status.vesc': 'VESC',
  'helper.status.pas': 'PAS',
  'helper.status.bound': 'bound',
  'helper.status.notBound': 'not bound',
  'helper.status.online': 'connected',
  'helper.status.linkOk': 'link ok',
  'helper.status.noData': 'no data',
  'helper.status.on': 'on',
  'helper.status.off': 'off',
  'helper.status.buttons': 'Remote buttons',
  'helper.status.noButtons':
      'No buttons learned yet — press any button on a bound remote.',
  'helper.status.throttle': 'Throttle override',
  'helper.status.toggle': 'Toggle',
  'helper.status.noVescWarn':
      'No link to the VESC — the override may not reach the motor.',
  'helper.status.waiting': 'Waiting for status…',

  // Parameters tab.
  'helper.params.can': 'CAN bus',
  'helper.params.helperId': 'Helper ID',
  'helper.params.vescId': 'VESC ID',
  'helper.params.bitrate': 'Bitrate, kbps',
  'helper.params.pas': 'Pedal assist',
  'helper.params.enabled': 'PAS enabled',
  'helper.params.reverse': 'Sensor reversed',
  'helper.params.reverse.desc': 'Flip if assist engages when back-pedaling',
  'helper.params.mode': 'Mode',
  'helper.params.mode.switch': 'Switch (levels)',
  'helper.params.mode.prop': 'Proportional (cadence)',
  'helper.params.levelCount': 'Level count',
  'helper.params.level': 'Assist level',
  'helper.params.level.off': '0 — off',
  'helper.params.startCurrent': 'Start current, %',
  'helper.params.startDelay': 'Start delay, ms',
  'helper.params.stopDelay': 'Stop delay, ms',
  'helper.params.minCadence': 'Min cadence, rpm',
  'helper.params.fullCadence': 'Full cadence, rpm',
  'helper.params.maxCurrent': 'Max current, A',
  'helper.params.ramp': 'Ramp, A/s',
  'helper.params.buttons': 'Button CAN commands',
  'helper.params.button': 'Button {n}',
  'helper.params.canId': 'CAN ID (hex)',
  'helper.params.canData': 'data (hex)',
  'helper.params.buttonsHint':
      'Buttons are learned by first press across ALL bound remotes (first '
          'ever pressed = A, next = B, …). What a frame does is decided by '
          'the LISP script on the VESC.',
  'helper.params.read': 'Read',
  'helper.params.write': 'Write',
  'helper.params.written': 'Parameters written',
  'helper.params.badValue': 'Check the field values',
  'helper.params.waiting': 'Reading parameters…',

  // Pairing tab.
  'helper.bind.scanButton': 'Scan for button',
  'helper.bind.scanCadence': 'Scan for cadence sensor',
  'helper.bind.scanning': 'The helper scans for about 6 s…',
  'helper.bind.empty': 'No scan results yet',
  'helper.bind.emptyHint': 'Wake the device up and start a scan above.',
  'helper.bind.unnamed': '(unnamed)',
  'helper.bind.button': 'button',
  'helper.bind.cadence': 'cadence',
  'helper.bind.bind': 'Bind selected',
  'helper.bind.bound': 'Bound',
  'helper.bind.unbindCadence': 'Unbind sensor',
  'helper.bind.unbindButtons': 'Unbind remotes (all)',

  // Firmware tab.
  'helper.fw.installed': 'On the helper: {v}',
  'helper.fw.bundled': 'Bundled with this app: {v}',
  'helper.fw.latest': 'Newer release online: {v}',
  'helper.fw.unknown': 'unknown',
  'helper.fw.check': 'Check',
  'helper.fw.checking': 'Checking GitHub…',
  'helper.fw.downloading': 'Downloading…',
  'helper.fw.upToDate': 'The helper is up to date.',
  'helper.fw.available': 'A newer release is available.',
  'helper.fw.flash': 'Flash firmware',
  'helper.fw.flashing': 'Flashing… do not turn anything off',
  'helper.fw.done': 'Flashed — the helper is rebooting',
  'helper.fw.warn':
      'The helper reboots after flashing and the link drops; reconnect once '
          'it comes back.',
  'helper.err.busy': 'A transfer is already running',
  'lisp.editor.title': 'LISP script',
  'lisp.editor.hint': 'LISP (LispBM) code for the VESC…',
  'lisp.tab.code': 'Code',
  'lisp.tab.vars': 'Variables',
  'lisp.open': 'Open file',
  'lisp.save': 'Save file',
  'lisp.read': 'Read',
  'lisp.upload': 'Upload',
  'lisp.uploadRun': 'Upload & Run',
  'lisp.run': 'Run',
  'lisp.stop': 'Stop',
  'lisp.loaded': 'Script read from VESC',
  'lisp.uploaded': 'Uploaded to VESC',
  'lisp.uploadedRun': 'Uploaded and running',
  'lisp.opened': 'File loaded',
  'lisp.saved': 'File saved',
  'lisp.gaugesPaused': 'Reading/uploading briefly pauses the dashboard gauges.',
  'lisp.vars.waiting': 'Waiting for variables…',
  'lisp.vars.none': 'No variables',

  // Structural check of the script (the rules that fail silently on hardware).
  'lisp.lint.check': 'Check',
  'lisp.lint.title': 'Script check',
  'lisp.lint.clean': 'No problems found.',
  'lisp.lint.summary': '{e} errors, {w} warnings · {kb} KB packed',
  'lisp.lint.close': 'Close',

  // Asynchronous `(print ...)` output from the script running on the VESC.
  'lisp.console.title': 'Device output',
  'lisp.console.empty': 'No output from the script yet.',
  'lisp.console.dead': 'This VESC has not sent any print output on this link. '
      'On a direct adapter the output may be going to the head unit instead.',
  'lisp.console.clear': 'Clear',
  'lisp.console.copy': 'Copy',
  'lisp.console.copied': 'Output copied',
  'lisp.console.raw': 'Raw packets',
  'lisp.console.dropped': '{n} earlier lines dropped',

  // Which NUS link the editor talks over: the head unit's bridge, or a
  // stand-alone VESC BLE adapter connected by the app itself.
  'lisp.adapter.pick': 'Choose adapter',
  'lisp.adapter.change': 'Change',
  'lisp.adapter.headunit': 'Head unit (built-in bridge)',
  'lisp.adapter.headunit.on': 'Connected — relays to the VESC over CAN',
  'lisp.adapter.headunit.off': 'Head unit not connected',
  'lisp.adapter.none': 'No adapter selected',
  'lisp.adapter.scan': 'Scan',
  'lisp.adapter.scanning': 'Scanning…',
  'lisp.adapter.empty': 'No VESC adapter found. Power it up and scan again.',
  'lisp.adapter.showAll': 'Show all devices',
  'lisp.adapter.nusHint': 'Otherwise only devices advertising the VESC UART '
      'service are listed.',
  'lisp.state.idle': 'Not connected',
  'lisp.state.connecting': 'Connecting…',
  'lisp.state.connected': 'Connected',
  'lisp.state.failed': 'Connection lost — retrying',

  // AI assistant — settings. (The chat UI's own keys live further down.)
  'agent.settings.title': 'AI assistant',
  'agent.settings.subtitle': 'Edit the LISP script with a language model',
  'agent.settings.provider': 'Provider',
  'agent.settings.keyFrom': 'Get a key at {url}',

  // Short how-to, shown right above the key field.
  'agent.settings.howto': 'Where to get a key',
  'agent.settings.howto.1': 'Open {url} and sign in (Google or GitHub works).',
  'agent.settings.howto.2':
      'Go to Keys → Create key, give it any name, copy the value — it is '
          'shown once.',
  'agent.settings.howto.3':
      'Top up a few dollars under Credits. A session here usually costs '
          'well under a cent.',
  'agent.settings.howto.4': 'Paste the key below and tap ✓.',
  'agent.settings.howto.tapCopy': 'Tap the address to copy it.',
  'agent.settings.howto.copied': 'Address copied',
  'agent.settings.key': 'Your API key',
  'agent.settings.key.hint': 'Leave blank to use the built-in key',
  'agent.settings.key.set': 'Key saved',
  'agent.settings.key.cleared': 'Key removed',
  'agent.settings.key.stored': 'Stored in the system keystore.',
  'agent.settings.key.embedded':
      'Running on the key built into this build — nothing to set up. Enter '
          'your own above to bill your own account. Note it can be recovered '
          'from the APK, so do not share this build.',
  'agent.settings.key.own': 'Using your own key.',
  'agent.settings.key.remember': 'Remember the key',
  'agent.settings.key.rememberOff':
      'Off: the key is kept in memory only and must be re-entered next launch.',
  'agent.settings.key.tip':
      'Use a separate key with a spending limit for this app.',
  'agent.settings.model': 'Model',
  'agent.settings.model.default': 'Default: {m}',
  'agent.settings.model.fast': 'Fast',
  'agent.settings.model.strong': 'Strong',
  'agent.settings.advanced': 'Advanced',
  'agent.settings.baseUrl': 'API endpoint',
  'agent.settings.strict': 'Strict tool schemas',
  'agent.settings.strict.desc':
      'Makes the API guarantee the shape of tool arguments.',
  'agent.settings.thinking': 'Thinking mode',
  'agent.settings.thinking.desc':
      'Slower and pricier per step; better on hard problems.',
  'agent.settings.temperature': 'Temperature',
  'agent.settings.budget': 'Limits per session',
  'agent.settings.maxSteps': 'Model turns',
  'agent.settings.maxFlashes': 'Flashes to the VESC',
  'agent.settings.spendCap': 'Spend cap, \$',
  'agent.settings.spendCap.off': 'No cap',
  'agent.settings.spendTotal': 'Spent so far: {usd}',
  'agent.settings.pricing': 'Prices as of {date}',
  'agent.settings.pricing.provider': 'Cost is reported by the provider.',

  // AI assistant — the chat tab.
  'agent.tab': 'Assistant',
  'agent.hint': 'Describe what the script should do…',
  'agent.send': 'Send',
  'agent.setup': 'Set up the AI assistant',
  'agent.setup.desc':
      'Describe what the script should do and the assistant edits it for you. '
          'It needs an API key of your own — takes a couple of minutes to get, '
          'and the setup screen walks you through it.',
  'agent.empty': 'Ask for a change to the script, or a question about it. '
      'The assistant reads the script off the VESC, edits it, and — once you '
      'confirm — flashes it and checks that it really runs.',
  // Shown once before the assistant can be used at all.
  'agent.disclaimer.title': 'Read this before using the assistant',
  'agent.disclaimer.p1':
      'The assistant is a language model. It gets things wrong, invents '
          'functions that do not exist, and misunderstands what you asked for '
          '— confidently, and without saying so.',
  'agent.disclaimer.p2':
      'What it writes runs on the motor controller of a vehicle you ride. A '
          'bad script can cut assist, apply current you did not expect, or '
          'leave the bike without brakes-by-wire behaviour you rely on.',
  'agent.disclaimer.p3':
      'Read every change before you confirm it. Test with the vehicle on a '
          'stand and the driven wheel off the ground. Never test on a road, '
          'and never while anyone is riding it.',
  'agent.disclaimer.p4':
      'Your conversation and your script are sent to the AI provider over the '
          'internet.',
  'agent.disclaimer.risk':
      'You use this entirely at your own risk. Nobody else is responsible for '
          'what the script does to your hardware or to you.',
  'agent.disclaimer.accept': 'I understand — continue',
  'agent.disclaimer.short':
      'The assistant can be wrong. Read every change, test on a stand with the '
          'wheel off the ground — at your own risk.',

  'agent.newSession': 'New chat',
  'agent.newSession.body':
      'Forget this conversation and start fresh? The script in the Code tab is '
          'not touched.',
  'agent.step': 'Step {n}/{max}',
  'agent.flashes': 'Flashes {n}/{max}',
  'agent.cached': '{pct}% cached',
  'agent.stop': 'Stop',
  'agent.stopping': 'Stopping after the current transfer…',
  'agent.stopScript': 'STOP SCRIPT',
  'agent.cancelled': 'Stopped.',
  'agent.stopped.script': 'The script on the VESC was stopped.',
  'agent.thinking': 'Thinking',
  'agent.paused.bg': 'Paused — the app is in the background.',
  'agent.working': 'Working…',
  'agent.strong.once': 'Use the strong model for this turn',

  'agent.confirm.flash': 'Flash to the VESC',
  'agent.confirm.flashRun': 'Flash and run',
  'agent.confirm.start': 'Start the script',
  'agent.confirm.revert': 'Restore the original script',
  'agent.confirm.approve': 'Confirm',
  'agent.confirm.cancel': 'Cancel',
  'agent.confirm.approved': 'Approved',
  'agent.confirm.declined': 'Declined',
  'agent.confirm.warnings': 'Warnings from the check:',
  'agent.confirm.runWarning':
      'The script will start immediately. Put the vehicle on a stand and keep '
          'clear of the drivetrain.',

  'agent.verify.ok': 'Verified — the script is running',
  'agent.verify.flashedNotRun': 'Written to flash, not started',
  'agent.verify.evalError': 'The script reported an error',
  'agent.verify.notRunning': 'The script does not appear to be running',
  'agent.verify.oomSuspect': 'Heap is climbing — possible out of memory',
  'agent.verify.cpuSaturated': 'Running, but CPU is pinned',
  'agent.verify.expectMissed': 'Expected output did not appear',
  'agent.verify.inconclusive': 'Could not verify on this link',
  'agent.verify.linkLost': 'Lost contact with the VESC after flashing',

  'agent.budget.steps': 'Step budget used up.',
  'agent.budget.tools': 'Tool-call budget used up.',
  'agent.budget.time': 'Session time limit reached.',
  'agent.budget.spend': 'Spend cap reached.',

  // Errors surfaced by the API client.
  'agent.err.internal': 'The assistant hit an internal error.',
  'agent.err.stopFailed': 'Could not stop the script.',
  'agent.err.auth': 'API key rejected. Check it in AI assistant settings.',
  'agent.err.balance': 'The account has no balance left.',
  'agent.err.ratelimit': 'Rate limited — retrying…',
  'agent.err.server': 'The AI service is having trouble. Retrying…',
  'agent.err.network': 'No connection to the AI service.',
  'agent.err.protocol': 'Unexpected response from the AI service.',
  'agent.err.nokey': 'Set an API key to use the assistant.',

  'lisp.err.noconn': 'No connection to the VESC adapter',
  'lisp.err.notarget': 'Pick an adapter to talk to the VESC.',
  'lisp.err.connect': 'Could not connect to the adapter',
  'lisp.err.nonus': 'That device has no VESC UART (NUS) service',
  'lisp.err.timeout': 'The VESC did not respond',
  'lisp.err.read': 'Could not read the script',
  'lisp.err.write': 'Upload failed',
  'lisp.err.erase': 'Could not erase old code',
  'lisp.err.unknown': 'LISP operation failed',
};

const _ru = <String, String>{
  'app.title': 'VESC Display Tool',

  'home.status.connected': 'Подключено',
  'home.status.disconnected': 'Не подключено',
  'home.state.idle': 'BLE простой',
  'home.state.scanning': 'Сканирую…',
  'home.state.connecting': 'Подключаюсь…',
  'home.state.connected': 'Соединение установлено',
  'home.state.disconnected':
      'Соединение потеряно, жду пока устройство появится в эфире…',
  'home.forget': 'Забыть устройство',
  'home.restart': 'Перезапустить BLE',
  'home.restart.toast': 'Перезапуск Bluetooth-соединения…',
  'home.forget.confirm.title': 'Забыть устройство?',
  'home.forget.confirm.body':
      'Авто-подключение отключится. Чтобы снова подключиться, придётся пройти '
      'pairing заново.',
  'home.forget.cancel': 'Отмена',
  'home.forget.ok': 'Забыть',

  'home.notif.title': 'Доступ к уведомлениям',
  'home.notif.granted': 'Разрешено',
  'home.notif.denied': 'Нужно разрешить в системных настройках',
  'home.batt.title': 'Без энергосбережения',
  'home.batt.granted': 'ОС не будет душить BLE-соединение',
  'home.batt.denied':
      'Без этого ОС отключит BLE через ~30 мин в фоне',
  'home.pairing.title': 'Pairing с head unit',
  'home.pairing.none': 'Устройство не выбрано',
  'home.pairing.saved': 'Сохранено: ',
  'home.filter.title': 'Какие приложения транслировать',
  'home.ios.title': 'iOS режим',
  'home.ios.body':
      'На iOS уведомления приложений не пересылаются (sandbox). '
      'Доступно только pairing и media-метаданные собственного приложения.',
  'home.lang.title': 'Язык',
  'home.test.title': 'Тестовая панель',

  'pairing.title': 'Поиск head unit',
  'pairing.empty':
      'Нажмите «обновить» чтобы начать сканирование. Head unit должен быть '
      'рядом и в режиме advertising.',
  'pairing.error': 'Ошибка: ',
  'pairing.unnamed': 'Безымянное',

  'filter.title': 'Приложения',
  'filter.search': 'Поиск…',

  'test.notif.section': 'Тест уведомлений',
  'test.media.section': 'Тест медиа',
  'test.notif.whatsapp': 'WhatsApp-стиль',
  'test.notif.telegram': 'Telegram-стиль',
  'test.notif.long': 'Длинный текст',
  'test.not_connected': 'Не подключено к head unit',
  'test.sent': 'Отправлено: ',
  'test.seek': '+30 сек',

  'lang.choose': 'Выбор языка',
  'lang.en': 'English',
  'lang.ru': 'Русский',

  'home.fw.title': 'Обновить прошивку устройства',
  'home.about.title': 'О приложении',
  'about.title': 'О приложении',
  'about.app': 'Версия приложения',
  'about.fw': 'Прошивка в комплекте',
  'about.desc': 'Мост уведомлений, медиа и времени с телефона на головное устройство ESP32-P4 по BLE; прошивка устройства по WiFi.',
  'about.source': 'Исходный код',
  'fw.title': 'Обновление прошивки',
  'fw.device': 'Версия на устройстве',
  'fw.detected': 'Определённое устройство',
  'fw.detected.unknown': 'Не определено',
  'fw.select': 'Прошивка для заливки',
  'fw.warn.undetected':
      'Не удалось определить плату устройства — выбери нужную прошивку вручную.',
  'fw.warn.mismatch':
      'Выбранная прошивка для другой платы, чем определена — прошивай только если уверен.',
  'fw.bundled': 'В приложении',
  'fw.unsupported': 'Это устройство не поддерживает обновление прошивки из приложения.',
  'fw.disconnected': 'Сначала подключитесь к устройству по Bluetooth.',
  'fw.reading': 'Чтение информации с устройства…',
  'fw.uptodate': 'На устройстве уже эта версия.',
  'fw.flash': 'Прошить',
  'fw.flashing': 'Обновление…',
  'fw.warn.title': 'Прошить устройство?',
  'fw.warn.body': 'Прошивка, зашитая в приложение, будет залита на устройство. Не выключай его до конца — по завершении оно перезагрузится.',
  'fw.host': 'Адрес устройства',
  'fw.host.note': 'Убедись, что телефон в WiFi устройства.',
  'fw.method': 'Способ обновления',
  'fw.method.ble': 'Bluetooth',
  'fw.method.wifi': 'WiFi',
  'fw.method.ble.note':
      'Прошивка по уже установленному Bluetooth — WiFi не нужен. Медленнее (несколько минут); не закрывай приложение.',
  'fw.method.ble.unsupported':
      'Прошивка устройства не умеет обновляться по Bluetooth — обнови по WiFi.',
  'fw.warn.go': 'Прошить',
  'fw.cancel': 'Отмена',
  'fw.done': 'Прошивка залита — устройство перезагружается.',
  'fw.ota.uploading.wifi': 'Загрузка прошивки ({size} КБ)…',
  'fw.ota.uploading.ble': 'Передача прошивки по Bluetooth ({size} КБ)…',
  'fw.ota.verifying': 'Проверка образа…',
  'fw.ota.verifying.device': 'Проверка образа на устройстве…',
  'fw.ota.rejected': 'Устройство отклонило прошивку',
  'fw.ota.connfail': 'Не удалось подключиться к {host}: {err}',
  'fw.ota.blefail': 'Не удалось обновить по Bluetooth',
  'fw.ota.bleerror': 'Ошибка Bluetooth-обновления: {err}',
  'fw.ota.err.noconn': 'Нет Bluetooth-соединения с устройством',
  'fw.ota.err.badsha': 'Некорректная контрольная сумма образа',
  'fw.ota.err.linklost': 'Bluetooth-соединение потеряно',
  'fw.ota.err.timeout': 'Устройство не ответило вовремя',
  'fw.ota.err.nopart': 'На устройстве нет свободного раздела для обновления',
  'fw.ota.err.size': 'Образ не помещается в раздел устройства',
  'fw.ota.err.alloc': 'На устройстве не хватило памяти',
  'fw.ota.err.sha': 'Ошибка контрольной суммы — передача повредилась, повторите',
  'fw.ota.err.begin': 'Не удалось подготовить flash устройства',
  'fw.ota.err.write': 'Ошибка записи во flash устройства',
  'fw.ota.err.end': 'Образ не прошёл проверку на устройстве',
  'fw.ota.err.boot': 'Не удалось переключить загрузочный раздел',
  'fw.ota.err.proto': 'Сбой передачи — повторите обновление',
  'fw.ota.err.unknown': 'Ошибка устройства',
  'fw.manual.hint': 'Не удалось получить данные WiFi с устройства — введите вручную.',
  'fw.manual.ssid': 'Имя WiFi устройства (SSID)',
  'fw.manual.password': 'Пароль WiFi устройства',
  'fw.manual.needfields': 'Введите имя и пароль WiFi.',
  'fw.manual.current': 'Телефон в WiFi: ',
  'fw.manual.current.none': 'не подключён к WiFi',
  'fw.manual.scan': 'Поиск',
  'fw.manual.pick': 'Выберите WiFi устройства',
  'fw.manual.noscan': 'Сети не найдены — включена ли геолокация?',

  'home.files.title': 'Файлы устройства',
  'files.title': 'Файлы устройства',
  'files.unsupported': 'Прошивка этой магнитолы не поддерживает просмотр файлов.',
  'files.empty': 'Папка пуста',
  'files.loading': 'Загрузка…',
  'files.dir': 'Папка',
  'files.notready': 'Хранилище готовится (при первом запуске форматируется, ~1 мин).',
  'files.notready.retry': 'Повторить',
  'files.upload': 'Загрузить файл',
  'files.uploading': 'Загрузка…',
  'files.mkdir': 'Новая папка',
  'files.mkdir.name': 'Имя папки',
  'files.download': 'Скачать',
  'files.downloading': 'Скачивание…',
  'files.saved': 'Сохранено в: ',
  'files.rename': 'Переименовать',
  'files.rename.to': 'Новое имя',
  'files.delete': 'Удалить',
  'files.delete.confirm': 'Удалить безвозвратно?',
  'files.cancel': 'Отмена',
  'files.ok': 'OK',
  'files.truncated': 'Список обрезан — слишком много записей.',
  'files.err.noconn': 'Нет связи с магнитолой.',
  'files.err.notready': 'Хранилище ещё монтируется — повторите чуть позже.',
  'files.err.badpath': 'Недопустимый путь.',
  'files.err.noent': 'Файл или папка не найдены.',
  'files.err.notdir': 'Это не папка.',
  'files.err.isdir': 'Это папка.',
  'files.err.nospc': 'Недостаточно свободного места на устройстве.',
  'files.err.exist': 'Уже существует.',
  'files.err.io': 'Ошибка ввода-вывода.',
  'files.err.sha': 'Несовпадение контрольной суммы — передача повреждена.',
  'files.err.proto': 'Ошибка протокола.',
  'files.err.alloc': 'Недостаточно памяти на устройстве.',
  'files.err.busy': 'Устройство занято другой операцией.',
  'files.err.toobig': 'Файл слишком велик для хранилища устройства.',
  'files.err.unknown': 'Неизвестная ошибка.',

  'home.splash.title': 'Заставка загрузки',
  'splash.title': 'Заставка загрузки',
  'splash.intro':
      'Выберите GIF: приложение нарежет его на JPEG-кадры, которые магнитола '
          'проигрывает при старте через аппаратный декодер. Простой GIF можно '
          'оставить как запасной вариант.',
  'splash.set_animated': 'Анимация из GIF',
  'splash.set_animated.sub': 'Нарезать GIF на кадры и загрузить (рекомендуется)',
  'splash.set_gif': 'Простой GIF (запасной)',
  'splash.set_gif.sub': 'Загрузить сырой GIF — проигрывается медленным декодером',
  'splash.building': 'Нарезаю GIF на кадры…',
  'splash.clearing': 'Готовлю папку…',
  'splash.uploading': 'Загружаю кадры…',
  'splash.done': 'Заставка обновлена — перезагрузите магнитолу, чтобы увидеть.',
  'splash.reboot_hint': 'Перезагрузите магнитолу, чтобы увидеть новую заставку.',
  'splash.err.decode': 'Не удалось прочитать этот GIF.',

  'home.display.title': 'Настройки дисплея',
  'home.helper.title': 'Настройки BLE-хелпера',
  'home.helper.subtitle': 'Мост ESP32-C3: кнопки, датчик каденса, PAS',
  'home.lisp.title': 'Редактор LISP-скрипта',

  // VESC BLE Helper (ESP32-C3).
  'helper.title': 'BLE-хелпер',
  'helper.tab.status': 'Состояние',
  'helper.tab.params': 'Параметры',
  'helper.tab.bind': 'Привязка',
  'helper.tab.fw': 'Прошивка',
  'helper.connect': 'Подключить',
  'helper.disconnect': 'Отключить',
  'helper.searching': 'Ищу хелпер…',
  'helper.connecting': 'Подключение…',
  'helper.notFound': 'Хелпер не найден. Включите его и попробуйте снова.',
  'helper.disconnected': 'Не подключён',
  'helper.connected': 'Подключён',
  'helper.fw': 'Прошивка {v}',
  'helper.log': 'Журнал',
  'helper.intro': 'Хелпер связывает BLE-кнопки и датчик каденса с VESC по '
      'CAN. Подключитесь, чтобы настроить его.',

  // Вкладка состояния.
  'helper.status.cadence': 'Каденс',
  'helper.status.rpm': 'об/мин',
  'helper.status.offline': 'датчик офлайн',
  'helper.status.assist': 'Ассист',
  'helper.status.level': 'Уровень',
  'helper.status.battery': 'Батарея',
  'helper.status.links': 'Связи',
  'helper.status.sensor': 'Датчик',
  'helper.status.remote': 'Пульт',
  'helper.status.vesc': 'VESC',
  'helper.status.pas': 'PAS',
  'helper.status.bound': 'привязан',
  'helper.status.notBound': 'не привязан',
  'helper.status.online': 'на связи',
  'helper.status.linkOk': 'связь есть',
  'helper.status.noData': 'нет данных',
  'helper.status.on': 'вкл',
  'helper.status.off': 'выкл',
  'helper.status.buttons': 'Кнопки пульта',
  'helper.status.noButtons':
      'Кнопки ещё не изучены — нажмите любую на привязанном пульте.',
  'helper.status.throttle': 'Ручное управление газом',
  'helper.status.toggle': 'Переключить',
  'helper.status.noVescWarn':
      'Нет связи с VESC — команда может не дойти до мотора.',
  'helper.status.waiting': 'Жду данные…',

  // Вкладка параметров.
  'helper.params.can': 'Шина CAN',
  'helper.params.helperId': 'ID хелпера',
  'helper.params.vescId': 'ID VESC',
  'helper.params.bitrate': 'Скорость, кбит/с',
  'helper.params.pas': 'Педальный ассист',
  'helper.params.enabled': 'PAS включён',
  'helper.params.reverse': 'Датчик наоборот',
  'helper.params.reverse.desc':
      'Включите, если ассист срабатывает при обратном ходе педалей',
  'helper.params.mode': 'Режим',
  'helper.params.mode.switch': 'Ступенчатый (уровни)',
  'helper.params.mode.prop': 'Пропорциональный (каденс)',
  'helper.params.levelCount': 'Число уровней',
  'helper.params.level': 'Уровень ассиста',
  'helper.params.level.off': '0 — выкл',
  'helper.params.startCurrent': 'Стартовый ток, %',
  'helper.params.startDelay': 'Задержка старта, мс',
  'helper.params.stopDelay': 'Задержка стопа, мс',
  'helper.params.minCadence': 'Мин. каденс, об/мин',
  'helper.params.fullCadence': 'Полный каденс, об/мин',
  'helper.params.maxCurrent': 'Макс. ток, А',
  'helper.params.ramp': 'Нарастание, А/с',
  'helper.params.buttons': 'CAN-команды кнопок',
  'helper.params.button': 'Кнопка {n}',
  'helper.params.canId': 'CAN ID (hex)',
  'helper.params.canData': 'данные (hex)',
  'helper.params.buttonsHint':
      'Кнопки изучаются по первому нажатию на ВСЕХ привязанных пультах '
          '(первая нажатая = A, следующая = B, …). Что делает кадр — решает '
          'LISP-скрипт на VESC.',
  'helper.params.read': 'Прочитать',
  'helper.params.write': 'Записать',
  'helper.params.written': 'Параметры записаны',
  'helper.params.badValue': 'Проверьте значения полей',
  'helper.params.waiting': 'Читаю параметры…',

  // Вкладка привязки.
  'helper.bind.scanButton': 'Искать кнопку',
  'helper.bind.scanCadence': 'Искать датчик каденса',
  'helper.bind.scanning': 'Хелпер ищет около 6 с…',
  'helper.bind.empty': 'Пока ничего не найдено',
  'helper.bind.emptyHint': 'Разбудите устройство и запустите поиск выше.',
  'helper.bind.unnamed': '(без имени)',
  'helper.bind.button': 'кнопка',
  'helper.bind.cadence': 'каденс',
  'helper.bind.bind': 'Привязать выбранное',
  'helper.bind.bound': 'Привязано',
  'helper.bind.unbindCadence': 'Отвязать датчик',
  'helper.bind.unbindButtons': 'Отвязать все пульты',

  // Вкладка прошивки.
  'helper.fw.installed': 'На хелпере: {v}',
  'helper.fw.bundled': 'В приложении: {v}',
  'helper.fw.latest': 'Онлайн есть новее: {v}',
  'helper.fw.unknown': 'неизвестно',
  'helper.fw.check': 'Проверить',
  'helper.fw.checking': 'Спрашиваю GitHub…',
  'helper.fw.downloading': 'Скачиваю…',
  'helper.fw.upToDate': 'Хелпер актуален.',
  'helper.fw.available': 'Доступна более новая версия.',
  'helper.fw.flash': 'Прошить',
  'helper.fw.flashing': 'Прошиваю… ничего не выключайте',
  'helper.fw.done': 'Прошито — хелпер перезагружается',
  'helper.fw.warn':
      'После прошивки хелпер перезагрузится и связь пропадёт; подключитесь '
          'заново, когда он поднимется.',
  'helper.err.busy': 'Передача уже идёт',
  'lisp.editor.title': 'LISP-скрипт',
  'lisp.editor.hint': 'LISP-код (LispBM) для VESC…',
  'lisp.tab.code': 'Код',
  'lisp.tab.vars': 'Переменные',
  'lisp.open': 'Открыть файл',
  'lisp.save': 'Сохранить файл',
  'lisp.read': 'Прочитать',
  'lisp.upload': 'Залить',
  'lisp.uploadRun': 'Залить и запустить',
  'lisp.run': 'Запустить',
  'lisp.stop': 'Остановить',
  'lisp.loaded': 'Скрипт прочитан с VESC',
  'lisp.uploaded': 'Залито на VESC',
  'lisp.uploadedRun': 'Залито и запущено',
  'lisp.opened': 'Файл загружен',
  'lisp.saved': 'Файл сохранён',
  'lisp.gaugesPaused':
      'Чтение/заливка ненадолго приостанавливают приборы на дашборде.',
  'lisp.vars.waiting': 'Ожидание переменных…',
  'lisp.vars.none': 'Нет переменных',

  // Структурная проверка скрипта (правила, которые на железе отказывают молча).
  'lisp.lint.check': 'Проверить',
  'lisp.lint.title': 'Проверка скрипта',
  'lisp.lint.clean': 'Проблем не найдено.',
  'lisp.lint.summary': 'Ошибок: {e}, предупреждений: {w} · {kb} КБ упаковано',
  'lisp.lint.close': 'Закрыть',

  // Асинхронный вывод `(print ...)` скрипта, работающего на VESC.
  'lisp.console.title': 'Вывод устройства',
  'lisp.console.empty': 'Скрипт пока ничего не вывел.',
  'lisp.console.dead': 'По этому каналу VESC не прислал ни одной строки. '
      'На прямом адаптере вывод может уходить в головное устройство.',
  'lisp.console.clear': 'Очистить',
  'lisp.console.copy': 'Копировать',
  'lisp.console.copied': 'Вывод скопирован',
  'lisp.console.raw': 'Сырые пакеты',
  'lisp.console.dropped': 'Потеряно предыдущих строк: {n}',

  // Через какой NUS-канал работает редактор: мост головного устройства или
  // отдельный BLE-адаптер VESC, к которому приложение подключается само.
  'lisp.adapter.pick': 'Выбор адаптера',
  'lisp.adapter.change': 'Изменить',
  'lisp.adapter.headunit': 'Головное устройство (встроенный мост)',
  'lisp.adapter.headunit.on': 'Подключено — передаёт на VESC по CAN',
  'lisp.adapter.headunit.off': 'Головное устройство не подключено',
  'lisp.adapter.none': 'Адаптер не выбран',
  'lisp.adapter.scan': 'Искать',
  'lisp.adapter.scanning': 'Поиск…',
  'lisp.adapter.empty':
      'Адаптер VESC не найден. Включите его и повторите поиск.',
  'lisp.adapter.showAll': 'Показать все устройства',
  'lisp.adapter.nusHint': 'Иначе в списке только устройства с UART-сервисом '
      'VESC.',
  'lisp.state.idle': 'Не подключено',
  'lisp.state.connecting': 'Подключение…',
  'lisp.state.connected': 'Подключено',
  'lisp.state.failed': 'Связь потеряна — переподключаюсь',

  // ИИ-ассистент — настройки. (Ключи самого чата — ниже.)
  'agent.settings.title': 'ИИ-ассистент',
  'agent.settings.subtitle': 'Правка LISP-скрипта языковой моделью',
  'agent.settings.provider': 'Провайдер',
  'agent.settings.keyFrom': 'Ключ можно получить на {url}',

  // Короткая инструкция прямо над полем ключа.
  'agent.settings.howto': 'Где взять ключ',
  'agent.settings.howto.1':
      'Откройте {url} и войдите (подойдёт Google или GitHub).',
  'agent.settings.howto.2':
      'Keys → Create key, название любое, скопируйте значение — его показывают '
          'один раз.',
  'agent.settings.howto.3':
      'Пополните счёт на пару долларов в разделе Credits. Сессия здесь обычно '
          'стоит заметно меньше цента.',
  'agent.settings.howto.4': 'Вставьте ключ ниже и нажмите ✓.',
  'agent.settings.howto.tapCopy': 'Нажмите на адрес, чтобы скопировать.',
  'agent.settings.howto.copied': 'Адрес скопирован',
  'agent.settings.key': 'Ваш API-ключ',
  'agent.settings.key.hint': 'Пусто — будет использован встроенный',
  'agent.settings.key.set': 'Ключ сохранён',
  'agent.settings.key.cleared': 'Ключ удалён',
  'agent.settings.key.stored': 'Хранится в системном хранилище ключей.',
  'agent.settings.key.embedded':
      'Работает встроенный в сборку ключ — настраивать ничего не нужно. '
          'Введите свой выше, чтобы платить со своего счёта. Учтите: '
          'встроенный ключ достаётся из APK, не раздавайте эту сборку.',
  'agent.settings.key.own': 'Используется ваш ключ.',
  'agent.settings.key.remember': 'Запомнить ключ',
  'agent.settings.key.rememberOff':
      'Выключено: ключ живёт только в памяти, при следующем запуске нужно '
          'ввести заново.',
  'agent.settings.key.tip':
      'Заведите для приложения отдельный ключ с лимитом трат.',
  'agent.settings.model': 'Модель',
  'agent.settings.model.default': 'По умолчанию: {m}',
  'agent.settings.model.fast': 'Быстрая',
  'agent.settings.model.strong': 'Сильная',
  'agent.settings.advanced': 'Дополнительно',
  'agent.settings.baseUrl': 'Адрес API',
  'agent.settings.strict': 'Строгие схемы инструментов',
  'agent.settings.strict.desc':
      'API гарантирует форму аргументов инструментов.',
  'agent.settings.thinking': 'Режим размышления',
  'agent.settings.thinking.desc':
      'Медленнее и дороже на шаг; лучше на сложных задачах.',
  'agent.settings.temperature': 'Температура',
  'agent.settings.budget': 'Лимиты на сессию',
  'agent.settings.maxSteps': 'Ходов модели',
  'agent.settings.maxFlashes': 'Прошивок VESC',
  'agent.settings.spendCap': 'Лимит трат, \$',
  'agent.settings.spendCap.off': 'Без лимита',
  'agent.settings.spendTotal': 'Всего потрачено: {usd}',
  'agent.settings.pricing': 'Цены на {date}',
  'agent.settings.pricing.provider': 'Стоимость сообщает провайдер.',

  // ИИ-ассистент — вкладка чата.
  'agent.tab': 'Ассистент',
  'agent.hint': 'Опишите, что должен делать скрипт…',
  'agent.send': 'Отправить',
  'agent.setup': 'Настроить ИИ-ассистента',
  'agent.setup.desc':
      'Опишите словами, что должен делать скрипт, — ассистент его поправит. '
          'Нужен свой API-ключ: получается за пару минут, на экране настройки '
          'есть пошаговая инструкция.',
  'agent.empty': 'Попросите изменить скрипт или задайте вопрос о нём. '
      'Ассистент прочитает скрипт с VESC, поправит его и — после вашего '
      'подтверждения — зальёт и проверит, что он действительно работает.',
  // Показывается один раз, до первого использования ассистента.
  'agent.disclaimer.title': 'Прочитайте, прежде чем пользоваться ассистентом',
  'agent.disclaimer.p1':
      'Ассистент — языковая модель. Он ошибается, выдумывает несуществующие '
          'функции и неверно понимает задачу — уверенно и не предупреждая '
          'об этом.',
  'agent.disclaimer.p2':
      'То, что он пишет, исполняется на контроллере мотора техники, на '
          'которой вы ездите. Плохой скрипт может убрать ассист, подать '
          'неожиданный ток или лишить вас поведения тормозов, на которое вы '
          'рассчитываете.',
  'agent.disclaimer.p3':
      'Читайте каждое изменение перед подтверждением. Проверяйте на '
          'подставке, с вывешенным ведущим колесом. Никогда не проверяйте на '
          'дороге и никогда — когда на технике кто-то сидит.',
  'agent.disclaimer.p4':
      'Ваша переписка и ваш скрипт уходят провайдеру ИИ через интернет.',
  'agent.disclaimer.risk':
      'Вы пользуетесь этим исключительно на свой страх и риск. За то, что '
          'скрипт сделает с вашим железом и с вами, никто другой '
          'ответственности не несёт.',
  'agent.disclaimer.accept': 'Понимаю — продолжить',
  'agent.disclaimer.short':
      'Ассистент может ошибаться. Читайте каждое изменение, проверяйте на '
          'подставке с вывешенным колесом — на свой страх и риск.',

  'agent.newSession': 'Новый чат',
  'agent.newSession.body':
      'Забыть эту переписку и начать заново? Скрипт во вкладке Код не '
          'изменится.',
  'agent.step': 'Шаг {n}/{max}',
  'agent.flashes': 'Прошивок {n}/{max}',
  'agent.cached': '{pct}% из кеша',
  'agent.stop': 'Стоп',
  'agent.stopping': 'Останавливаюсь после текущей передачи…',
  'agent.stopScript': 'ОСТАНОВИТЬ СКРИПТ',
  'agent.cancelled': 'Остановлено.',
  'agent.stopped.script': 'Скрипт на VESC остановлен.',
  'agent.thinking': 'Размышление',
  'agent.paused.bg': 'Пауза — приложение свёрнуто.',
  'agent.working': 'Работаю…',
  'agent.strong.once': 'Сильная модель для этого хода',

  'agent.confirm.flash': 'Залить на VESC',
  'agent.confirm.flashRun': 'Залить и запустить',
  'agent.confirm.start': 'Запустить скрипт',
  'agent.confirm.revert': 'Вернуть исходный скрипт',
  'agent.confirm.approve': 'Подтвердить',
  'agent.confirm.cancel': 'Отмена',
  'agent.confirm.approved': 'Подтверждено',
  'agent.confirm.declined': 'Отклонено',
  'agent.confirm.warnings': 'Предупреждения проверки:',
  'agent.confirm.runWarning':
      'Скрипт запустится сразу. Поставьте технику на подставку и держитесь '
          'в стороне от привода.',

  'agent.verify.ok': 'Проверено — скрипт работает',
  'agent.verify.flashedNotRun': 'Записано во флеш, не запущено',
  'agent.verify.evalError': 'Скрипт сообщил об ошибке',
  'agent.verify.notRunning': 'Похоже, скрипт не работает',
  'agent.verify.oomSuspect': 'Куча растёт — возможна нехватка памяти',
  'agent.verify.cpuSaturated': 'Работает, но процессор загружен под завязку',
  'agent.verify.expectMissed': 'Ожидаемый вывод не появился',
  'agent.verify.inconclusive': 'На этом канале проверить не удалось',
  'agent.verify.linkLost': 'После заливки связь с VESC пропала',

  'agent.budget.steps': 'Лимит шагов исчерпан.',
  'agent.budget.tools': 'Лимит вызовов инструментов исчерпан.',
  'agent.budget.time': 'Достигнут лимит времени сессии.',
  'agent.budget.spend': 'Достигнут лимит трат.',

  // Ошибки клиента API.
  'agent.err.internal': 'Внутренняя ошибка ассистента.',
  'agent.err.stopFailed': 'Не удалось остановить скрипт.',
  'agent.err.auth': 'API-ключ отклонён. Проверьте его в настройках ассистента.',
  'agent.err.balance': 'На счету закончились средства.',
  'agent.err.ratelimit': 'Слишком много запросов — повтор…',
  'agent.err.server': 'Сервис ИИ не отвечает как надо. Повтор…',
  'agent.err.network': 'Нет связи с сервисом ИИ.',
  'agent.err.protocol': 'Неожиданный ответ от сервиса ИИ.',
  'agent.err.nokey': 'Укажите API-ключ, чтобы пользоваться ассистентом.',

  'lisp.err.noconn': 'Нет связи с адаптером VESC',
  'lisp.err.notarget': 'Выберите адаптер для связи с VESC.',
  'lisp.err.connect': 'Не удалось подключиться к адаптеру',
  'lisp.err.nonus': 'У этого устройства нет UART-сервиса VESC (NUS)',
  'lisp.err.timeout': 'VESC не ответил',
  'lisp.err.read': 'Не удалось прочитать скрипт',
  'lisp.err.write': 'Ошибка заливки',
  'lisp.err.erase': 'Не удалось стереть старый код',
  'lisp.err.unknown': 'Ошибка операции LISP',
};

const _tables = <String, Map<String, String>>{
  'en': _en,
  'ru': _ru,
};

const supportedLocales = [Locale('en'), Locale('ru')];

class LocaleNotifier extends ChangeNotifier {
  static const _prefKey = 'app_locale_v1';

  Locale _locale = const Locale('en');
  Locale get locale => _locale;

  Future<void> load() async {
    final p = await SharedPreferences.getInstance();
    final code = p.getString(_prefKey);
    if (code != null && _tables.containsKey(code)) {
      _locale = Locale(code);
    }
  }

  Future<void> set(Locale l) async {
    if (l == _locale) return;
    _locale = l;
    final p = await SharedPreferences.getInstance();
    await p.setString(_prefKey, l.languageCode);
    notifyListeners();
  }
}

class LocaleScope extends InheritedNotifier<LocaleNotifier> {
  const LocaleScope({
    super.key,
    required LocaleNotifier super.notifier,
    required super.child,
  });

  static LocaleNotifier of(BuildContext context) {
    final scope = context.dependOnInheritedWidgetOfExactType<LocaleScope>();
    assert(scope?.notifier != null, 'LocaleScope missing above MaterialApp');
    return scope!.notifier!;
  }
}

/// Returns the translation for [key] in the currently active locale.
/// Falls back to English if the key is missing in the active table,
/// then to the key itself so missing entries surface visibly.
String t(BuildContext context, String key) {
  final l = LocaleScope.of(context).locale.languageCode;
  return _tables[l]?[key] ?? _en[key] ?? key;
}
