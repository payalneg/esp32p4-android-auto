package com.aabridge.aa_bridge

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.SharedPreferences
import android.util.Log

/**
 * Fires after the phone finishes booting (and after the app is replaced
 * via Play / sideload). Starts the BLE foreground service only when the
 * user has actually paired with a head unit — otherwise we'd hold a
 * persistent notification for nothing on a fresh install.
 *
 * SharedPreferences key mirrors [BleService._prefSavedId] on the Dart
 * side; Flutter writes it via shared_preferences which under the hood
 * is the same flutter.aa_bridge.* xml file we read here.
 */
class BootReceiver : BroadcastReceiver() {
    override fun onReceive(context: Context, intent: Intent) {
        val action = intent.action ?: return
        if (action != Intent.ACTION_BOOT_COMPLETED &&
            action != "android.intent.action.QUICKBOOT_POWERON" &&
            action != Intent.ACTION_MY_PACKAGE_REPLACED &&
            action != Intent.ACTION_LOCKED_BOOT_COMPLETED) return

        val saved = pairedRemoteId(context)
        if (saved.isNullOrEmpty()) {
            Log.i(TAG, "no paired head unit, skipping autostart")
            return
        }
        Log.i(TAG, "autostart for paired $saved (event=$action)")
        BleForegroundService.start(context)
    }

    private fun pairedRemoteId(ctx: Context): String? {
        // shared_preferences plugin stores values prefixed with "flutter."
        // in <package>_preferences.xml.
        val prefs: SharedPreferences = ctx.getSharedPreferences(
            "${ctx.packageName}_preferences", Context.MODE_PRIVATE
        )
        return prefs.getString("flutter.ble_saved_remote_id_v1", null)
    }

    companion object { private const val TAG = "BootReceiver" }
}
