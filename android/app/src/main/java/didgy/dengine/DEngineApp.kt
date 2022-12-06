package didgy.dengine

import android.app.Application
import android.util.Log

class DEngineApp : Application() {

    override fun onCreate() {
        super.onCreate()

        try {
            System.loadLibrary("dengine")
        } catch (e: Exception) {
            Log.e("DEngine", "Unable to dynamically link DEngine. ${e.message}")
        }
    }
}