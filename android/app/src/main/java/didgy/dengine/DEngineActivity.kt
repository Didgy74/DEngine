package didgy.dengine

import android.annotation.SuppressLint
import android.app.Activity
import android.app.NativeActivity
import android.content.pm.ActivityInfo
import android.content.res.Configuration
import android.graphics.PixelFormat
import android.os.Bundle
import android.view.*
import android.view.View.OnTouchListener
import android.view.ViewTreeObserver.OnGlobalLayoutListener

class DEngineActivity :
    Activity(),
    SurfaceHolder.Callback2,
    OnGlobalLayoutListener,
    OnTouchListener
{
    val parentApp get() = application as DEngineApp

    private var mNativeBackendDataPtr: Long = 0
    lateinit var contentView: NativeView
    val INVALID_WINDOW_ID = -1
    var mWindowId = INVALID_WINDOW_ID

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
            mNativeBackendDataPtr,
            inputJobs.map { it.start }.toIntArray(),
            inputJobs.map { it.count }.toIntArray(),
            textOffsets.toIntArray(),
            inputJobs.map { it.text.length }.toIntArray(),
            inputJobs.joinToString { it.text })
    }

    fun nativeSendEventEndTextInputSession() =
        NativeInterface.sendEventEndTextInputSession(mNativeBackendDataPtr)

    fun nativeOnNewOrientation(newOrientation: Int) =
        NativeInterface.onNewOrientation(mNativeBackendDataPtr, newOrientation)

    fun nativeOnFontScale(newScale: Float) =
        NativeInterface.onFontScale(mNativeBackendDataPtr, mWindowId, newScale)

    fun nativeOnContentRectChanged(
        posX: Int,
        posY: Int,
        width: Int,
        height: Int)
    {
        NativeInterface.onContentRectChanged(
            mNativeBackendDataPtr,
            posX, posY,
            width, height)
    }

    fun nativeOnNativeWindowCreated(surface: Surface) =
        NativeInterface.onNativeWindowCreated(mNativeBackendDataPtr, surface)

    fun nativeOnNativeWindowDestroyed(surface: Surface) =
        NativeInterface.onNativeWindowDestroyed(mNativeBackendDataPtr, surface)

    override fun onCreate(savedState: Bundle?) {
        super.onCreate(savedState)

        mNativeBackendDataPtr = NativeInterface_InitWrapper(this)

        window.setFormat(PixelFormat.RGBA_8888)
        window.takeSurface(this)

        contentView = NativeView(this)
        setContentView(contentView)
        contentView.requestFocus()
        contentView.viewTreeObserver.addOnGlobalLayoutListener(this)
        contentView.setOnTouchListener(this)

        //contentView.viewTreeObserver.addOnGlobalLayoutListener(this)
        contentView.isFocusable = true
        contentView.isFocusableInTouchMode = true

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
                mNativeBackendDataPtr,
                event.getPointerId(0),
                x,
                y,
                action)
        }

        return true
    }
}

fun NativeInterface_InitWrapper(activity: DEngineActivity): Long {
    val config = Configuration(activity.resources.configuration)
    return NativeInterface.init(
        activity,
        activity.parentApp.assets,
        config.fontScale)
}