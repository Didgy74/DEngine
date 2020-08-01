package didgy.dengine.editor;

import android.app.NativeActivity;
import android.content.Context;
import android.os.Bundle;
import android.text.Editable;
import android.text.InputFilter;
import android.text.InputType;
import android.view.KeyEvent;
import android.view.View;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.InputMethodManager;

public class DEngineActivity extends NativeActivity {

    static {
        System.loadLibrary("dengine");
    }

    public native void nativeTestFunc(int utfValue);
    public native void charRemoveEventNative();

    static class NativeEditable implements Editable {
        public NativeEditable(DEngineActivity activity)
        {
            this.activity = activity;
        }
        DEngineActivity activity;

        @Override
        public Editable replace(int st, int en, CharSequence source, int start, int end) {

            if (source.length() > 0)
            {
                activity.nativeTestFunc(source.charAt(end - 1));
            }

            return this;
        }

        @Override
        public Editable replace(int st, int en, CharSequence text) {
            return replace(st, en, text, 0, text.length());
        }

        @Override
        public Editable insert(int where, CharSequence text, int start, int end) {
            return null;
        }

        @Override
        public Editable insert(int where, CharSequence text) {
            return null;
        }

        @Override
        public Editable delete(int st, int en) {
            return null;
        }

        @Override
        public Editable append(CharSequence text) {

            return null;
        }

        @Override
        public Editable append(CharSequence text, int start, int end) {
            return null;
        }

        @Override
        public Editable append(char text) {
            return null;
        }

        @Override
        public void clear() {

        }

        @Override
        public void clearSpans() {

        }

        @Override
        public void setFilters(InputFilter[] filters) {

        }

        @Override
        public InputFilter[] getFilters() {
            return new InputFilter[0];
        }

        @Override
        public void getChars(int start, int end, char[] dest, int destoff) {

        }

        @Override
        public void setSpan(Object what, int start, int end, int flags) {

        }

        @Override
        public void removeSpan(Object what) {

        }

        @Override
        public <T> T[] getSpans(int start, int end, Class<T> type) {
            return null;
        }

        @Override
        public int getSpanStart(Object tag) {
            return -1;
        }

        @Override
        public int getSpanEnd(Object tag) {
            return -1;
        }

        @Override
        public int getSpanFlags(Object tag) {
            return 0;
        }

        @Override
        public int nextSpanTransition(int start, int limit, Class type) {
            return 0;
        }

        @Override
        public int length() {
            return 0;
        }

        @Override
        public char charAt(int index) {
            return 0;
        }

        @Override
        public CharSequence subSequence(int start, int end) {
            return null;
        }

        @Override
        public String toString() {
            return null;
        }


    }

    public NativeEditable myEditable = null;

    static class NativeInputConnection extends BaseInputConnection {

        public NativeInputConnection(
                View targetView,
                DEngineActivity activity) {
            super(targetView, true);

            this.activity = activity;
        }

        public DEngineActivity activity;

        @Override
        public Editable getEditable() {
            return activity.myEditable;
        }



        @Override
        public boolean sendKeyEvent(KeyEvent event) {
            int keyAction = event.getAction();
            if (keyAction == KeyEvent.ACTION_UP)
            {
                int test = event.getUnicodeChar();
                if (48 <= test && test <= 57)
                    activity.nativeTestFunc(test);

                int yo = event.getKeyCode();
                if (yo == KeyEvent.KEYCODE_DEL)
                {
                    activity.charRemoveEventNative();
                }
            }

            return super.sendKeyEvent(event);
        }
    }

    static class NativeContentView extends View {
        public NativeContentView(DEngineActivity activity) {
            super(activity);

            this.activity = activity;
        }

        public DEngineActivity activity;

        @Override
        public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
            outAttrs.inputType = InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_DECIMAL | InputType.TYPE_NUMBER_FLAG_SIGNED;
            //outAttrs.inputType = InputType.TYPE_CLASS_TEXT;
            return new NativeInputConnection(this, activity);
        }
    }

    public NativeContentView mNativeContentView = null;

    @Override
    protected void onCreate(Bundle savedState) {
        super.onCreate(savedState);

        mNativeContentView = new NativeContentView(this);
        mNativeContentView.setFocusableInTouchMode(true);
        setContentView(mNativeContentView);
        mNativeContentView.requestFocus();
        mNativeContentView.getViewTreeObserver().addOnGlobalLayoutListener(this);



        myEditable = new NativeEditable(this);

    }

    @Override
    public void onGlobalLayout() {
        super.onGlobalLayout();
    }

    public void openSoftInput()
    {
        Runnable test = new Runnable() {
            public void run() {

                InputMethodManager imm = (InputMethodManager)
                        getSystemService(Context.INPUT_METHOD_SERVICE);
                assert imm != null;
                imm.showSoftInput(mNativeContentView, InputMethodManager.SHOW_IMPLICIT);

            }
        };
        runOnUiThread(test);
    }

    public int getCurrentOrientation()
    {
        return getResources().getConfiguration().orientation;
    }
}
