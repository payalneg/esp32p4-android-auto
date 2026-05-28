package com.aabridge.aa_bridge

import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.graphics.Bitmap
import android.graphics.Canvas
import android.graphics.drawable.BitmapDrawable
import android.graphics.drawable.Drawable
import java.io.ByteArrayOutputStream

object IconExtractor {
    fun iconPng(ctx: Context, pkg: String, size: Int = 96): ByteArray? {
        if (pkg.isBlank()) return null
        val pm = ctx.packageManager
        val drawable: Drawable = try {
            pm.getApplicationIcon(pkg)
        } catch (_: PackageManager.NameNotFoundException) {
            return null
        }
        val bmp = drawable.toBitmap(size, size)
        val out = ByteArrayOutputStream()
        bmp.compress(Bitmap.CompressFormat.PNG, 100, out)
        return out.toByteArray()
    }

    fun listLaunchable(ctx: Context): List<Map<String, String>> {
        val pm = ctx.packageManager
        val intent = Intent(Intent.ACTION_MAIN, null).apply {
            addCategory(Intent.CATEGORY_LAUNCHER)
        }
        return pm.queryIntentActivities(intent, 0)
            .map {
                mapOf(
                    "package" to it.activityInfo.packageName,
                    "label" to it.loadLabel(pm).toString(),
                )
            }
            .distinctBy { it["package"] }
            .sortedBy { it["label"]?.lowercase() }
    }

    private fun Drawable.toBitmap(w: Int, h: Int): Bitmap {
        if (this is BitmapDrawable && bitmap != null && intrinsicWidth == w && intrinsicHeight == h) {
            return bitmap
        }
        val bmp = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888)
        val canvas = Canvas(bmp)
        setBounds(0, 0, w, h)
        draw(canvas)
        return bmp
    }
}
