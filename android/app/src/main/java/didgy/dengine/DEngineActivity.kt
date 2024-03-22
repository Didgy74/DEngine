package didgy.dengine

import android.annotation.SuppressLint
import android.app.Activity
import android.content.pm.ActivityInfo
import android.content.res.Configuration
import android.os.Bundle
import android.view.*
import android.view.View.OnTouchListener
import android.view.ViewTreeObserver.OnGlobalLayoutListener
class DEngineActivity :
    Activity(),
    SurfaceHolder.Callback2,
    OnGlobalLayoutListener,
    OnTouchListener,
    InputQueue.Callback
{
    val parentApp get() = application as DEngineApp

    lateinit var contentView: NativeView
    var mWindowId = DEngineApp.INVALID_WINDOW_ID

    var accessilityData = DEngineApp.AccessibilityUpdateData()
    data class NativeTextInputJob(
        val start: Int,
        val count: Int,
        val text: String)
    // Input text jobs can be submitted in batches, therefore
    // we want signal all of them at once when that happens.
    // This function will turn the data into the linear arrays
    // that the native code expects
    fun nativeOnTextInput(inputJobs: List<NativeTextInputJob>) {
        if (inputJobs.isEmpty())
            return

        var textOffset = 0
        val textOffsets = inputJobs.map {
            val temp = textOffset
            textOffset += it.text.length
            temp
        }
        NativeInterface.onTextInput(
            parentApp.nativeBackendDataPtr,
            inputJobs.map { it.start }.toIntArray(),
            inputJobs.map { it.count }.toIntArray(),
            textOffsets.toIntArray(),
            inputJobs.map { it.text.length }.toIntArray(),
            inputJobs.joinToString { it.text })
    }

    fun nativeSendEventEndTextInputSession() =
        NativeInterface.sendEventEndTextInputSession(parentApp.nativeBackendDataPtr)

    fun nativeOnNewOrientation(newOrientation: Int) =
        NativeInterface.onNewOrientation(parentApp.nativeBackendDataPtr, newOrientation)

    fun nativeOnFontScale(newScale: Float) =
        NativeInterface.onFontScale(parentApp.nativeBackendDataPtr, mWindowId, newScale)

    fun nativeOnContentRectChanged(
        posX: Int,
        posY: Int,
        width: Int,
        height: Int)
    {
        NativeInterface.onContentRectChanged(
            parentApp.nativeBackendDataPtr,
            posX, posY,
            width, height)
    }

    fun nativeOnNativeWindowCreated(surface: Surface) =
        NativeInterface.onNativeWindowCreated(parentApp.nativeBackendDataPtr, surface)

    fun nativeOnNativeWindowDestroyed(surface: Surface) =
        NativeInterface.onNativeWindowDestroyed(parentApp.nativeBackendDataPtr, surface)

    override fun onCreate(savedState: Bundle?) {
        super.onCreate(savedState)
        parentApp.tryInit(this)

        contentView = NativeView(this)
        setContentView(contentView)
        contentView.viewTreeObserver.addOnGlobalLayoutListener(this)
        contentView.setOnTouchListener(this)
        contentView.requestFocus()


        window.takeSurface(this)

        currConfig = Configuration(resources.configuration)

    }

    // This is called from the native main game thread.
    @SuppressLint
    fun NativeEvent_OpenSoftInput(text: String, softInputFilter: Int) {
        runOnUiThread {
            contentView.beginInputSession(
                text,
                NativeInputFilter.fromNativeEnum(softInputFilter))
        }
    }
    @SuppressLint
    fun NativeEvent_HideSoftInput() {
        runOnUiThread(contentView::endInputSession)
    }
    override fun surfaceCreated(holder: SurfaceHolder) {
        nativeOnNativeWindowCreated(holder.surface)
    }
    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {

    }
    override fun surfaceDestroyed(holder: SurfaceHolder) {
        if (holder.surface != null) {
            //Log.e("DEngine Activity", "During surface destroyed, expected incoming surface to be null.")
        }
        nativeOnNativeWindowDestroyed(holder.surface)
    }

    override fun surfaceRedrawNeeded(holder: SurfaceHolder) {
        //
    }

    private var prevLocationX = 0
    private var prevLocationY = 0
    private var prevWidth = 0
    private var prevHeight = 0
    override fun onGlobalLayout() {
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
                prevWidth, prevHeight)
        }
    }

    private lateinit var currConfig: Configuration
    override fun onConfigurationChanged(newConfig: Configuration) {
        super.onConfigurationChanged(newConfig)
        val diff = newConfig.diff(currConfig)
        if (diff and ActivityInfo.CONFIG_FONT_SCALE != 0) {
            nativeOnFontScale(newConfig.fontScale)
        }
        if (diff and ActivityInfo.CONFIG_ORIENTATION != 0) {
            nativeOnNewOrientation(newConfig.orientation)
        }
        currConfig = newConfig
    }

    override fun onTouch(v: View, event: MotionEvent): Boolean {
        if (event.source and InputDevice.SOURCE_TOUCHSCREEN > 0) {
            val location = IntArray(2)
            v.getLocationInSurface(location)
            val action = event.action

            val x = event.getX(0) + location[0]
            val y = event.getY(0) + location[1]

            NativeInterface.onTouch(
                parentApp.nativeBackendDataPtr,
                event.getPointerId(0),
                x,
                y,
                action)
        }

        return true
    }

    override fun onInputQueueCreated(queue: InputQueue?) {

    }

    override fun onInputQueueDestroyed(queue: InputQueue?) {

    }
}