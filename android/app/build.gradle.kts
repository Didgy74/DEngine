import org.jetbrains.kotlin.gradle.dsl.JvmTarget

plugins {
    alias(libs.plugins.android.application)
}

android {
    namespace = "com.example.dengine"
    compileSdk = 37
    ndkVersion = "30.0.14904198"

    defaultConfig {
        applicationId = "com.example.dengine"
        minSdk = 34
        targetSdk = 37
        versionCode = 1
        versionName = "1.0"

        //testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"

        ndk {
            // We don't support x86, fuck those guys for now
            //noinspection ChromeOsAbiSupport
            //abiFilters += setOf("arm64-v8a", "x86_64")
            abiFilters += setOf("arm64-v8a")
        }

        externalNativeBuild {
            cmake {
                arguments += mutableListOf(
                    "-DDENGINE_PRECOMPILE_ASSET_OUTPUT_DIR=${project.layout.buildDirectory.get().asFile.absolutePath}/assets/assets",
                )
            }
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_11
        targetCompatibility = JavaVersion.VERSION_11
    }

    externalNativeBuild {
        cmake {
            path = File("CMakeLists.txt")
            version = "4.1.2"
        }
    }

    sourceSets {
        named("main") {
            assets.directories.add("${project.layout.buildDirectory.get().asFile.absolutePath}/assets")
        }
        named("debug") {
            jniLibs.directories.add("src/lib")
        }
    }
}

kotlin {
    compilerOptions {
        jvmTarget = JvmTarget.JVM_11
    }
}

dependencies {
    //implementation(libs.kotlinx.coroutines.core)
    implementation(libs.androidx.core.ktx)
    //implementation(libs.material)
    //testImplementation(libs.junit)
    //androidTestImplementation(libs.androidx.junit)
    //androidTestImplementation(libs.androidx.espresso.core)
}