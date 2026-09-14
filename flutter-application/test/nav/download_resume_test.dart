/// A province is 190 MB, and before this the first dropped connection threw
/// all of it away. What the resume has to get right is the conversation with
/// the server: ask for the tail, and notice when the answer is not one.
library;

import 'dart:io';
import 'dart:math';

import 'package:aa_bridge/nav/map_data.dart';
import 'package:flutter_test/flutter_test.dart';

class _Server {
  _Server(this.body, {this.etag = 'v1'});

  List<int> body;
  String etag;
  bool honourRanges = true;
  String? lastRange;
  int partialAnswers = 0;
  int wholeAnswers = 0;
  late HttpServer _http;

  Uri get url => Uri.parse('http://127.0.0.1:${_http.port}/region.osm.pbf');

  Future<void> start() async {
    _http = await HttpServer.bind(InternetAddress.loopbackIPv4, 0);
    _http.listen((HttpRequest req) async {
      final range = req.headers.value(HttpHeaders.rangeHeader);
      final ifRange = req.headers.value(HttpHeaders.ifRangeHeader);
      lastRange = range;
      var from = 0;
      // A validator that no longer matches means the file changed underneath
      // us: a real server answers with the whole of the new one.
      if (honourRanges &&
          range != null &&
          range.startsWith('bytes=') &&
          (ifRange == null || ifRange == etag)) {
        from = int.parse(range.substring(6).split('-').first);
      }
      final slice = body.sublist(from.clamp(0, body.length));
      req.response.headers.set(HttpHeaders.etagHeader, etag);
      req.response.statusCode =
          from > 0 ? HttpStatus.partialContent : HttpStatus.ok;
      if (from > 0) {
        partialAnswers++;
        req.response.headers.set(HttpHeaders.contentRangeHeader,
            'bytes $from-${body.length - 1}/${body.length}');
      } else {
        wholeAnswers++;
      }
      req.response.headers.contentLength = slice.length;
      req.response.add(slice);
      await req.response.close();
    });
  }

  Future<void> stop() => _http.close(force: true);
}

void main() {
  late Directory tmp;
  late List<int> body;
  late _Server server;
  late HttpClient client;
  late File dest;

  List<int> bytes(int n, int seed) {
    final rnd = Random(seed);
    return List<int>.generate(n, (_) => rnd.nextInt(256));
  }

  setUp(() async {
    tmp = await Directory.systemTemp.createTemp('dl');
    body = bytes(300000, 7);
    server = _Server(body);
    await server.start();
    client = HttpClient();
    dest = File('${tmp.path}/region.osm.pbf');
  });

  tearDown(() async {
    client.close(force: true);
    await server.stop();
    await tmp.delete(recursive: true);
  });

  /// What an interrupted download leaves behind.
  Future<void> leaveHalfDownloaded(int n, {String? tag = 'v1'}) async {
    await File('${dest.path}.part').writeAsBytes(body.sublist(0, n));
    if (tag != null) {
      await File('${dest.path}.part.tag').writeAsString(tag);
    }
  }

  Future<void> fetch() =>
      MapData.instance.downloadTo(client, server.url, dest, (_, __) {});

  test('picks up where it stopped', () async {
    await leaveHalfDownloaded(120000);
    await fetch();

    expect(server.lastRange, 'bytes=120000-');
    expect(server.partialAnswers, 1);
    expect(await dest.readAsBytes(), body);
    expect(await File('${dest.path}.part').exists(), isFalse);
    expect(await File('${dest.path}.part.tag').exists(), isFalse);
  });

  test('starts over when the extract was rebuilt', () async {
    await leaveHalfDownloaded(90000, tag: 'yesterday');
    await fetch();

    expect(server.wholeAnswers, 1, reason: 'stale validator, whole file');
    expect(await dest.readAsBytes(), body);
  });

  test('starts over when a part file has no validator to check', () async {
    await leaveHalfDownloaded(90000, tag: null);
    await fetch();

    expect(server.lastRange, isNull, reason: 'nothing to resume against');
    expect(await dest.readAsBytes(), body);
  });

  test('gets the file even from a server that ignores ranges', () async {
    await leaveHalfDownloaded(50000);
    server.honourRanges = false;
    await fetch();

    expect(await dest.readAsBytes(), body);
  });

  test('a clean download still works and leaves no scraps', () async {
    await fetch();

    expect(await dest.readAsBytes(), body);
    expect(await File('${dest.path}.part').exists(), isFalse);
  });
}
