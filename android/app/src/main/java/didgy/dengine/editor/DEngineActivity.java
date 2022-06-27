package didgy.dengine.editor;

import android.app.NativeActivity;
import android.content.Context;
import android.content.res.Configuration;
import android.os.Bundle;
import android.os.Handler;
import android.text.Editable;
import android.text.InputFilter;
import android.text.InputType;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.util.Log;
import android.view.KeyEvent;
import android.view.SurfaceHolder;
import android.view.View;
import android.view.WindowInsets;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.CompletionInfo;
import android.view.inputmethod.CorrectionInfo;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.ExtractedText;
import android.view.inputmethod.ExtractedTextRequest;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.InputContentInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;
import android.widget.TextView;

import java.security.Key;

public class DEngineActivity extends NativeActivity  {

    static {
        System.loadLibrary("dengine");
    }

    // Return type is the pointer to the backend-data
    public native double nativeInit();

    public native void nativeOnTextInput(
        double backendDataPtr,
        int start,
        int count,
        String text,
        int textLen);
    public native void nativeSendEventEndTextInputSession(double backendDataPtr);
    public native void nativeOnNewOrientation(int newOrientation);
    public native void nativeOnContentRectChanged(
            double backendDataPtr,
            int posX,
            int posY,
            int width,
            int height);

    public double mNativeBackendDataPtr = 0;
    public NativeContentView mNativeContentView = null;
    public Configuration mCurrentConfig = null;

    int mLastContentX = 0;
    int mLastContentY = 0;
    int mLastContentWidth = 0;
    int mLastContentHeight = 0;

    @Override
    protected void onCreate(Bundle savedState) {

        super.onCreate(savedState);

        mNativeContentView = new NativeContentView(this);

        setContentView(mNativeContentView);
        mNativeContentView.getViewTreeObserver().addOnGlobalLayoutListener(this);

        mNativeBackendDataPtr = nativeInit();

        mCurrentConfig = new Configuration(getResources().getConfiguration());
    }

    @Override
    protected void onStart() {
        super.onStart();
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);

        if (newConfig.orientation != mCurrentConfig.orientation)
        {
            nativeOnNewOrientation(newConfig.orientation);
        }

        mCurrentConfig = newConfig;
    }

    @Override
    public void onGlobalLayout() {
        super.onGlobalLayout();

        final int[] newLocation = new int[2];

        mNativeContentView.getLocationInWindow(newLocation);
        int w = mNativeContentView.getWidth();
        int h = mNativeContentView.getHeight();
        if (newLocation[0] != mLastContentX || newLocation[1] != mLastContentY
                || w != mLastContentWidth || h != mLastContentHeight) {
            mLastContentX = newLocation[0];
            mLastContentY = newLocation[1];
            mLastContentWidth = w;
            mLastContentHeight = h;
            nativeOnContentRectChanged(
                    mNativeBackendDataPtr,
                    mLastContentX,
                    mLastContentY,
                    mLastContentWidth,
                    mLastContentHeight);
        }
    }

    @Override
    public void onContentChanged() {
        super.onContentChanged();
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        super.surfaceCreated(holder);
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        super.surfaceChanged(holder, format, width, height);
    }

    // This is called from the native main game thread.
    public void nativeEvent_openSoftInput(String text, int softInputFilter) {
        class OpenSoftInputRunnable implements Runnable {
            final DEngineActivity activity;
            final int softInputFilter;
            final String innerText;
            OpenSoftInputRunnable(DEngineActivity inActivity, String text, int filter) {
                activity = inActivity;
                softInputFilter = filter;
                innerText = text;
            }
            public void run() {
                activity.mNativeContentView.beginInputSession(innerText, softInputFilter);
            }
        }
        OpenSoftInputRunnable test = new OpenSoftInputRunnable(this, text, softInputFilter);
        runOnUiThread(test);
    }

    public void nativeEvent_hideSoftInput()
    {
        class HideSoftInputRunnable implements Runnable {
            NativeContentView mNativeView = null;
            HideSoftInputRunnable(NativeContentView in) {
                mNativeView = in;
            }
            public void run() {
                mNativeView.endInputSession();
            }
        }
        HideSoftInputRunnable test = new HideSoftInputRunnable(mNativeContentView);
        runOnUiThread(test);
    }
}

