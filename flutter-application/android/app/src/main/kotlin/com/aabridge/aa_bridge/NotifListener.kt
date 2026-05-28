package com.aabridge.aa_bridge

import android.content.ComponentName
import android.content.Context
import android.os.Handler
import android.os.Looper
import android.provider.Settings
import android.service.notification.NotificationListenerService
import android.service.notification.StatusBarNotification
import android.text.TextUtils
import io.flutter.plugin.common.EventChannel

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
        val payload = mapOf(
            "id" to (sbn.key.hashCode().toLong() and 0xffffffffL),
            "package" to sbn.packageName,
            "appName" to appName,
            "title" to title,
            "text" to text,
            "postedAtMs" to sbn.postTime,
            "removed" to removed,
        )
        NotifEventStream.dispatch(payload)
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
