package didgy.dengine;

import android.app.NativeActivity;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.os.Bundle;
import android.view.SurfaceHolder;
import android.view.inputmethod.InputMethodManager;

import java.util.ArrayList;

public class DEngineActivity extends NativeActivity {

    static {
        System.loadLibrary("dengine");
    }

    // Return type is the pointer to the backend-data
    public native double nativeInit(float fontScale);

    public static final int NATIVE_TEXT_JOB_MEMBER_COUNT = 4;
    /*
        This will send multiple text input events to
        the native code at once, the actual text data is in
        the String parameter. The int array is actually a struct-array
        similar to
        struct {
            int oldStart
            int oldCount
            int textOffset
            int textCount
        }
    */
    public native void nativeOnTextInput(
        double backendDataPtr,
        int[] infos,
        String text);
    public static class NativeTextInputJob {
        int start;
        int count;
        CharSequence text;
        NativeTextInputJob(int start, int count, CharSequence text) {
            assert start >= 0;
            this.start = start;
            this.count = count;
            this.text = text;
        }
    }
    // Input text jobs can be submitted in batches, therefore
    // we want signal all of them at once when that happens.
    // This function will turn the data into the linear arrays
    // that the native code expects
    public void nativeOnTextInputWrapper(NativeTextInputJob[] inputJobs) {

        var stringBuilder = new StringBuilder();
        var tempInts = new ArrayList<Integer>();

        for (var item : inputJobs) {
            assert item.text != null;
            assert item.count != 0 || item.text.length() != 0;

            var stringOffset = stringBuilder.length();

            // Push our four members
            tempInts.add(item.start);
            tempInts.add(item.count);
            tempInts.add(stringOffset);
            tempInts.add(item.text.length());
            stringBuilder.append(item.text);
        }
        assert tempInts.size() % NATIVE_TEXT_JOB_MEMBER_COUNT == 0;
        int[] arr = tempInts.stream()
                .mapToInt(i -> i)
                .toArray();
        var outString = stringBuilder.toString();
        nativeOnTextInput(
                mNativeBackendDataPtr,
                arr,
                outString);
    }
    public native void nativeSendEventEndTextInputSession(double backendDataPtr);
    public native void nativeOnNewOrientation(double backendDataPtr, int newOrientation);
    public void nativeOnNewOrientationWrapper(int newOrientation) {
        nativeOnNewOrientation(mNativeBackendDataPtr, newOrientation);
    }
    public native void nativeOnFontScale(double backendDataPtr, int windowId, float newScale);
    public void nativeOnFontScaleWrapper(float newScale) {
        assert mWindowId != INVALID_WINDOW_ID;
        nativeOnFontScale(mNativeBackendDataPtr, mWindowId, newScale);
    }
    public native void nativeOnContentRectChanged(
            double backendDataPtr,
            int posX,
            int posY,
            int width,
            int height);
    public void nativeOnContentRectChangedWrapper(
            int posX,
            int posY,
            int width,
            int height) {
        nativeOnContentRectChanged(
                mNativeBackendDataPtr,
                posX, posY,
                width, height);
    }

    public double mNativeBackendDataPtr = 0;
    public NativeView mNativeContentView = null;
    public Configuration mCurrConfig = null;
    public static final int INVALID_WINDOW_ID = -1;
    public int mWindowId = INVALID_WINDOW_ID;

    int mLastContentX = 0;
    int mLastContentY = 0;
    int mLastContentWidth = 0;
    int mLastContentHeight = 0;

    @Override
    protected void onCreate(Bundle savedState) {

        super.onCreate(savedState);

        mNativeContentView = new NativeView(this);

        setContentView(mNativeContentView);
        mNativeContentView.getViewTreeObserver().addOnGlobalLayoutListener(this);
        mNativeContentView.setFocusable(true);
        mNativeContentView.setFocusableInTouchMode(true);

        var config = new Configuration(getResources().getConfiguration());
        mCurrConfig = config;

        var metrics = getResources().getDisplayMetrics();

        var fontScale = config.fontScale;
        mNativeBackendDataPtr = nativeInit(fontScale);
    }

    @Override
    protected void onStart() {
        super.onStart();
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        assert mCurrConfig != null;

        super.onConfigurationChanged(newConfig);

        int diff = newConfig.diff(mCurrConfig);

        if ((diff & ActivityInfo.CONFIG_FONT_SCALE) != 0) {
            nativeOnFontScaleWrapper(newConfig.fontScale);
        }

        if ((diff & ActivityInfo.CONFIG_ORIENTATION) != 0) {
            nativeOnNewOrientationWrapper(newConfig.orientation);
        }

        mCurrConfig = newConfig;
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
        runOnUiThread(() ->
                this.mNativeContentView.beginInputSession(
                    text,
                    NativeInputFilter.fromNativeEnum(softInputFilter)));
    }

    public void nativeEvent_hideSoftInput() {
        var temp = mNativeContentView;
        runOnUiThread(temp::endInputSession);
    }

    public void nativeEvent_createWindow(int windowId) {
        assert windowId != INVALID_WINDOW_ID;
        assert mWindowId == INVALID_WINDOW_ID;
        mWindowId = windowId;
    }
}

class BeginInputSessionInfo {
    public DEngineActivity mActivity = null;
    public NativeView mView = null;
    public String mText = null;
    public NativeInputFilter inputFilter = null;
    public InputMethodManager mIMM = null;

    public int mSelStart = 0;
    public int mSelCount = 0;

    public int getSelStart() { return mSelStart; }
    public int getSelCount() { return mSelCount; }
    public int getSelEnd() { return mSelStart + mSelCount; }
}

