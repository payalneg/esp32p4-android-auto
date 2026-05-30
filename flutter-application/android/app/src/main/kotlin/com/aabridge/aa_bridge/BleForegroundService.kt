package com.aabridge.aa_bridge

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.app.Service
import android.content.Context
import android.content.Intent
import android.content.pm.ServiceInfo
import android.os.Build
import android.os.IBinder
import androidx.core.app.NotificationCompat

/**
 * Background-lived foreground service that holds the app's process alive
 * while a BLE link to the head unit is open. We don't keep BLE state here —
 * flutter_blue_plus inside the Dart isolate owns the connection. The service
 * exists purely so Android doesn't kill the process when the UI is closed.
 *
 * The persistent notification doubles as a "status pill" in the system
 * shade — `updateStatus()` rebuilds it whenever the Dart side reports a
 * connection-state transition so the user can glance at the shade and
 * see "Connected to SuperVESCDisplay" or "Disconnected — retrying…"
 * without opening the app.
 */
class BleForegroundService : Service() {
    override fun onBind(intent: Intent?): IBinder? = null

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        val state = intent?.getStringExtra(EXTRA_STATE) ?: lastState
        val sub   = intent?.getStringExtra(EXTRA_SUBTITLE) ?: lastSubtitle
        lastState    = state
        lastSubtitle = sub
        startForegroundCompat(state, sub)
        return START_STICKY
    }

    private fun startForegroundCompat(state: String, subtitle: String) {
        val nm = getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O &&
            nm.getNotificationChannel(CHANNEL_ID) == null
        ) {
            val ch = NotificationChannel(
                CHANNEL_ID, "AA Bridge link", NotificationManager.IMPORTANCE_LOW
            )
            // No sound — this is a status pill, not a heads-up.
            ch.setSound(null, null)
            nm.createNotificationChannel(ch)
        }

        val connected = state == STATE_CONNECTED
        val icon = if (connected)
            android.R.drawable.stat_sys_data_bluetooth
        else
            android.R.drawable.stat_sys_warning

        val launch = packageManager.getLaunchIntentForPackage(packageName)
        val pi = if (launch != null) PendingIntent.getActivity(
            this, 0, launch,
            PendingIntent.FLAG_IMMUTABLE or PendingIntent.FLAG_UPDATE_CURRENT,
        ) else null

        val title = if (connected) "AA Bridge — connected"
        else "AA Bridge — disconnected"

        val n: Notification = NotificationCompat.Builder(this, CHANNEL_ID)
            .setContentTitle(title)
            .setContentText(subtitle.ifEmpty {
                if (connected) "Linked to head unit" else "Waiting for head unit…"
            })
            .setSmallIcon(icon)
            .setOngoing(true)
            .setOnlyAlertOnce(true)
            .setShowWhen(false)
            .also { b -> if (pi != null) b.setContentIntent(pi) }
            .build()

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.UPSIDE_DOWN_CAKE) {
            startForeground(
                NOTIF_ID, n,
                ServiceInfo.FOREGROUND_SERVICE_TYPE_CONNECTED_DEVICE,
            )
        } else {
            startForeground(NOTIF_ID, n)
        }
    }

    companion object {
        private const val CHANNEL_ID = "aabridge.link"
        private const val NOTIF_ID = 7242
        private const val EXTRA_STATE = "state"
        private const val EXTRA_SUBTITLE = "subtitle"

        const val STATE_CONNECTED    = "connected"
        const val STATE_DISCONNECTED = "disconnected"

        // Process-level so re-entering onStartCommand without extras
        // doesn't overwrite a fresh status with a stale boot default.
        @Volatile private var lastState: String = STATE_DISCONNECTED
        @Volatile private var lastSubtitle: String = ""

        fun start(ctx: Context) {
            val i = Intent(ctx, BleForegroundService::class.java)
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                ctx.startForegroundService(i)
            } else {
                ctx.startService(i)
            }
        }

        fun stop(ctx: Context) {
            ctx.stopService(Intent(ctx, BleForegroundService::class.java))
        }

        /** Called from Dart to push a fresh status into the shade. */
        fun updateStatus(ctx: Context, state: String, subtitle: String) {
            val i = Intent(ctx, BleForegroundService::class.java)
                .putExtra(EXTRA_STATE, state)
                .putExtra(EXTRA_SUBTITLE, subtitle)
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                ctx.startForegroundService(i)
            } else {
                ctx.startService(i)
            }
        }
    }
}
