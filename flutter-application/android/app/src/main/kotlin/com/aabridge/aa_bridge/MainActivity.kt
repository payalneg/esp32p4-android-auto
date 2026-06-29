package com.aabridge.aa_bridge

import io.flutter.embedding.android.FlutterActivity
import io.flutter.embedding.engine.FlutterEngine

class MainActivity : FlutterActivity() {
    override fun configureFlutterEngine(flutterEngine: FlutterEngine) {
        super.configureFlutterEngine(flutterEngine)
        // UI engine: command channels only (permission check, open settings,
        // install-app list). The notification/media EVENT streams + the BLE
        // pump live on the foreground-service background engine — see
        // AaBridgeApplication / AaBridgeChannels.
        AaBridgeChannels.register(
            this,
            flutterEngine.dartExecutor.binaryMessenger,
            includeEventChannels = false,
        )
    }
}
