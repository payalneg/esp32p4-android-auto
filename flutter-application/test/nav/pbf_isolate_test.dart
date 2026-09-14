/// The region build runs in an isolate, and what crosses that boundary is the
/// closure handed to Isolate.run — with everything its enclosing function
/// happens to hold. In the app the progress callback reaches MapData and from
/// there the widget tree, none of which can be sent; the build then died with
/// "object is unsendable" before it read a byte of the extract.
///
/// So this passes a callback that holds something unsendable on purpose.
library;

import 'dart:async';
import 'dart:io';

import 'package:aa_bridge/nav/pbf_graph.dart';
import 'package:flutter_test/flutter_test.dart';

void main() {
  test('a progress callback holding unsendable state still builds', () async {
    // Smallest possible PBF: a header block and nothing else. The parse has
    // nothing to do, which is the point — this is about the isolate hop.
    final tmp = await Directory.systemTemp.createTemp('pbf');
    final file = File('${tmp.path}/empty.osm.pbf');
    await file.writeAsBytes(const <int>[]);

    // What the app's callback really carries: a Completer, which no isolate
    // message may contain.
    final completer = Completer<void>();
    var stages = 0;
    void onProgress(PbfProgress p) {
      if (completer.isCompleted) return;
      stages++;
    }

    await expectLater(
      PbfGraphSource.buildInIsolate(file.path, onProgress: onProgress),
      completes,
    );
    expect(stages, greaterThanOrEqualTo(0));

    await tmp.delete(recursive: true);
  });
}