class InputEditable extends SpannableStringBuilder  {
    public DEngineActivity mActivity;

    public InputEditable(DEngineActivity activity, String initialString, InputFilter[] filters) {
        super(initialString);
        mActivity = activity;
        setFilters(filters);
    }

    @Override
    public SpannableStringBuilder replace(int st, int en, CharSequence source, int start, int end) {
        var returnVal = super.replace(st, en, source, start, end);

        assert(st <= en);

        String replacementString = source.subSequence(start, end).toString();
        this.mActivity.nativeOnTextInput(
                this.mActivity.mNativeBackendDataPtr,
                st,
                en - st,
                replacementString,
                replacementString.length());

        return returnVal;
    }

    static InputFilter[] genInputFilters(int dengineInputFilter) {

        switch (dengineInputFilter)
        {
            case NativeContentView.SOFT_INPUT_FILTER_INTEGER:
                return new InputFilter[]{ new SignedIntegerTextFilter() };
            case NativeContentView.SOFT_INPUT_FILTER_UNSIGNED_INTEGER:
                return new InputFilter[]{ new UnsignedIntegerTextFilter(), new MinusFilter() };
            case NativeContentView.SOFT_INPUT_FILTER_FLOAT:
                return new InputFilter[]{ new FloatTextFilter(), new DotFilter(), new MinusFilter() };
            case NativeContentView.SOFT_INPUT_FILTER_UNSIGNED_FLOAT:
                return new InputFilter[]{ new UnsignedFloatTextFilter(), new DotFilter() };
        }
        return null;
    }

    static boolean charSequenceContains(CharSequence seq, char character) {
        int length = seq.length();
        for (int i = 0; i < length; i++) {
            if (seq.charAt(i) == character) {
                return true;
            }
        }
        return false;
    }

    static class SignedIntegerTextFilter implements InputFilter {
        @Override
        public CharSequence filter(CharSequence source, int start, int end, Spanned dest, int dstart, int dend) {
            boolean sourceHasInvalidChar = false;
            int sourceLength = end - start;
            for (int i = start; i < end; i++) {
                char c = source.charAt(i);
                if (!(48 <= c && c <= 57) &&
                        c != '-') {
                    sourceHasInvalidChar = true;
                    break;
                }
            }
            if (sourceHasInvalidChar) {
                StringBuilder returnVal = new StringBuilder(sourceLength);
                for (int i = start; i < end; i++) {
                    char c = source.charAt(i);
                    if ((48 <= c && c <= 57) ||
                            c == '-') {
                        returnVal.append(c);
                    }
                }
                return returnVal;
            }
            else
                return null;
        }
    }

    static class UnsignedIntegerTextFilter implements InputFilter {
        @Override
        public CharSequence filter(CharSequence source, int start, int end, Spanned dest, int dstart, int dend) {
            boolean sourceHasInvalidChar = false;
            int sourceLength = end - start;
            for (int i = start; i < end; i++) {
                char c = source.charAt(i);
                if (!(48 <= c && c <= 57)) {
                    sourceHasInvalidChar = true;
                    break;
                }
            }
            if (sourceHasInvalidChar) {
                StringBuilder returnVal = new StringBuilder(sourceLength);
                for (int i = start; i < end; i++) {
                    char c = source.charAt(i);
                    if (48 <= c && c <= 57) {
                        returnVal.append(c);
                    }
                }
                return returnVal;
            }
            else
                return null;
        }
    }

    static class UnsignedFloatTextFilter implements InputFilter {
        @Override
        public CharSequence filter(CharSequence source, int start, int end, Spanned dest, int dstart, int dend) {
            boolean sourceHasInvalidChar = false;
            int sourceLength = end - start;
            for (int i = start; i < end; i++) {
                char c = source.charAt(i);
                if (!('0' <= c && c <= '9') && c != '.') {
                    sourceHasInvalidChar = true;
                    break;
                }
            }
            if (sourceHasInvalidChar) {
                StringBuilder returnVal = new StringBuilder(sourceLength);
                for (int i = start; i < end; i++) {
                    char c = source.charAt(i);
                    if (('0' <= c && c <= '9') ||
                            c == '.')
                        returnVal.append(c);
                }
                return returnVal;
            }
            else
                return null;
        }
    }

