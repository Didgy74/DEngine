package didgy.dengine;

import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.view.View;

public class MainActivity extends Activity {

    static {
        System.loadLibrary("dengine");
    }

    public native void nativeTestFunc();

    static class NativeView extends View
    {
        public NativeView(Context context) {
            super(context);
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        nativeTestFunc();




        super.onCreate(savedInstanceState);
    }
}
