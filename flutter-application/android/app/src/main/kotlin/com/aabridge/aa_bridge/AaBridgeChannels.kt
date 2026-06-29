package com.aabridge.aa_bridge

import android.content.Context
import android.content.Intent
import android.provider.Settings
import io.flutter.plugin.common.BinaryMessenger
import io.flutter.plugin.common.EventChannel
import io.flutter.plugin.common.MethodChannel

/**
 * Registers the AA-bridge platform channels on a FlutterEngine's messenger.
 *
 * Called for BOTH engines:
 *   - the UI engine (MainActivity) — command channels only, so the app can
 *     query notification permission, open settings and list installed apps.
 *   - the foreground-service background engine (see [AaBridgeApplication]) —
 *     where the BLE pump lives, so it ALSO gets the notification/media event
 *     streams + icon lookups.
 *
 * The notification/media EVENT channels back single-sink singletons
 * ([NotifEventStream], [MediaListener]); only ONE engine may own them. That's
 * the background engine — the UI engine passes [includeEventChannels] = false.
 */
object AaBridgeChannels {
    fun register(
        context: Context,
        messenger: BinaryMessenger,
        includeEventChannels: Boolean,
    ) {
        val appCtx = context.applicationContext

        MethodChannel(messenger, "aabridge/notifications.cmd")
            .setMethodCallHandler { call, result ->
                when (call.method) {
                    "hasPermission" ->
                        result.success(NotifListener.isPermissionGranted(appCtx))
                    "openPermissionSettings" -> {
                        // NEW_TASK so it works from an app (non-Activity) context.
                        appCtx.startActivity(
                            Intent(Settings.ACTION_NOTIFICATION_LISTENER_SETTINGS)
                                .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                        )
                        result.success(null)
                    }
                    "getIconPng" -> {
                        val pkg = call.argument<String>("package") ?: ""
                        result.success(IconExtractor.iconPng(appCtx, pkg))
                    }
                    "listInstalledApps" ->
                        result.success(IconExtractor.listLaunchable(appCtx))
                    else -> result.notImplemented()
                }
            }

        MethodChannel(messenger, "aabridge/media.cmd")
            .setMethodCallHandler { call, result ->
                when (call.method) {
                    "control" -> {
                        MediaListener.control(appCtx, call.argument<String>("cmd") ?: "")
                        result.success(null)
                    }
                    else -> result.notImplemented()
                }
            }

        if (includeEventChannels) {
            EventChannel(messenger, "aabridge/notifications")
                .setStreamHandler(NotifEventStream)
            EventChannel(messenger, "aabridge/media")
                .setStreamHandler(MediaListener.streamHandler(appCtx))
        }
    }
}