    static class FloatTextFilter implements InputFilter {
        @Override
        public CharSequence filter(CharSequence source, int start, int end, Spanned dest, int dstart, int dend) {
            boolean sourceHasInvalidChar = false;
            int sourceLength = end - start;
            for (int i = start; i < end; i++) {
                char c = source.charAt(i);
                if (!(48 <= c && c <= 57) && c != '.' &&  c != '-') {
                    sourceHasInvalidChar = true;
                    break;
                }
            }
            if (sourceHasInvalidChar) {
                StringBuilder returnVal = new StringBuilder(sourceLength);
                for (int i = start; i < end; i++) {
                    char c = source.charAt(i);
                    if ((48 <= c && c <= 57) ||
                            c == '.' ||
                            c == '-')
                        returnVal.append(c);
                }
                return returnVal;
            }
            else
                return null;
        }
    }

    // Returns null if no change was made
    static CharSequence inputFilter_oneDot(CharSequence source, Spanned dest) {
        return null;
    }

    // Makes sure to remove dot if it doesn't follow decimal rules.
    static class DotFilter implements InputFilter {
        @Override
        public CharSequence filter(CharSequence source, int start, int end, Spanned dest, int dstart, int dend) {
            if (charSequenceContains(source.subSequence(start, end), '.')) {
                if (charSequenceContains(dest, '.')) {
                    // We need to build a sequence with no dot.
                    StringBuilder returnVal = new StringBuilder(end - start);
                    for (int i = start; i < end; i++) {
                        char c = source.charAt(i);
                        if (c != '.')
                            returnVal.append(c);
                    }
                    return returnVal;
                }
            }
            return null;
        }
    }

    static class MinusFilter implements InputFilter {
        @Override
        public CharSequence filter(CharSequence source, int start, int end, Spanned dest, int dstart, int dend) {
            if (charSequenceContains(source.subSequence(start, end), '-')) {
                if (dest.length() != 0) {
                    return "";
                }
            }
            return null;
        }
    }
}

class BeginInputSessionInfo {
    public String mText = null;
    // This is NOT the DEngine filter
    public int mDengineInputFilter = 0;
    public int mSelStart = 0;
    public int mSelCount = 0;

    public InputFilter[] genInputFilters() {
        return InputEditable.genInputFilters(mDengineInputFilter);
    }

    public int getInputType() {
        return NativeContentView.dengineInputFilterToAndroidInputType(mDengineInputFilter);
    }

    public int getSelStart() { return mSelStart; }
    public int getSelCount() { return mSelCount; }
    public int getSelEnd() { return mSelStart + mSelCount; }
}

class NativeContentView extends View {

    // This is REQUIRED to match DEngine::Application::SoftInputFilter on native side.
    static public final int SOFT_INPUT_FILTER_INTEGER = 0;
    static public final int SOFT_INPUT_FILTER_UNSIGNED_INTEGER = 1;
    static public final int SOFT_INPUT_FILTER_FLOAT = 2;
    static public final int SOFT_INPUT_FILTER_UNSIGNED_FLOAT = 3;
    static public int dengineInputFilterToAndroidInputType(int input) {
        int out = 0;
        switch (input)
        {
            case SOFT_INPUT_FILTER_INTEGER:
                out = InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_SIGNED;
                break;
            case SOFT_INPUT_FILTER_UNSIGNED_INTEGER:
                out = InputType.TYPE_CLASS_NUMBER;
                break;
            case SOFT_INPUT_FILTER_FLOAT:
                out = InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_DECIMAL | InputType.TYPE_NUMBER_FLAG_SIGNED;
                break;
            case SOFT_INPUT_FILTER_UNSIGNED_FLOAT:
                out = InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_DECIMAL;
                break;
        }
        return out;
    }

