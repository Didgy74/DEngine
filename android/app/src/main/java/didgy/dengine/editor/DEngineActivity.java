package didgy.dengine.editor;

import android.app.NativeActivity;
import android.content.Context;
import android.os.Bundle;
import android.text.InputType;
import android.util.AttributeSet;
import android.view.KeyEvent;
import android.view.View;
import android.view.WindowManager;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.InputMethodManager;

public class DEngineActivity extends NativeActivity {

    static class NativeInputConnection extends BaseInputConnection {

        public NativeInputConnection(View targetView, boolean fullEditor) {
            super(targetView, fullEditor);
        }

        @Override
        public boolean sendKeyEvent(KeyEvent event) {
            int test = event.getUnicodeChar();

            return super.sendKeyEvent(event);
        }
    }

    static class NativeContentView extends View {
        public NativeContentView(Context context) {
            super(context);
        }

        @Override
        public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
            outAttrs.inputType = InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_DECIMAL | InputType.TYPE_NUMBER_FLAG_SIGNED;
            //outAttrs.inputType = InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS;
            return new NativeInputConnection(this, false);
        }
    }

    private NativeContentView mNativeContentView = null;


    @Override
    protected void onCreate(Bundle savedState) {
        super.onCreate(savedState);

        mNativeContentView = new NativeContentView(this);
        mNativeContentView.setFocusableInTouchMode(true);
        setContentView(mNativeContentView);
        mNativeContentView.requestFocus();
    }

    @Override
    public void onGlobalLayout() {
        super.onGlobalLayout();
    }

    public void openSoftInput()
    {
        runOnUiThread(new Runnable() {
            public void run() {

                InputMethodManager imm = (InputMethodManager)
                        getSystemService(Context.INPUT_METHOD_SERVICE);
                assert imm != null;
                imm.showSoftInput(mNativeContentView, InputMethodManager.SHOW_IMPLICIT);

            }
        });
    }

    public int getCurrentOrientation()
    {
        return getResources().getConfiguration().orientation;
    }
}
