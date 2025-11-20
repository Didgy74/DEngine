package didgy.dengine

import android.annotation.SuppressLint
import android.content.Context
import android.graphics.Insets
import android.util.AttributeSet
import android.util.Log
import android.view.View
import android.view.WindowInsets
import android.view.WindowInsetsAnimation
import android.view.accessibility.AccessibilityNodeInfo
import android.view.inputmethod.EditorInfo
import android.view.inputmethod.InputConnection
import android.view.inputmethod.InputMethodManager

@SuppressLint("ClickableViewAccessibility")
class NativeView : View {
    private var _dengineActivity: DEngineActivity? = null
    val dengineActivity get() = _dengineActivity!!
    val dengineApp get() = dengineActivity.parentApp

    var mDEngineOngoingInsetsAnimationCounter = 0

    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        val width = getDefaultSize(0, widthMeasureSpec)
        val height = getDefaultSize(0, heightMeasureSpec)
        setMeasuredDimension(width, height)
    }

    override fun onInitializeAccessibilityNodeInfo(info: AccessibilityNodeInfo?) {
        super.onInitializeAccessibilityNodeInfo(info)
        assert(false)
    }

    // TODO: For now we assume that this is always called after the orientation is reported.
    override fun onApplyWindowInsets(insets: WindowInsets): WindowInsets {
        if (mDEngineOngoingInsetsAnimationCounter == 0) {
            val systemBarInsets = insets.getInsets(WindowInsets.Type.systemBars())
            val imeInsets = insets.getInsets(WindowInsets.Type.ime())
            DEngineApp.verboseEventLog(DEngineApp.EventType.WindowInsets, insets.toString())

            ToNative.onApplyWindowInsets(
                dengineApp.nativeBackendDataPtr,
                systemBarInsets.left,
                systemBarInsets.right,
                systemBarInsets.top,
                systemBarInsets.bottom,
                imeInsets.left,
                imeInsets.right,
                imeInsets.top,
                imeInsets.bottom)
        }
        return super.onApplyWindowInsets(insets)
    }

    val mIMM get() = dengineActivity.getSystemService(InputMethodManager::class.java)!!
    var mDEngineBeginTextInputSessionInfo: BeginInputSessionInfo? = null
    var mInputConnectionCounter = 0

    var mDEngineTextInputConnection: NativeInputConnection? = null

    fun beginInputSession(
        initialString: String,
        selectionStart: Int,
        selectionCount: Int,
        inputFilter: NativeInputFilter)
    {
        assert(selectionStart + selectionCount <= initialString.length)
        mDEngineBeginTextInputSessionInfo = BeginInputSessionInfo(
            activity = dengineActivity,
            inputFilter = inputFilter,
            text = initialString,
            selStart = selectionStart,
            selCount = selectionCount)

        isFocusable = true
        isFocusableInTouchMode = true
        requestFocus()

        mIMM.restartInput(this)
        mIMM.showSoftInput(this, 0)
    }

    // Called from C++ thread
    @SuppressLint
    fun fromNative_beginInputSession(
        initialString: String,
        selectionStart: Int,
        selectionCount: Int,
        inputFilter: Int)
    {
        this.dengineActivity.runOnUiThread {
            beginInputSession(
                initialString,
                selectionStart,
                selectionCount,
                NativeInputFilter.fromNativeEnum(inputFilter))
        }
    }

    fun endInputSession() {
        mDEngineTextInputConnection = null
        mIMM.hideSoftInputFromWindow(this.windowToken, 0)
        mIMM.restartInput(this)
    }

    fun dengineFromNativeUpdateInputConnection(
        selectionStart: Int,
        selectionCount: Int,
        textData: ByteArray): Boolean
    {
        if (mDEngineTextInputConnection == null)
            return false
        return mDEngineTextInputConnection!!.dengineFromNativeUpdateInputConnection(
            selectionStart,
            selectionCount,
            textData)
    }

    // Under some circumstances, this may be called twice in a row.
    // idk why, but we gotta make sure to just do the same thing twice if
    // it does.
    //
    // Even if we return an existing connection, we have to fill out the outAttrs
    override fun onCreateInputConnection(outAttrs: EditorInfo): InputConnection? {
        if (mDEngineTextInputConnection != null) {
            TODO()
            mDEngineTextInputConnection!!.setEditorInfo(outAttrs)
            return mDEngineTextInputConnection
        }

        if (mDEngineBeginTextInputSessionInfo != null) {
             val sessionInputInfo = mDEngineBeginTextInputSessionInfo!!
            mDEngineBeginTextInputSessionInfo = null

            mDEngineTextInputConnection = NativeInputConnection(
                this,
                sessionInputInfo,
                mIMM,
                dengineActivity.mWindowId,
                dengineApp.dEngineTextInputConnectionHandlerThread)
            mDEngineTextInputConnection!!.setEditorInfo(outAttrs)
            return mDEngineTextInputConnection
        }

        return null
    }

    override fun onCheckIsTextEditor(): Boolean {
        // This means we don't want the IME to automatically pop up
        // when opening this View.
        return false
    }

    val myAccessibilityNodeProvider = CustomAccessibilityNodeProvider(this)
    override fun getAccessibilityNodeProvider() = myAccessibilityNodeProvider

    init {
        isFocusableInTouchMode = true
    }

    constructor(mainActivity: DEngineActivity) :
            this(mainActivity, null, 0) {
        this._dengineActivity = mainActivity

        this.setWindowInsetsAnimationCallback(AnimatedInsetsCallback(this))
    }


    constructor(context: Context?, attrs: AttributeSet?, defStyleAttr: Int) :
            super(context, attrs, defStyleAttr, 0) {
    }

    constructor(context: Context?, attrs: AttributeSet?, defStyleAttr: Int, defStyleRes: Int) :
            super(context, attrs, defStyleAttr, defStyleRes) {
    }

    class AnimatedInsetsCallback(
        private val nativeView: NativeView
    ) : WindowInsetsAnimation.Callback(DISPATCH_MODE_STOP) {

        override fun onPrepare(animation: WindowInsetsAnimation) {
            Log.e("Nils", "window insets prepare")

            nativeView.mDEngineOngoingInsetsAnimationCounter++
        }

        override fun onStart(
            animation: WindowInsetsAnimation,
            bounds: WindowInsetsAnimation.Bounds
        ): WindowInsetsAnimation.Bounds {
            // Capture the inset bounds for animation start/end
            return bounds
        }

        override fun onProgress(
            insets: WindowInsets,
            runningAnimations: List<WindowInsetsAnimation>
        ): WindowInsets {
            val systemBarInsets = insets.getInsets(WindowInsets.Type.systemBars())
            val imeInsets = insets.getInsets(WindowInsets.Type.ime())

            ToNative.onApplyWindowInsets(
                nativeView.dengineApp.nativeBackendDataPtr,
                systemBarInsets.left,
                systemBarInsets.right,
                systemBarInsets.top,
                systemBarInsets.bottom,
                imeInsets.left,
                imeInsets.right,
                imeInsets.top,
                imeInsets.bottom)
            return insets
        }

        override fun onEnd(animation: WindowInsetsAnimation) {
            nativeView.mDEngineOngoingInsetsAnimationCounter--
        }
    }
}