    public DEngineActivity mActivity;
    public InputMethodManager mIMM;
    public BeginInputSessionInfo mBeginInputSessionInfo = null;
    public int mInputConnectionCounter = 0;

    public NativeContentView(DEngineActivity activity) {
        super(activity);

        this.mActivity = activity;
        mIMM = getContext().getSystemService(InputMethodManager.class);
    }

    // This should not be used.
    public NativeContentView(Context activity) {
        super(activity);
        this.mActivity = null;
        System.exit(-1);
    }

    public void beginInputSession(String initialString, int dengineInputFilter) {
        //assert(mBeginInputSessionInfo == null);
        mBeginInputSessionInfo = new BeginInputSessionInfo();
        mBeginInputSessionInfo.mText = initialString;
        mBeginInputSessionInfo.mDengineInputFilter = dengineInputFilter;
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
    }

    @Override
    public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
        //if (mBeginInputSessionInfo == null) {
        //return null;
        //}

        outAttrs.initialSelStart = mBeginInputSessionInfo.getSelStart();
        outAttrs.initialSelEnd = mBeginInputSessionInfo.getSelEnd();
        outAttrs.imeOptions |= EditorInfo.IME_ACTION_DONE;
        outAttrs.imeOptions |= EditorInfo.IME_FLAG_NO_EXTRACT_UI;
        outAttrs.imeOptions |= EditorInfo.IME_FLAG_NO_FULLSCREEN;

        outAttrs.inputType = mBeginInputSessionInfo.getInputType();

        var nextConnectionId = mInputConnectionCounter;
        mInputConnectionCounter += 1;

        var ic = new NativeInputConnection2(
                this,
                mActivity,
                mBeginInputSessionInfo,
                nextConnectionId);
        //mBeginInputSessionInfo = null;
        return ic;
    }

    @Override
    public boolean onCheckIsTextEditor() {
        // This means we don't want the IME to automatically pop up
        // when opening this View.
        return false;
    }
}

class NativeInputConnection extends BaseInputConnection {
    InputEditable mEditable = null;
    public DEngineActivity mActivity;
    public int mInputConnectionID;

    public NativeInputConnection(
            NativeContentView targetView,
            DEngineActivity activity,
            BeginInputSessionInfo info,
            int inputConnectionId) {
        super(targetView, false);

        mActivity = activity;
        mInputConnectionID = inputConnectionId;
    }

    @Override
    public ExtractedText getExtractedText(ExtractedTextRequest request, int flags) {
        ExtractedText returnVal = new ExtractedText();
        returnVal.text = mEditable;
        returnVal.selectionStart = returnVal.text.length();
        returnVal.selectionEnd = returnVal.text.length();
        return returnVal;
    }

    @Override
    public void closeConnection() {
        super.closeConnection();
    }

    @Override
    public boolean commitText (CharSequence text, int newCursorPosition) {
        return super.commitText(text, newCursorPosition);
    }

    @Override
    public boolean performEditorAction (int actionCode) {
        var result = super.performEditorAction(actionCode);
        mActivity.nativeSendEventEndTextInputSession(
            mActivity.mNativeBackendDataPtr);
        return result;
    }

    @Override
    public boolean finishComposingText() {
        return super.finishComposingText();
    }

    @Override
    public Editable getEditable() {
        return mEditable;
    }
}

class NativeInputConnection2 implements InputConnection {
    InputEditable mEditable = null;
    public DEngineActivity mActivity;
    public int mInputConnectionID;
    public int mSelStart = 0;
    public int mSelCount = 0;

    public NativeInputConnection2(
            NativeContentView targetView,
            DEngineActivity activity,
            BeginInputSessionInfo inputInfo,
            int inputConnectionId)
    {
        mActivity = activity;
        mInputConnectionID = inputConnectionId;

        mEditable = new InputEditable(
                activity,
                inputInfo.mText,
                inputInfo.genInputFilters());

        mSelStart = inputInfo.getSelStart();
        mSelCount = inputInfo.getSelCount();
    }

