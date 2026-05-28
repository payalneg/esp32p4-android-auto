package com.aabridge.aa_bridge

import android.content.ComponentName
import android.content.Context
import android.graphics.Bitmap
import android.media.MediaMetadata
import android.media.session.MediaController
import android.media.session.MediaSessionManager
import android.media.session.PlaybackState
import android.os.Handler
import android.os.Looper
import android.view.KeyEvent
import io.flutter.plugin.common.EventChannel
import java.io.ByteArrayOutputStream

/**
 * Reads metadata from the currently-active MediaSession and forwards
 * snapshots over an EventChannel. Requires the NotificationListener permission
 * since MediaSessionManager.getActiveSessions checks the same gate.
 */
object MediaListener {
    private var controllers: List<MediaController> = emptyList()
    private var sink: EventChannel.EventSink? = null
    private val main = Handler(Looper.getMainLooper())
    private var ticker: Runnable? = null
    private var lastSourcePackage: String? = null

    fun streamHandler(ctx: Context): EventChannel.StreamHandler =
        object : EventChannel.StreamHandler {
            override fun onListen(arguments: Any?, events: EventChannel.EventSink?) {
                sink = events
                attach(ctx)
            }
            override fun onCancel(arguments: Any?) {
                sink = null
                stopTicker()
                detach()
            }
        }

    fun control(ctx: Context, cmd: String) {
        val c = activeController() ?: return
        when (cmd) {
            "play" -> c.transportControls.play()
            "pause" -> c.transportControls.pause()
            "next" -> c.transportControls.skipToNext()
            "prev" -> c.transportControls.skipToPrevious()
            else -> {}
        }
    }

    private fun activeController(): MediaController? =
        controllers.firstOrNull {
            it.playbackState?.state == PlaybackState.STATE_PLAYING
        } ?: controllers.firstOrNull()

    private fun attach(ctx: Context) {
        val msm = ctx.getSystemService(Context.MEDIA_SESSION_SERVICE) as MediaSessionManager
        val cn = ComponentName(ctx, NotifListener::class.java)
        try {
            controllers = msm.getActiveSessions(cn)
            controllers.forEach { it.registerCallback(callback) }
        } catch (_: SecurityException) {
            controllers = emptyList()
        }
        msm.addOnActiveSessionsChangedListener({ list ->
            controllers.forEach { it.unregisterCallback(callback) }
            controllers = list ?: emptyList()
            controllers.forEach { it.registerCallback(callback) }
            pushSnapshot()
        }, cn)
        startTicker()
        pushSnapshot()
    }

    private fun detach() {
        controllers.forEach { it.unregisterCallback(callback) }
        controllers = emptyList()
    }

    private val callback = object : MediaController.Callback() {
        override fun onMetadataChanged(metadata: MediaMetadata?) = pushSnapshot()
        override fun onPlaybackStateChanged(state: PlaybackState?) = pushSnapshot()
        override fun onSessionDestroyed() = pushSnapshot()
    }

    private fun startTicker() {
        stopTicker()
        ticker = object : Runnable {
            override fun run() {
                pushSnapshot()
                main.postDelayed(this, 1000)
            }
        }.also { main.postDelayed(it, 1000) }
    }

    private fun stopTicker() {
        ticker?.let { main.removeCallbacks(it) }
        ticker = null
    }

    private fun pushSnapshot() {
        val s = sink ?: return
        val c = activeController()
        if (c == null) {
            s.success(emptySnapshot())
            return
        }
        val md = c.metadata
        val ps = c.playbackState
        val title = md?.getString(MediaMetadata.METADATA_KEY_TITLE) ?: ""
        val artist = md?.getString(MediaMetadata.METADATA_KEY_ARTIST) ?: ""
        val album = md?.getString(MediaMetadata.METADATA_KEY_ALBUM) ?: ""
        val dur = md?.getLong(MediaMetadata.METADATA_KEY_DURATION) ?: 0L
        val pos = ps?.position ?: 0L
        val playing = ps?.state == PlaybackState.STATE_PLAYING
        val srcPkg = c.packageName
        // Album art only when the source changed or art changed, to keep
        // the snapshot ticker cheap.
        val art = bitmapToPng(
            md?.getBitmap(MediaMetadata.METADATA_KEY_ALBUM_ART)
                ?: md?.getBitmap(MediaMetadata.METADATA_KEY_ART)
        )
        lastSourcePackage = srcPkg
        s.success(mapOf(
            "title" to title,
            "artist" to artist,
            "album" to album,
            "durationMs" to dur,
            "positionMs" to pos,
            "isPlaying" to playing,
            "sourceApp" to srcPkg,
            "albumArt" to art,
        ))
    }

    private fun emptySnapshot() = mapOf(
        "title" to "", "artist" to "", "album" to "",
        "durationMs" to 0, "positionMs" to 0, "isPlaying" to false,
        "sourceApp" to "", "albumArt" to null,
    )

    private fun bitmapToPng(b: Bitmap?): ByteArray? {
        if (b == null) return null
        // Limit album art to 200x200 to keep BLE transfer short.
        val scaled = if (b.width > 200 || b.height > 200)
            Bitmap.createScaledBitmap(b, 200, 200, true) else b
        val out = ByteArrayOutputStream()
        scaled.compress(Bitmap.CompressFormat.PNG, 100, out)
        return out.toByteArray()
    }

    // Helper for any future hardware-key relaying.
    @Suppress("unused")
    private fun keyEventDown(c: MediaController, key: Int) {
        c.dispatchMediaButtonEvent(KeyEvent(KeyEvent.ACTION_DOWN, key))
        c.dispatchMediaButtonEvent(KeyEvent(KeyEvent.ACTION_UP, key))
    }
}
