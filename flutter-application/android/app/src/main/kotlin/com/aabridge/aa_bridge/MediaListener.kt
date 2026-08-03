package com.aabridge.aa_bridge

import android.content.ComponentName
import android.content.Context
import android.graphics.Bitmap
import android.graphics.Canvas
import android.graphics.Matrix
import android.graphics.Paint
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
    // True once we've already pushed a fully-empty snapshot. Stops the
    // 1 Hz ticker from continually emitting "" / "" / "", which would
    // overwrite anything the test panel (or another producer) sent
    // directly through BleService.sendMedia. We still want a *single*
    // empty push when a real session ends, so the head unit clears
    // itself — that's why we don't drop empties unconditionally.
    private var lastWasEmpty = false

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

    private var sessionsListener: MediaSessionManager.OnActiveSessionsChangedListener? = null
    private var msm: MediaSessionManager? = null

    private fun attach(ctx: Context) {
        val msm = ctx.getSystemService(Context.MEDIA_SESSION_SERVICE) as MediaSessionManager
        this.msm = msm
        val cn = ComponentName(ctx, NotifListener::class.java)
        try {
            controllers = msm.getActiveSessions(cn)
            controllers.forEach { it.registerCallback(callback) }
        } catch (_: SecurityException) {
            controllers = emptyList()
        }
        val listener = MediaSessionManager.OnActiveSessionsChangedListener { list ->
            controllers.forEach { it.unregisterCallback(callback) }
            controllers = list ?: emptyList()
            controllers.forEach { it.registerCallback(callback) }
            pushSnapshot()
        }
        sessionsListener = listener
        msm.addOnActiveSessionsChangedListener(listener, cn)
        startTicker()
        pushSnapshot()
    }

    private fun detach() {
        sessionsListener?.let { listener ->
            try {
                msm?.removeOnActiveSessionsChangedListener(listener)
            } catch (_: Exception) {}
        }
        sessionsListener = null
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
            // Push exactly one empty snapshot per "no session" transition.
            // Subsequent ticker runs while nothing's playing stay silent
            // so they don't clobber data fed in via the test panel /
            // direct BleService.sendMedia.
            if (!lastWasEmpty) {
                s.success(emptySnapshot())
                lastWasEmpty = true
            }
            return
        }
        lastWasEmpty = false
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
        // Produce a 352×136 JPEG sized exactly for the head unit's
        // music tile. JPEG (q85) for photographic content runs ~4-5×
        // smaller than PNG, and the P4 has a hardware JPEG decoder so
        // the head unit blits it once into a RGB565 framebuffer
        // instead of re-decoding per frame. Dimensions are 8-aligned
        // (P4 JPEG hardware accepts 8-aligned input and rounds its
        // RGB565 output up to 16 internally — the head unit allocates
        // a 352×144 output buffer to absorb the rounding); lv_img
        // 350×135 crops the small overhang. No alpha needed — album
        // art is opaque.
        val dstW = 344
        val dstH = 136
        val out = Bitmap.createBitmap(dstW, dstH, Bitmap.Config.ARGB_8888)
        val canvas = Canvas(out)
        val sx = dstW.toFloat() / b.width
        val sy = dstH.toFloat() / b.height
        val scale = if (sx > sy) sx else sy
        val drawnW = b.width  * scale
        val drawnH = b.height * scale
        val m = Matrix().apply {
            postScale(scale, scale)
            postTranslate((dstW - drawnW) / 2f, (dstH - drawnH) / 2f)
        }
        canvas.drawBitmap(b, m, Paint(Paint.FILTER_BITMAP_FLAG))
        val bytes = ByteArrayOutputStream()
        out.compress(Bitmap.CompressFormat.JPEG, 85, bytes)
        out.recycle()
        return bytes.toByteArray()
    }

    // Helper for any future hardware-key relaying.
    @Suppress("unused")
    private fun keyEventDown(c: MediaController, key: Int) {
        c.dispatchMediaButtonEvent(KeyEvent(KeyEvent.ACTION_DOWN, key))
        c.dispatchMediaButtonEvent(KeyEvent(KeyEvent.ACTION_UP, key))
    }
}
