package didgy.dengine;

import android.content.Context;
import android.os.LocaleList;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.InputMethodManager;

import java.util.Locale;

public class NativeView extends View {

    public DEngineActivity mActivity;
    public InputMethodManager mIMM;
    public BeginInputSessionInfo mBeginInputSessionInfo = null;
    public int mInputConnectionCounter = 0;

    public NativeView(DEngineActivity activity) {
        super(activity);

        mActivity = activity;
        mIMM = mActivity.getSystemService(InputMethodManager.class);
    }
    // This should not be used.
    public NativeView(Context activity) {
        super(activity);
        this.mActivity = null;
    }

    private boolean mFirstTimeInputSession = true;
    public void beginInputSession(String initialString, NativeInputFilter inputFilter) {
        //assert(mBeginInputSessionInfo == null);
        mBeginInputSessionInfo = new BeginInputSessionInfo();
        mBeginInputSessionInfo.mActivity = mActivity;
        mBeginInputSessionInfo.mView = this;
        mBeginInputSessionInfo.mIMM = mIMM;
        mBeginInputSessionInfo.mText = initialString;
        mBeginInputSessionInfo.inputFilter = inputFilter;
        mBeginInputSessionInfo.mSelStart = initialString.length();
        mBeginInputSessionInfo.mSelCount = 0;

        setFocusable(true);
        setFocusableInTouchMode(true);
        requestFocus();
        mIMM.restartInput(this);
        mIMM.showSoftInput(this, 0);
    }

    public void endInputSession() {
        mIMM.hideSoftInputFromWindow(this.getWindowToken(), 0);
        mBeginInputSessionInfo = null;
        mIMM.restartInput(this);
    }

    @Override
    public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
        if (mBeginInputSessionInfo == null) {
            return null;
        }

        outAttrs.initialSelStart = mBeginInputSessionInfo.getSelStart();
        outAttrs.initialSelEnd = mBeginInputSessionInfo.getSelEnd();
        outAttrs.imeOptions |= EditorInfo.IME_ACTION_DONE;
        outAttrs.inputType = mBeginInputSessionInfo.inputFilter.toAndroidInputType();
        outAttrs.hintLocales = new LocaleList(Locale.ENGLISH);

        var nextConnectionId = mInputConnectionCounter;
        mInputConnectionCounter += 1;

        var ic = new NativeInputConnection(
                this,
                mBeginInputSessionInfo);
        return ic;
    }

    @Override
    public boolean onCheckIsTextEditor() {
        // This means we don't want the IME to automatically pop up
        // when opening this View.
        return false;
    }
}
