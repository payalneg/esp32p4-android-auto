/// Hosts the developer [TestPanel] on its own screen, tucked behind the home
/// AppBar overflow menu so it no longer clutters the main list.
library;

import 'package:flutter/material.dart';

import '../i18n/strings.dart';
import 'test_panel.dart';

class TestPanelScreen extends StatelessWidget {
  const TestPanelScreen({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: Text(t(context, 'home.test.title'))),
      body: ListView(
        padding: const EdgeInsets.all(16),
        children: const [TestPanel()],
      ),
    );
  }
}
