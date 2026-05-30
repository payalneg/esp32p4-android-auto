package com.aabridge.aa_bridge

import android.content.Context
import android.content.Intent
import android.content.pm.LauncherApps
import android.content.pm.PackageManager
import android.graphics.Bitmap
import android.graphics.drawable.Drawable
import android.os.Process
import android.util.Log
import androidx.core.graphics.drawable.toBitmap
import java.io.ByteArrayOutputStream

object IconExtractor {
    private const val TAG = "IconExtractor"

    // 72 px keeps app icons in the 2-6 KB range while remaining sharp
    // at the 96 px slot in the notification toast.
    fun iconPng(ctx: Context, pkg: String, size: Int = 72): ByteArray? {
        if (pkg.isBlank()) return null
        var source = "launcher"
        var drawable: Drawable? = launcherIcon(ctx, pkg)
        if (drawable == null) {
            source = "appicon"
            drawable = applicationIcon(ctx, pkg)
        }
        if (drawable == null) {
            Log.w(TAG, "$pkg: no drawable")
            return null
        }

        // androidx-core's toBitmap(width, height) renders the drawable
        // into a fixed-size bitmap in one pass.
        val bmp = drawable.toBitmap(size, size, Bitmap.Config.ARGB_8888)
        val bytes = ByteArrayOutputStream()
        bmp.compress(Bitmap.CompressFormat.PNG, 100, bytes)
        val data = bytes.toByteArray()
        Log.i(TAG, "$pkg via=$source cls=${drawable.javaClass.simpleName} " +
                "intrinsic=${drawable.intrinsicWidth}x${drawable.intrinsicHeight} " +
                "png=${data.size}B")
        return data
    }

    /**
     * Preferred: ask the launcher service for the already-masked icon. For
     * AdaptiveIconDrawable apps the OS pre-applies the shape mask + scales
     * the foreground/background layers and hands back a flat drawable
     * we can render 1:1. PackageManager.getApplicationIcon() on the same
     * pkg returns the raw adaptive drawable, which when drawn at 72×72
     * bleeds past its safe zone and shows up on the head unit as one
     * main tile plus four edge half-tiles.
     */
    private fun launcherIcon(ctx: Context, pkg: String): Drawable? {
        val svc = ctx.getSystemService(Context.LAUNCHER_APPS_SERVICE)
            as? LauncherApps ?: return null
        val acts = try {
            svc.getActivityList(pkg, Process.myUserHandle())
        } catch (_: SecurityException) { null } ?: return null
        if (acts.isEmpty()) return null
        // density 0 → device default; LauncherApps returns the already
        // shape-masked icon at this density.
        return acts.first().getIcon(0)
    }

    private fun applicationIcon(ctx: Context, pkg: String): Drawable? {
        return try {
            ctx.packageManager.getApplicationIcon(pkg)
        } catch (_: PackageManager.NameNotFoundException) { null }
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
}
