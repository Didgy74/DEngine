package didgy.dengine

import android.annotation.SuppressLint
import android.app.Application
import android.util.Log
import android.view.accessibility.AccessibilityEvent

class DEngineApp : Application() {

    companion object {
        const val INVALID_WINDOW_ID = -1

        @SuppressLint
        @JvmStatic
        fun NativeEvent_AccessibilityUpdate(
            app: DEngineApp,
            windowId: Int,
            data: IntArray,
            textData: ByteArray)
        {
            // Find the window
            val activity = app.tempActivity!!
            activity.runOnUiThread {
                activity.accessilityData = AccessibilityUpdateData(
                    data = data,
                    stringBytes = textData)
                activity.contentView.sendAccessibilityEvent(AccessibilityEvent.TYPE_WINDOW_CONTENT_CHANGED)
            }
        }
    }


    private var mNativeBackendDataPtr: Long = 0
    val nativeBackendDataPtr get() = mNativeBackendDataPtr
    private var mNativeInitialized = false

    private var mWindows = HashMap<Int, DEngineActivity>()
    private val mUnassignedActivities: MutableList<DEngineActivity> = mutableListOf()
    private var tempActivity: DEngineActivity? = null

    fun tryInit(activity: DEngineActivity) {
        if (mNativeInitialized) {
            return
        }

        tempActivity = activity

        mNativeBackendDataPtr = NativeInterface.init(
        this,
            this.javaClass,
            activity,
            assets,
            resources.configuration.fontScale * 0.5f)
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

    /*
        The int array contains structs of the following elements
        struct Element {
            int posX
            int posY
            int extentX
            int extentY
            int textStart
            int textCount
            int isClickable
        }
    */
    class AccessibilityUpdateData(
        val data: IntArray = IntArray(0),
        val stringBytes: ByteArray = ByteArray(0))
    {
        companion object {
            const val elementMemberCount = 7
        }

        init {
            if (data.size % elementMemberCount != 0) {
                throw IllegalArgumentException("IntArray is not valid.")
            }
        }

        data class Element(
            val posX: Int,
            val posY: Int,
            val width: Int,
            val height: Int,
            val textStart: Int,
            val textCount: Int,
            val mIsClickable: Int, )
        {
            val isClickable get() = mIsClickable > 0
        }

        fun ElementCount(): Int {
            return data.size / elementMemberCount
        }

        fun GetElement(index: Int): Element {
            val offset = index * elementMemberCount
            assert(offset + 6 <= data.size)

            return Element(
                posX = data[offset + 0],
                posY = data[offset + 1],
                width = data[offset + 2],
                height = data[offset + 3],
                textStart = data[offset + 4],
                textCount = data[offset + 5],
                mIsClickable = data[offset + 6])
        }
    }
}