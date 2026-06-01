package com.aabridge.aa_bridge

import android.content.Intent
import android.provider.Settings
import io.flutter.embedding.android.FlutterActivity
import io.flutter.embedding.engine.FlutterEngine
import io.flutter.plugin.common.EventChannel
import io.flutter.plugin.common.MethodChannel

class MainActivity : FlutterActivity() {
    override fun configureFlutterEngine(flutterEngine: FlutterEngine) {
        super.configureFlutterEngine(flutterEngine)
        val messenger = flutterEngine.dartExecutor.binaryMessenger

        EventChannel(messenger, "aabridge/notifications")
            .setStreamHandler(NotifEventStream)
        MethodChannel(messenger, "aabridge/notifications.cmd")
            .setMethodCallHandler { call, result ->
                when (call.method) {
                    "hasPermission" -> result.success(
                        NotifListener.isPermissionGranted(this)
                    )
                    "openPermissionSettings" -> {
                        startActivity(Intent(Settings.ACTION_NOTIFICATION_LISTENER_SETTINGS))
                        result.success(null)
                    }
                    "getIconPng" -> {
                        val pkg = call.argument<String>("package") ?: ""
                        result.success(IconExtractor.iconPng(this, pkg))
                    }
                    "listInstalledApps" -> {
                        result.success(IconExtractor.listLaunchable(this))
                    }
                    else -> result.notImplemented()
                }
            }

        EventChannel(messenger, "aabridge/media")
            .setStreamHandler(MediaListener.streamHandler(this))
        MethodChannel(messenger, "aabridge/media.cmd")
            .setMethodCallHandler { call, result ->
                when (call.method) {
                    "control" -> {
                        val cmd = call.argument<String>("cmd") ?: ""
                        MediaListener.control(this, cmd)
                        result.success(null)
                    }
                    else -> result.notImplemented()
                }
            }

        MethodChannel(messenger, "aabridge/foreground")
            .setMethodCallHandler { call, result ->
                when (call.method) {
                    "start" -> {
                        BleForegroundService.start(this); result.success(null)
                    }
                    "stop" -> {
                        BleForegroundService.stop(this); result.success(null)
                    }
                    "status" -> {
                        val state = call.argument<String>("state")
                            ?: BleForegroundService.STATE_DISCONNECTED
                        val sub = call.argument<String>("subtitle") ?: ""
                        BleForegroundService.updateStatus(this, state, sub)
                        result.success(null)
                    }
                    else -> result.notImplemented()
                }
            }
    }
}
