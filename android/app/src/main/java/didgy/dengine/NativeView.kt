package didgy.dengine

import android.content.Context
import android.graphics.Rect
import android.os.Bundle
import android.util.AttributeSet
import android.util.Log
import android.view.View
import android.view.accessibility.AccessibilityNodeInfo
import android.view.accessibility.AccessibilityNodeInfo.ExtraRenderingInfo
import android.view.accessibility.AccessibilityNodeProvider
import android.view.inputmethod.EditorInfo
import android.view.inputmethod.InputConnection
import android.view.inputmethod.InputMethodManager

class NativeView : View
{
    private var _activity: DEngineActivity? = null
    val activity get() = _activity!!

    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        val width = getDefaultSize(0, widthMeasureSpec)
        val height = getDefaultSize(0, heightMeasureSpec)
        setMeasuredDimension(width, height)
    }

    val mIMM get() = activity.getSystemService(InputMethodManager::class.java)!!
    var mBeginInputSessionInfo: BeginInputSessionInfo? = null
    var mInputConnectionCounter = 0

    var ic: NativeInputConnection? = null

    fun beginInputSession(initialString: String, inputFilter: NativeInputFilter) {
        mBeginInputSessionInfo = BeginInputSessionInfo(
            activity = activity,
            inputFilter = inputFilter,
            text = initialString,
            selStart = initialString.length,
            selCount = 0)

        isFocusable = true
        isFocusableInTouchMode = true
        requestFocus()

        mIMM.restartInput(this)
        mIMM.showSoftInput(this, 0)
    }

    fun endInputSession() {
        ic = null
        mIMM.hideSoftInputFromWindow(this.windowToken, 0)
        mIMM.restartInput(this)
    }

    // Under some circumstances, this may be called twice in a row.
    // idk why, but we gotta make sure to just do the same thing twice if
    // it does.
    override fun onCreateInputConnection(outAttrs: EditorInfo): InputConnection? {

        if (ic != null) {
            ic!!.setEditorInfo(outAttrs)
            return ic
        }

        if (mBeginInputSessionInfo != null) {
            val sessionInputInfo = mBeginInputSessionInfo!!
            mBeginInputSessionInfo = null

            ic = NativeInputConnection(
                this,
                sessionInputInfo)
            ic!!.setEditorInfo(outAttrs)
            return ic
        }

        return null
    }

    override fun onCheckIsTextEditor(): Boolean {
        // This means we don't want the IME to automatically pop up
        // when opening this View.
        return false
    }

    var myAccessibilityNodeProvider: MyAccessibilityNodeProvider
    override fun getAccessibilityNodeProvider(): AccessibilityNodeProvider {
        return myAccessibilityNodeProvider
    }

    class MyAccessibilityNodeProvider(var nativeView: NativeView) : AccessibilityNodeProvider() {

        fun fillAcessibilityNodeInfo(info: AccessibilityNodeInfo, virtualViewId: Int) {

            val rawData = nativeView.activity.accessilityData
            val elementCount = rawData.ElementCount()
            if (elementCount == 0)
                return

            if (virtualViewId == HOST_VIEW_ID) {
                for (i in 0 until elementCount) {
                    info.addChild(this.nativeView, i)
                }
            } else if (virtualViewId in 0 until elementCount) {
                info.setParent(this.nativeView)

                val element = rawData.GetElement(virtualViewId)
                if (element.textCount > 0) {
                    val elementString = rawData.stringBytes
                        .sliceArray(element.textStart until element.textStart + element.textCount)
                        .toString(Charsets.US_ASCII)
                    info.text = elementString
                }

                val myRect = Rect()
                myRect.left = element.posX
                myRect.top = element.posY
                myRect.right = myRect.left + element.width
                myRect.bottom = myRect.top + element.height
                info.setBoundsInScreen(myRect)

                info.isVisibleToUser = true
                info.isImportantForAccessibility = true
                info.isScreenReaderFocusable = true
                info.isClickable = element.isClickable
                info.isEnabled = true
            } else {
                Log.v(
                    "DEngine - NativeView",
                    "requested AccessibilityNodeInfo for a virtual view id that does not exist")
            }
        }

        override fun createAccessibilityNodeInfo(virtualViewId: Int): AccessibilityNodeInfo {
            val out = AccessibilityNodeInfo(nativeView, virtualViewId)
            fillAcessibilityNodeInfo(out, virtualViewId)
            return out
        }

        override fun performAction(virtualViewId: Int, action: Int, arguments: Bundle?): Boolean {
            val baseValue = super.performAction(virtualViewId, action, arguments)

            return false
        }
    }

    init {
        this.isFocusable = true
        this.isFocusableInTouchMode = true
        this.importantForAccessibility = IMPORTANT_FOR_ACCESSIBILITY_YES
        this.myAccessibilityNodeProvider = MyAccessibilityNodeProvider(this)
    }

    constructor(mainActivity: DEngineActivity) :
        this(mainActivity, null, 0)
    {
        this._activity = mainActivity
    }


    constructor(context: Context?, attrs: AttributeSet?, defStyleAttr: Int) :
        super(context, attrs, defStyleAttr, 0)
    {
    }

    constructor(context: Context?, attrs: AttributeSet?, defStyleAttr: Int, defStyleRes: Int) :
        super(context, attrs, defStyleAttr, defStyleRes)
    {

    }
}