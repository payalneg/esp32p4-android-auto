# Android app 0.3.42 (build 102)

This release reduces background activity when the head unit is out of range.
The foreground service can keep waiting for Bluetooth without holding the
phone's CPU awake for the entire wait.

## Changes

- Hold the CPU wake lock only while connected; release it on disconnection
  and service teardown.
- Stop observing media sessions and processing album covers while
  disconnected. Send the current music state when the link returns.
- Skip notification/icon processing and stop clock/greeting timers without
  an active connection.
- Space manual connection recovery attempts through 40, 80, 160 and 300
  seconds. Android's normal autoConnect request remains pending between them.
- Prevent a late permission check from starting GPS after the app has been
  hidden with neither an active route nor a connected head unit.

## Installation and signing key

The previous signing key is no longer available. This APK uses the new key
also used for the battery-fix build installed on 2026-10-20.

- From that battery-fix build: install this APK as a normal update.
- From older GitHub APKs: uninstall the old app before installing this one.
  Uninstalling resets local app data, including offline maps, settings,
  assistant history and saved API keys. Pair the head unit and download maps
  again after installation.

The public APK has no embedded assistant API key.

## Firmware and verification

This is an app-only release. The APK carries the existing **1.3.33** firmware
images for Waveshare, JC4880 and S3-Touch-LCD-4; no new firmware is released.
Standalone firmware downloads remain in
[v1.3.33](https://github.com/payalneg/esp32p4-android-auto/releases/tag/v1.3.33).
The hardware limitations documented for that firmware still apply.

The battery-fix build was checked on an Android 16 phone: its foreground
service stayed running without an app CPU wake lock or active GPS request
while disconnected. The user subsequently reported improved battery use.
This is not a quantified battery-life benchmark. The final release is covered
by 25 reconnect/feed/navigation tests and APK integrity/signature checks.

The separate Android 16 restriction on starting the combined location/BLE
service from the background is not fixed here. After reboot or an app update,
opening the app may still be needed to start that service. Map transmission
after reconnecting to a real head unit still needs hardware verification.

## По-русски

Уменьшена фоновая работа без головы рядом: снимается блокировка сна,
останавливаются обработка медиа и лишние таймеры, реже выполняются
принудительные повторы подключения. Автоподключение Android сохранено.
Версия приложения — **0.3.42 (102)**, прошивка — прежняя **1.3.33**.

Ключ подписи изменился. Исправленная сборка от 20 октября обновляется обычным
способом. Для перехода с прежнего APK с GitHub потребуется удалить старое
приложение: локальные карты, настройки, история и API-ключи сбросятся.
После установки нужно заново привязать голову и загрузить карты.

Ограничение фонового автозапуска на Android 16 остаётся: после перезагрузки
телефона или обновления может потребоваться открыть приложение.