    private int getSelStart() { return mSelStart; }
    private int getSelEnd() { return mSelStart + mSelCount; }

    @Override
    public CharSequence getTextBeforeCursor(int n, int flags) {
        int start = Math.max(0, getSelStart() - n);
        var temp = mEditable.subSequence(start, getSelStart());
        return temp;
    }

    @Override
    public CharSequence getTextAfterCursor(int n, int flags) {
        int end = Math.min(mEditable.length(), getSelEnd() + n);
        var temp = mEditable.subSequence(getSelEnd(), end);
        return temp;
    }

    @Override
    public CharSequence getSelectedText(int flags) {
        var temp = mEditable.subSequence(getSelStart(), getSelEnd());
        return temp;
    }

    @Override
    public int getCursorCapsMode(int reqModes) {
        return InputType.TYPE_NUMBER_FLAG_DECIMAL | InputType.TYPE_NUMBER_FLAG_SIGNED;
    }

    @Override
    public ExtractedText getExtractedText(ExtractedTextRequest request, int flags) {
        var returnVal = new ExtractedText();
        returnVal.text = mEditable;
        returnVal.selectionStart = getSelStart();
        returnVal.selectionEnd = getSelEnd();
        return returnVal;
    }

    @Override
    public boolean deleteSurroundingText(int beforeLength, int afterLength) {
        return false;
    }

    @Override
    public boolean deleteSurroundingTextInCodePoints(int beforeLength, int afterLength) {
        return false;
    }

    @Override
    public boolean setComposingText(CharSequence text, int newCursorPosition) {
        return false;
    }

    @Override
    public boolean setComposingRegion(int start, int end) {
        return false;
    }

    @Override
    public boolean finishComposingText() {
        return false;
    }

    @Override
    public boolean commitText(CharSequence text, int newCursorPosition) {
        assert(false);
        // Not implemented
        return false;
    }

    @Override
    public boolean commitCompletion(CompletionInfo text) {
        return false;
    }

    @Override
    public boolean commitCorrection(CorrectionInfo correctionInfo) {
        return false;
    }

    @Override
    public boolean setSelection(int start, int end) {
        assert(start <= end);
        mSelStart = start;
        mSelCount = end - start;
        return true;
    }

    @Override
    public boolean performEditorAction(int editorAction) {
        mActivity.nativeSendEventEndTextInputSession(mActivity.mNativeBackendDataPtr);
        return true;
    }

    @Override
    public boolean performContextMenuAction(int id) {
        return false;
    }

    @Override
    public boolean beginBatchEdit() {
        return false;
    }

    @Override
    public boolean endBatchEdit() {
        return false;
    }

    @Override
    public boolean sendKeyEvent(KeyEvent event) {
        boolean isNumber = true;

        int unicode = event.getUnicodeChar();
        int action = event.getAction();
        int keycode = event.getKeyCode();

        if (isNumber && action == KeyEvent.ACTION_UP) {
            // Check if we are pushing a number
            if (unicode != 0) {
                mEditable.append((char)unicode);
            } else {

                boolean deleteOneChar =
                        keycode == KeyEvent.KEYCODE_DEL &&
                        mSelCount == 0 &&
                        getSelStart() > 0;
                if (deleteOneChar) {
                    mEditable.delete(getSelStart() - 1, getSelStart());
                    mSelStart = getSelStart() - 1;
                    mSelCount = 0;
                }

            }
        }

        return true;
    }

    @Override
    public boolean clearMetaKeyStates(int states) {
        return false;
    }

    @Override
    public boolean reportFullscreenMode(boolean enabled) {
        return false;
    }

    @Override
    public boolean performPrivateCommand(String action, Bundle data) {
        return false;
    }

    @Override
    public boolean requestCursorUpdates(int cursorUpdateMode) {
        return false;
    }

    @Override
    public Handler getHandler() {
        return null;
    }

    @Override
    public void closeConnection() {
    }

    @Override
    public boolean commitContent(InputContentInfo inputContentInfo, int flags, Bundle opts) {
        return false;
    }
}