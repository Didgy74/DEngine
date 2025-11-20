package didgy.dengine

import android.annotation.SuppressLint
import android.app.Activity
import android.content.Context
import android.content.res.Configuration
import android.os.Bundle
import android.util.Log
import android.view.*
import android.view.accessibility.AccessibilityEvent
import android.view.accessibility.AccessibilityManager
import androidx.core.view.WindowCompat

class DEngineActivity :
    Activity(),
    SurfaceHolder.Callback2,
    InputQueue.Callback
{
    val parentApp get() = application as DEngineApp

    lateinit var contentView: NativeView
    // Must be Long to match C++ stuff
    var mWindowId: Long = DEngineApp.INVALID_WINDOW_ID

    // Don't use 0 just to be safe
    private var mAccessibilityIdTracker: Int = 1
    private fun getNextAccessibilityChildId(): Int {
        val temp = mAccessibilityIdTracker
        mAccessibilityIdTracker += 1
        return temp
    }

    private lateinit var mViewConfiguration: ViewConfiguration

    // Don't access directly, the key-value might change. Use wrapper functions
    // There MUST be a 1:1 relationship here
    // This maps the DEngine WidgetID to the virtual child-ID exposed by the AccessibilityNodeInfo
    // If the value exists here, it MUST exist within the accessibility-data
    fun accessibilityWidgetIdFromVirtualAccessChild(virtualAccessChild: Int): Long? {
        val temp = accessibilityData
            .children
            .asSequence()
            .find { it.key.virtualAccessChildId == virtualAccessChild }
        if (temp == null)
            return null
        return temp.key.widgetId
    }

    var accessibilityData = VirtualAccessibilityData(
        focusedId = null,
        accessFocusedId = null,
        children = mapOf(),
        /*
        children = mapOf(
            1L to VirtualAccessibilityElement(
                posX = 0,
                posY = 0,
                width = 400,
                height = 400,
                text = "First"
            ),
            2L to VirtualAccessibilityElement(
                posX = 400,
                posY = 0,
                width = 400,
                height = 400,
                text = "Second"
            ),
            3L to VirtualAccessibilityElement(
                posX = 0,
                posY = 400,
                width = 400,
                height = 400,
                text = "Third"
            ),
        ),
         */
    )

    fun nativeOnNewOrientation(newOrientation: Int) =
        ToNative.onNewOrientation(parentApp.nativeBackendDataPtr, newOrientation)

    fun nativeOnNativeWindowCreated(surface: Surface) {
        ToNative.onNativeWindowCreated(parentApp.nativeBackendDataPtr, surface)
    }
    fun nativeOnNativeWindowDestroyed(surface: Surface) =
        ToNative.onNativeWindowDestroyed(parentApp.nativeBackendDataPtr, surface)

    override fun onCreate(savedState: Bundle?) {
        super.onCreate(savedState)

        parentApp.tryInit(this)

        WindowCompat.enableEdgeToEdge(window)

        mViewConfiguration = ViewConfiguration.get(this)

        contentView = NativeView(this)

        setContentView(contentView)
        contentView.requestLayout()
        contentView.requestFocus()
        contentView.requestApplyInsets()

        window.takeSurface(this)
        // Route this window's raw input (touch, mouse, gamepad, keys) to the native
        // AInputQueue on the game thread instead of the view-hierarchy dispatch. This
        // drives onInputQueueCreated/onInputQueueDestroyed (we implement InputQueue.Callback).
        // IME text still flows through NativeView's InputConnection, independent of this.
        window.takeInputQueue(this)

        val currConfig = Configuration(resources.configuration)
        currOrientation = currConfig.orientation
        currFontScale = currConfig.fontScale
    }


    // Called from the native C++ thread
    fun fromNative_accessibilityUpdate(
        allWidgetId: LongArray,
        allPosX: IntArray,
        allPosY: IntArray,
        allWidth: IntArray,
        allHeight: IntArray,
        allClickable: BooleanArray,
        allTextStart: IntArray,
        allTextCount: IntArray,
        textData: ByteArray)
    {
        runOnUiThread {
            val allLength = listOf(
                allWidgetId.size,
                allPosX.size,
                allPosY.size,
                allWidth.size,
                allHeight.size,
                allClickable.size,
                allTextStart.size,
                allTextCount.size,)
            if (allLength.any { it != allWidgetId.size })
                return@runOnUiThread

            var newAccesChildren = mutableMapOf<WidgetId_VirtualAccessChild_Pair, VirtualAccessibilityElement>()
            // Check if certain WidgetIDs already exist, and reuse them.
            for (index in allWidgetId.indices) {

                val widgetId = allWidgetId[index]

                var virtualChildId = 0
                val temp = accessibilityData.children.keys.find { it.widgetId == widgetId }
                if (temp != null) {
                    virtualChildId = temp.virtualAccessChildId
                } else {
                    virtualChildId = getNextAccessibilityChildId()
                }

                val newChildKey = WidgetId_VirtualAccessChild_Pair(
                    widgetId = widgetId,
                    virtualAccessChildId = virtualChildId,)
                if (newAccesChildren.containsKey(newChildKey)) {
                    Log.e(
                        "DEngine",
                        "Received multiple accessibility element from C++ with widgetId $widgetId")
                    return@runOnUiThread
                }

                val posX = allPosX[index]
                val posY = allPosY[index]
                val width = allWidth[index]
                val height = allHeight[index]
                val clickable = allClickable[index];
                val textStart = allTextStart[index]
                val textCount = allTextCount[index]
                if (textStart < 0 ||
                    textCount < 0 ||
                    textStart + textCount > textData.size)
                {
                    Log.e(
                        "DEngine",
                        "Received accessibility element from C++ with widgetId $widgetId " +
                        "that contained out-of-bounds text slice.")
                    return@runOnUiThread
                }

                val tempString = String(
                    textData,
                    textStart,
                    textCount,
                    Charsets.US_ASCII)

                newAccesChildren.set(
                    newChildKey,
                    VirtualAccessibilityElement(
                        posX = posX,
                        posY = posY,
                        width = width,
                        height = height,
                        clickable = clickable,
                        text = tempString))
            }

            val temp = VirtualAccessibilityData(
                focusedId = null,
                accessFocusedId = null,
                children = newAccesChildren)
            if (accessibilityData == temp) {
                return@runOnUiThread
            }
            accessibilityData = temp



            val accessibilityManager = getSystemService(Context.ACCESSIBILITY_SERVICE) as AccessibilityManager
            if (accessibilityManager.isEnabled) {
                // Send the accessibility event

                val event = AccessibilityEvent()
                event.setSource(contentView)
                event.eventType = AccessibilityEvent.TYPE_WINDOW_CONTENT_CHANGED
                event.packageName = packageName
                event.className = contentView.javaClass.name
                val success = contentView
                    .parent
                    .requestSendAccessibilityEvent(
                        contentView,
                        event)
                if (!success) {
                    Log.e("DEngine", "Unable to send accessibility event")
                }
            }
        }
    }

    // This is called from the native main game thread.
    @SuppressLint
    fun NativeEvent_HideSoftInput() {
        runOnUiThread(contentView::endInputSession)
    }

    // Surface related callbacks
    override fun surfaceCreated(holder: SurfaceHolder) {
        DEngineApp.verboseEventLog(DEngineApp.EventType.NativeSurfaceCreated)
        nativeOnNativeWindowCreated(holder.surface)
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        DEngineApp.verboseEventLog(DEngineApp.EventType.NativeSurfaceChanged)
    }

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        DEngineApp.verboseEventLog(DEngineApp.EventType.NativeSurfaceDestroyed)
        nativeOnNativeWindowDestroyed(holder.surface)
    }

    override fun surfaceRedrawNeeded(holder: SurfaceHolder) {
    }
    // Surface related callbacks end

    override fun dispatchGenericMotionEvent(event: MotionEvent?): Boolean {
        if (event == null)
            return false

        for (pointerIndex in 0 until event.pointerCount) {
            val pointerId = event.getPointerId(pointerIndex)
            
            val type = event.getToolType(pointerIndex)
            val source = event.source
            InputDevice.SOURCE_TOUCHSCREEN
            if (type == MotionEvent.TOOL_TYPE_FINGER && (source and InputDevice.SOURCE_TOUCHSCREEN) > 0) {
                // If we get a hover event from tool type finger from touchscreen, it indicates
                // that TalkBack wants us to put accessibility focus on something

                return true
            }
        }

        return false
    }

    // Touch (and all other raw input) is delivered to the native AInputQueue via
    // window.takeInputQueue(this), so we no longer forward it from dispatchTouchEvent.

    /*
    private var prevLocationX = 0
    private var prevLocationY = 0
    private var prevWidth = 0
    private var prevHeight = 0
    override fun onGlobalLayout() {
        if (Build.VERSION.SDK_INT <= 34) {
            val newLocation = IntArray(2)
            contentView.getLocationInWindow(newLocation)

            val newW = contentView.width
            val newH = contentView.height
            val somethingChanged =
                newLocation[0] != prevLocationX ||
                newLocation[1] != prevLocationY ||
                newW != prevWidth ||
                newH != prevHeight

            if (somethingChanged) {
                prevLocationX = newLocation[0]
                prevLocationY = newLocation[1]
                prevWidth = newW
                prevHeight = newH
                nativeOnContentRectChanged(
                    prevLocationX, prevLocationY,
                    prevWidth, prevHeight
                )
            }
        }
    }
     */

    private var currOrientation = 0
    private var currFontScale = 0f
    override fun onConfigurationChanged(newConfig: Configuration) {
        super.onConfigurationChanged(newConfig)
        if (currFontScale != newConfig.fontScale) {
            DEngineApp.verboseEventLog(DEngineApp.EventType.ConfigFontScale)
            currFontScale = newConfig.fontScale
            ToNative.onFontScale(parentApp.nativeBackendDataPtr, mWindowId, currFontScale)
        }

        if (currOrientation != newConfig.orientation) {
            DEngineApp.verboseEventLog(DEngineApp.EventType.ConfigOrientation)
            currOrientation = newConfig.orientation
            ToNative.onNewOrientation(parentApp.nativeBackendDataPtr, currOrientation)
        }
    }

    override fun onInputQueueCreated(queue: InputQueue) {
        ToNative.onInputQueueCreated(
            parentApp.nativeBackendDataPtr,
            this,
            mWindowId,
            queue)
    }

    override fun onInputQueueDestroyed(queue: InputQueue) {
        ToNative.onInputQueueDestroyed(
            parentApp.nativeBackendDataPtr,
            this,
            mWindowId,
            queue)
    }
}