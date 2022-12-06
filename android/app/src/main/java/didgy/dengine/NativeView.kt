package didgy.dengine

import android.view.View
import android.view.inputmethod.EditorInfo
import android.view.inputmethod.InputConnection
import android.view.inputmethod.InputMethodManager

class NativeView(
    private val _activity: DEngineActivity?) : View(_activity)
{
    val activity get() = _activity!!
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
}