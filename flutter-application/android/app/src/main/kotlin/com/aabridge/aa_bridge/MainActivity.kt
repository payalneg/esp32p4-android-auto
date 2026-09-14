package com.aabridge.aa_bridge

import android.content.Intent
import io.flutter.embedding.android.FlutterActivity
import io.flutter.embedding.engine.FlutterEngine
import io.flutter.plugin.common.MethodChannel

class MainActivity : FlutterActivity() {
    /** Place the activity was launched with (a URI or shared text), until Dart collects it. */
    private var pendingNavUri: String? = null
    private var navChannel: MethodChannel? = null

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

        // "Navigate" / "open in maps" / "share" from other apps. A cold start
        // carries the payload in the launch intent; Dart asks for it once it
        // is up. A running activity gets onNewIntent and pushes it over.
        pendingNavUri = navUriOf(intent)
        navChannel = MethodChannel(flutterEngine.dartExecutor.binaryMessenger, "aabridge/nav.intent")
            .also { ch ->
                ch.setMethodCallHandler { call, result ->
                    when (call.method) {
                        "takeInitial" -> {
                            result.success(pendingNavUri)
                            pendingNavUri = null
                        }
                        else -> result.notImplemented()
                    }
                }
            }
    }

    override fun onNewIntent(intent: Intent) {
        super.onNewIntent(intent)
        val uri = navUriOf(intent) ?: return
        val ch = navChannel
        if (ch == null) pendingNavUri = uri else ch.invokeMethod("open", uri)
    }

    private fun navUriOf(intent: Intent?): String? = when (intent?.action) {
        Intent.ACTION_VIEW -> intent.data?.takeIf { it.scheme == "geo" || it.scheme == "google.navigation" }?.toString()
        // Shared text: Google Maps, Yandex, OSM and chats all put the link in EXTRA_TEXT.
        Intent.ACTION_SEND -> intent.getStringExtra(Intent.EXTRA_TEXT)?.takeIf { it.isNotBlank() }
        else -> null
    }
}
