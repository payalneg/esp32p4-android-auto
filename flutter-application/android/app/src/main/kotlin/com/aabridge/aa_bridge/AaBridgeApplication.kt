package com.aabridge.aa_bridge

import android.app.Application
import com.pravera.flutter_foreground_task.FlutterForegroundTaskLifecycleListener
import com.pravera.flutter_foreground_task.FlutterForegroundTaskPlugin
import com.pravera.flutter_foreground_task.FlutterForegroundTaskStarter
import io.flutter.embedding.engine.FlutterEngine

/**
 * Custom Application that wires our platform channels onto the foreground
 * service's background FlutterEngine (the one running the BLE isolate —
 * ble_host.dart).
 *
 * flutter_foreground_task creates that engine when the service starts; the
 * engine constructor auto-registers pub plugins (flutter_blue_plus,
 * shared_preferences, path_provider…), but NOT our hand-rolled channels. We
 * hook [FlutterForegroundTaskLifecycleListener.onEngineCreate] to add them.
 *
 * This must run even when the OS restarts the service without the Activity, so
 * it lives in Application.onCreate (declared via android:name in the manifest).
 */
class AaBridgeApplication : Application() {
    override fun onCreate() {
        super.onCreate()
        FlutterForegroundTaskPlugin.addTaskLifecycleListener(
            object : FlutterForegroundTaskLifecycleListener {
                override fun onEngineCreate(flutterEngine: FlutterEngine?) {
                    val engine = flutterEngine ?: return
                    AaBridgeChannels.register(
                        applicationContext,
                        engine.dartExecutor.binaryMessenger,
                        includeEventChannels = true,
                    )
                }

                override fun onTaskStart(starter: FlutterForegroundTaskStarter) {}
                override fun onTaskRepeatEvent() {}
                override fun onTaskDestroy() {}
                override fun onEngineWillDestroy() {}
            }
        )
    }
}
