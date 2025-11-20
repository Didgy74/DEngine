package didgy.dengine

import android.app.Application
import android.os.HandlerThread
import android.util.Log
import android.view.ViewConfiguration

class DEngineApp : Application() {

    enum class EventType {
        ConfigFontScale,
        ConfigDensity,
        ConfigOrientation,
        NativeSurfaceCreated,
        NativeSurfaceChanged,
        NativeSurfaceDestroyed,
        WindowInsets,
    }

    companion object {
        const val INVALID_WINDOW_ID: Long = -1

        const val ENABLE_VERBOSE_EVENT_LOGCAT: Boolean = true
        const val VERBOSE_EVENT_LOGCAT_CATEGORY: String = "DEngineVerboseEventLog"

        fun verboseEventLog(eventType: EventType, msg: String? = null) {
            if (ENABLE_VERBOSE_EVENT_LOGCAT) {
                if (msg != null) {
                    Log.e(VERBOSE_EVENT_LOGCAT_CATEGORY, "${eventType.name} - $msg")
                } else {
                    Log.e(VERBOSE_EVENT_LOGCAT_CATEGORY, eventType.name)
                }
            }
        }
    }

    private var mNativeBackendDataPtr: Long = 0
    // This is treated as const after program has started.
    val nativeBackendDataPtr get() = mNativeBackendDataPtr
    private var mNativeInitialized = false

    private var mWindows = mapOf<Int, DEngineActivity>()
    private val mUnassignedActivities = mutableListOf<DEngineActivity>()
    var tempActivity: DEngineActivity? = null

    private lateinit var mDEngineTextInputConnectionHandlerThread: HandlerThread
    val dEngineTextInputConnectionHandlerThread: HandlerThread get() { return mDEngineTextInputConnectionHandlerThread }

    fun tryInit(activity: DEngineActivity) {
        if (mNativeInitialized) {
            return
        }

        mDEngineTextInputConnectionHandlerThread = HandlerThread("DEngineInputConnectionHandler")
        mDEngineTextInputConnectionHandlerThread.start() // Start the thread and initialize its Looper

        tempActivity = activity

        val viewConfig = ViewConfiguration.get(activity)
        val scrollTouchSlopPx = viewConfig.scaledTouchSlop
        val scrollBarWidthPx = viewConfig.scaledScrollBarSize;

        mNativeBackendDataPtr = ToNative.init(
        this,
            this.javaClass,
            NativeToAndroid::class.java,
            activity,
            assets,
            resources.configuration.fontScale * 0.5f,
            scrollTouchSlopPx,
            scrollBarWidthPx)
        mNativeInitialized = true
    }

    override fun onCreate() {
        super.onCreate()

        try {
            System.loadLibrary("dengine")
        } catch (e: Exception) {
            Log.e("DEngine", "Unable to dynamically link DEngine. ${e.message}")
        }
    }
}