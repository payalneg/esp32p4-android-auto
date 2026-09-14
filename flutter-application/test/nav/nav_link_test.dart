/// Everything another app might hand us as "a place".
library;

import 'package:aa_bridge/nav/nav_link.dart';
import 'package:flutter_test/flutter_test.dart';

void main() {
  NavLink? u(String s) => parseNavLink(Uri.parse(s));

  void expectPoint(NavLink? l, double lat, double lon, {LinkIntent? intent}) {
    expect(l, isNotNull);
    expect(l!.point, isNotNull, reason: '$l');
    expect(l.point!.lat, closeTo(lat, 1e-6));
    expect(l.point!.lon, closeTo(lon, 1e-6));
    if (intent != null) expect(l.intent, intent);
  }

  group('geo: and google.navigation:', () {
    test('geo: with coordinates in the path', () {
      expectPoint(u('geo:50.0619,19.9368'), 50.0619, 19.9368, intent: LinkIntent.show);
      expectPoint(u('geo:50.0619,19.9368?z=17'), 50.0619, 19.9368);
      expectPoint(u('geo:50.0619,19.9368;u=35'), 50.0619, 19.9368);
    });

    test('geo: q= with coordinates and a label wins over a 0,0 path', () {
      final l = u('geo:0,0?q=50.0619,19.9368(Rynek%20G%C5%82%C3%B3wny)');
      expectPoint(l, 50.0619, 19.9368);
      expect(l!.query, isNull);
    });

    test('geo: q= with text is a search', () {
      final l = u('geo:0,0?q=Rynek+G%C5%82%C3%B3wny')!;
      expect(l.point, isNull);
      expect(l.query, 'Rynek Główny');
    });

    test('geo:0,0 with nothing else is nothing', () {
      expect(u('geo:0,0'), isNull);
      expect(u('geo:'), isNull);
    });

    test('google.navigation: means go there now', () {
      expectPoint(u('google.navigation:q=50.0619,19.9368'), 50.0619, 19.9368,
          intent: LinkIntent.navigate);
      final t = u('google.navigation:q=Rynek+Krakow&mode=b')!;
      expect(t.query, 'Rynek Krakow');
      expect(t.intent, LinkIntent.navigate);
      expect(u('google.navigation:mode=b'), isNull);
    });
  });

  group('Google Maps URLs', () {
    test('the directions link Maps shares (the one from the screenshot)', () {
      expectPoint(
          u('https://www.google.com/maps/dir/?api=1&destination=50.08273%2C19.95736'),
          50.08273, 19.95736, intent: LinkIntent.route);
    });

    test('a place link: the pin beats the viewport', () {
      final l = u('https://www.google.com/maps/place/Rynek+G%C5%82%C3%B3wny/'
          '@50.0700,19.9000,17z/data=!3m1!4b1!4m6!3m5!1s0x0:0x0!8m2!3d50.0616!4d19.9373');
      expectPoint(l, 50.0616, 19.9373, intent: LinkIntent.show);
    });

    test('a place link with only a viewport still gives a point', () {
      expectPoint(u('https://www.google.com/maps/@50.0616,19.9373,17z'), 50.0616, 19.9373);
    });

    test('a place by name only becomes a search', () {
      final l = u('https://www.google.com/maps/place/Rynek+G%C5%82%C3%B3wny/')!;
      expect(l.point, isNull);
      expect(l.query, 'Rynek Główny');
    });

    test('old-style q= and daddr=', () {
      expectPoint(u('https://maps.google.com/?q=50.0619,19.9368'), 50.0619, 19.9368);
      expectPoint(u('https://maps.google.com/maps?daddr=50.0619,19.9368'), 50.0619, 19.9368,
          intent: LinkIntent.route);
      expectPoint(u('https://www.google.com/maps/search/?api=1&query=50.0619%2C19.9368'),
          50.0619, 19.9368, intent: LinkIntent.show);
    });

    test('short links must be followed first', () {
      final l = u('https://maps.app.goo.gl/AbCdEf123')!;
      expect(l.point, isNull);
      expect(l.expand, Uri.parse('https://maps.app.goo.gl/AbCdEf123'));
    });
  });

  group('other map sites', () {
    test('OpenStreetMap: marker, else the map hash', () {
      expectPoint(
          u('https://www.openstreetmap.org/?mlat=50.0619&mlon=19.9368#map=17/50.07/19.90'),
          50.0619, 19.9368);
      expectPoint(u('https://www.openstreetmap.org/#map=17/50.0619/19.9368'), 50.0619, 19.9368);
      expect(u('https://osm.org/go/0Ok2iZ--')!.expand, isNotNull);
    });

    test('Yandex writes lon,lat', () {
      expectPoint(u('https://yandex.ru/maps/?pt=19.9368,50.0619&z=17'), 50.0619, 19.9368);
      expectPoint(u('https://yandex.com/maps/?whatshere%5Bpoint%5D=19.9368%2C50.0619'),
          50.0619, 19.9368);
      expectPoint(u('https://yandex.ru/maps/?rtext=50.00,19.90~50.0619,19.9368'),
          19.9368, 50.0619, intent: LinkIntent.route);
      expect(u('https://yandex.ru/maps/-/CDqwYZ3M')!.expand, isNotNull);
    });

    test('Apple Maps', () {
      expectPoint(u('https://maps.apple.com/?ll=50.0619,19.9368&q=Rynek'), 50.0619, 19.9368);
      expectPoint(u('https://maps.apple.com/?daddr=50.0619,19.9368'), 50.0619, 19.9368,
          intent: LinkIntent.route);
    });

    test('Waze', () {
      expectPoint(u('https://waze.com/ul?ll=50.0619,19.9368&navigate=yes'), 50.0619, 19.9368,
          intent: LinkIntent.route);
      expectPoint(u('https://www.waze.com/live-map/directions?to=ll.50.0619%2C19.9368'),
          50.0619, 19.9368, intent: LinkIntent.route);
      expect(u('https://waze.com/h/abc')!.expand, isNotNull);
    });

    test('2GIS writes lon,lat', () {
      expectPoint(u('https://2gis.ru/krakow?m=19.9368%2C50.0619%2F16'), 50.0619, 19.9368);
      expectPoint(u('https://2gis.pl/krakow/geo/70030076189178855/19.9368,50.0619'),
          50.0619, 19.9368);
    });

    test('anything else is not ours', () {
      expect(u('https://example.com/maps?q=50,19'), isNull);
      expect(u('mailto:x@y.z'), isNull);
    });
  });

  group('shared text', () {
    test('a link inside prose, with a full stop after it', () {
      final l = parseNavText('Otwiera Google Maps '
          'https://www.google.com/maps/dir/?api=1&destination=50.08273%2C19.95736.');
      expectPoint(l, 50.08273, 19.95736, intent: LinkIntent.route);
    });

    test('a geo URI inside text', () {
      expectPoint(parseNavText('see geo:50.0619,19.9368 tonight'), 50.0619, 19.9368);
    });

    test('bare coordinates, alone or in a sentence', () {
      expectPoint(parseNavText('50.0619, 19.9368'), 50.0619, 19.9368);
      expectPoint(parseNavText('be at 50.0619, 19.9368 by six'), 50.0619, 19.9368);
      expectPoint(parseNavText('point 50.0619 19.9368'), 50.0619, 19.9368);
    });

    test('nothing usable is null', () {
      expect(parseNavText('see you at 6'), isNull);
      expect(parseNavText('https://example.com/ price 12.50, 3.99'), isNull);
      expect(parseNavText(''), isNull);
    });
  });
}
