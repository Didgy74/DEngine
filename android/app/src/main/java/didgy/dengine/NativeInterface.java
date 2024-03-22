package didgy.dengine;

import android.content.res.AssetManager;
import android.view.InputQueue;
import android.view.Surface;

public class NativeInterface {

    // Return type is the pointer to the backend-data
    static native long init(
        DEngineApp app,
        Class<DEngineApp> appClass,
        DEngineActivity activity,
        AssetManager assetMgr,
        float fontScale);

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
    static native void onTextInput(
        long backendPtr,
        int[] info_oldStart,
        int[] info_oldCount,
        int[] info_textOffset,
        int[] info_textCount,
        String allStrings);

    static native void sendEventEndTextInputSession(long backendPtr);

    static native void onNewOrientation(long backendPtr, int newOrientation);

    static native void onFontScale(long backendPtr, int windowId, float newScale);

    static native void onContentRectChanged(
        long backendPtr,
        int posX,
        int posY,
        int width,
        int height);

    static native void onNativeWindowCreated(
        long backendPtr,
        Surface surface);

    static native void onNativeWindowDestroyed(
        long backendPtr,
        Surface surface);

    static native void onInputQueuCreated(
        long backendPtr,
        InputQueue queue);

    static native void onTouch(
        long backendPtr,
        int pointerId,
        float x,
        float y,
        int action);

    static void NativeEvent_AccessibilityUpdate(int windowId, int[] dataArray, byte[] textArray) {

    }
}
