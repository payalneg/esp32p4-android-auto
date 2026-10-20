package com.aabridge.aa_bridge

import android.annotation.SuppressLint
import android.content.Context
import android.os.PowerManager

/** Keep the CPU awake for a working link, never while waiting for a bike. */
object LinkPower {
    private var lock: PowerManager.WakeLock? = null
    val connected: Boolean get() = lock?.isHeld == true

    @SuppressLint("WakelockTimeout")
    fun setConnected(ctx: Context, connected: Boolean) {
        if (connected) {
            if (lock?.isHeld == true) return
            val power = ctx.getSystemService(Context.POWER_SERVICE) as PowerManager
            lock = power.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "aabridge:connected").apply {
                setReferenceCounted(false)
                acquire()
            }
        } else {
            release()
        }
    }

    fun release() {
        lock?.let { if (it.isHeld) it.release() }
        lock = null
    }
}
