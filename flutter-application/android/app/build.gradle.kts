plugins {
    id("com.android.application")
    id("kotlin-android")
    // The Flutter Gradle Plugin must be applied after the Android and Kotlin Gradle plugins.
    id("dev.flutter.flutter-gradle-plugin")
}

android {
    namespace = "com.aabridge.aa_bridge"
    compileSdk = flutter.compileSdkVersion
    // flutter_local_notifications drags in a desugar dep that requires NDK 27.
    ndkVersion = "27.0.12077973"

    packaging {
        jniLibs {
            // Keep native libs compressed. From minSdk 23 up AGP stores them
            // uncompressed so the system can map them straight from the APK,
            // which doubled the file (31 → 60 MB) — and release APKs are
            // committed to this repo. Extraction at install is the old, fine
            // behaviour.
            useLegacyPackaging = true
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_11
        targetCompatibility = JavaVersion.VERSION_11
        // Required by flutter_local_notifications on minSdk < 26 — pulls
        // ZoneId/LocalDateTime back into the older Android runtime.
        isCoreLibraryDesugaringEnabled = true
    }

    kotlinOptions {
        jvmTarget = JavaVersion.VERSION_11.toString()
    }

    defaultConfig {
        // TODO: Specify your own unique Application ID (https://developer.android.com/studio/build/application-id.html).
        applicationId = "com.aabridge.aa_bridge"
        // You can update the following values to match your application needs.
        // For more information, see: https://flutter.dev/to/review-gradle-config.
        // 24, not Flutter's default 21: flutter_tts (spoken turn-by-turn in
        // the navigator) declares minSdk 24. Android 7.0 is from 2016.
        minSdk = 24
        targetSdk = flutter.targetSdkVersion
        versionCode = flutter.versionCode
        versionName = flutter.versionName
    }

    buildTypes {
        release {
            signingConfig = signingConfigs.getByName("debug")
        }
    }
}

dependencies {
    implementation("androidx.core:core-ktx:1.13.1")
    coreLibraryDesugaring("com.android.tools:desugar_jdk_libs:2.1.4")
}

flutter {
    source = "../.."
}
