// BIG NOTE:
// This class doesn't work because NDK has no function for grabbing AInputQueue.

package didgy.dengine;

import android.app.Activity;
import android.content.Context;
import android.content.res.AssetManager;
import android.content.res.Configuration;
import android.graphics.Insets;
import android.os.Bundle;
import android.view.InputQueue;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.View;
import android.view.ViewTreeObserver;
import android.view.WindowInsets;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

public class MainActivity extends Activity implements SurfaceHolder.Callback2,
  InputQueue.Callback, ViewTreeObserver.OnGlobalLayoutListener {

    public native void nativeOnCreate(AssetManager assetManager);
    public native void nativeOnStart();
    public native void nativeOnSurfaceCreated(Surface aNativeWindow);
    public native void nativeOnInputQueueCreated(InputQueue inputQueue);

    public InputQueue curInputQueue = null;

    static class NativeView extends View
    {
        public NativeView(Context context) {
            super(context);
        }

        @Override
        public WindowInsets onApplyWindowInsets(WindowInsets insets) {

            Insets test = insets.getStableInsets();
            Insets test2 = insets.getSystemWindowInsets();

            android.view.DisplayCutout yo = insets.getDisplayCutout();

            return insets;
        }
    }

    public NativeView mNativeView = null;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        getWindow().takeSurface(this);
        getWindow().takeInputQueue(this);

        System.loadLibrary("dengine");

        mNativeView = new NativeView(this);
        this.setContentView(mNativeView);
        mNativeView.requestFocus();
        mNativeView.getViewTreeObserver().addOnGlobalLayoutListener(this);

        AssetManager assetmanager = getAssets();
        nativeOnCreate(assetmanager);
    }

    @Override
    protected void onStart() {
        super.onStart();
        nativeOnStart();
    }

    @Override
    protected void onStop() {
        super.onStop();
    }

    @Override
    public void onConfigurationChanged( Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
    }

    @Override
    public void onLowMemory() {
        super.onLowMemory();
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
    }

    @Override
    public void onGlobalLayout() {
        android.view.Display test = mNativeView.getDisplay();

        android.graphics.Rect globalRect = new android.graphics.Rect();
        android.graphics.Point globalOffset = new android.graphics.Point();
        mNativeView.getGlobalVisibleRect(globalRect, globalOffset);

        int top = mNativeView.getTop();

        WindowInsets wInsets = mNativeView.getRootWindowInsets();

        int width = mNativeView.getWidth();
        int height = mNativeView.getHeight();
    }

    @Override
    public void onInputQueueCreated(InputQueue queue) {
        curInputQueue = queue;
        Class c;
        try {
            c = Class.forName("android.view.InputQueue");
            Method[] test = c.getMethods();

            int i = test.length;

        } catch (ClassNotFoundException e) {
            e.printStackTrace();
        }
        nativeOnInputQueueCreated(queue);
    }

    @Override
    public void onInputQueueDestroyed(InputQueue queue) {

    }

    @Override
    public void surfaceRedrawNeeded(SurfaceHolder holder) {

    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        nativeOnSurfaceCreated(holder.getSurface());
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {

    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {

    }
}
