package com.aabridge.aa_bridge

import android.content.ComponentName
import android.content.Context
import android.graphics.Bitmap
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.drawable.Icon
import android.os.Handler
import android.os.Looper
import android.provider.Settings
import android.service.notification.NotificationListenerService
import android.service.notification.StatusBarNotification
import android.text.TextUtils
import io.flutter.plugin.common.EventChannel
import java.io.ByteArrayOutputStream

class NotifListener : NotificationListenerService() {

    override fun onListenerConnected() {
        super.onListenerConnected()
        connected = true
        // Flush whatever is already on-screen so the head unit has fresh state.
        try {
            activeNotifications?.forEach { emit(it, removed = false) }
        } catch (_: Throwable) {}
    }

    override fun onListenerDisconnected() {
        connected = false
        super.onListenerDisconnected()
    }

    override fun onNotificationPosted(sbn: StatusBarNotification) = emit(sbn, false)
    override fun onNotificationRemoved(sbn: StatusBarNotification) = emit(sbn, true)

    private fun emit(sbn: StatusBarNotification, removed: Boolean) {
        val n = sbn.notification ?: return
        val extras = n.extras ?: return
        val title = extras.getCharSequence(android.app.Notification.EXTRA_TITLE)?.toString() ?: ""
        val text = extras.getCharSequence(android.app.Notification.EXTRA_TEXT)?.toString() ?: ""
        if (TextUtils.isEmpty(title) && TextUtils.isEmpty(text)) return
        val pm = packageManager
        val appName = try {
            pm.getApplicationLabel(pm.getApplicationInfo(sbn.packageName, 0)).toString()
        } catch (_: Throwable) { sbn.packageName }
        // During turn-by-turn navigation Google Maps puts the maneuver
        // arrow in the notification's *large* icon (a per-turn 126×126
        // bitmap) — the small icon is just the static nav-status resource.
        // Ship the rendered arrow so the head unit shows the real maneuver.
        // Only for category=navigation; everything else keeps the launcher
        // icon (resolved phone-side by package). Fall back to the small
        // icon if some nav app doesn't set a large icon.
        val category = n.category ?: ""
        val iconPng: ByteArray? =
            if (category == android.app.Notification.CATEGORY_NAVIGATION)
                renderIconPng(n.getLargeIcon()) ?: renderIconPng(n.smallIcon)
            else null
        val payload = mapOf(
            "id" to (sbn.key.hashCode().toLong() and 0xffffffffL),
            "package" to sbn.packageName,
            "appName" to appName,
            "title" to title,
            "text" to text,
            "postedAtMs" to sbn.postTime,
            "removed" to removed,
            "category" to category,
            "iconPng" to iconPng,
        )
        NotifEventStream.dispatch(payload)
    }

    /** Render an [Icon] (typically a navigation maneuver arrow) to a
     *  square white-on-transparent PNG sized for the head unit's toast
     *  icon slot. Maneuver small-icons are monochrome silhouettes meant
     *  for the status bar, so we force-tint white for legibility on the
     *  dark toast card. Returns null on any failure — the caller then
     *  falls back to the launcher icon. */
    private fun renderIconPng(icon: Icon?, sizePx: Int = 72): ByteArray? {
        if (icon == null) return null
        return try {
            val d = icon.loadDrawable(this) ?: return null
            val bmp = Bitmap.createBitmap(sizePx, sizePx, Bitmap.Config.ARGB_8888)
            val canvas = Canvas(bmp)
            d.setBounds(0, 0, sizePx, sizePx)
            d.setTint(Color.WHITE)
            d.draw(canvas)
            val out = ByteArrayOutputStream()
            bmp.compress(Bitmap.CompressFormat.PNG, 100, out)
            bmp.recycle()
            out.toByteArray()
        } catch (_: Throwable) {
            null
        }
    }

    companion object {
        @Volatile var connected = false

        fun isPermissionGranted(ctx: Context): Boolean {
            val flat = Settings.Secure.getString(
                ctx.contentResolver, "enabled_notification_listeners"
            ) ?: return false
            val self = ComponentName(ctx, NotifListener::class.java).flattenToString()
            return flat.split(":").any { it == self }
        }
    }
}

object NotifEventStream : EventChannel.StreamHandler {
    private var sink: EventChannel.EventSink? = null
    private val main = Handler(Looper.getMainLooper())
    private val backlog = ArrayDeque<Map<String, Any?>>()

    override fun onListen(arguments: Any?, events: EventChannel.EventSink?) {
        sink = events
        while (backlog.isNotEmpty()) {
            events?.success(backlog.removeFirst())
        }
    }
    override fun onCancel(arguments: Any?) { sink = null }

    fun dispatch(payload: Map<String, Any?>) {
        main.post {
            val s = sink
            if (s == null) {
                // Drop oldest when overflowing — we never want to grow unboundedly.
                if (backlog.size > 64) backlog.removeFirst()
                backlog.addLast(payload)
            } else {
                s.success(payload)
            }
        }
    }
}
