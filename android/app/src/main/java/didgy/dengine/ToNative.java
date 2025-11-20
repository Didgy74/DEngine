package didgy.dengine;

import android.content.res.AssetManager;
import android.view.InputQueue;
import android.view.Surface;

public class ToNative {

    // Return type is the pointer to the backend-data
    static native long init(
        DEngineApp app,
        Class<DEngineApp> appClass,
        Class<NativeToAndroid> nativeToAndroidClass,
        DEngineActivity activity,
        AssetManager assetMgr,
        float fontScale,
        int touchScrollSlopPx,
        int scrollBarWidthPx);

    /*
    EventType:
    0 -> TextReplace
    1 -> SetSelection

    This will send multiple text input events to the native code at once, the actual text data is
    in the String parameter. The int array is actually a struct-array similar to
    struct {
        int oldStart
        int oldCount
        int textOffset
        int textCount
    }

    Considerations:
        -   I don't think it's necessary to change the type of the string paramater, because we have
            to turn it into something like a Span<u32> internally anyways
    */
    static native void onTextInput(
        long backendPtr,
        int[] info_eventType,
        int[] info_textReplaceOldStart,
        int[] info_textReplaceOldCount,
        int[] info_textReplaceTextOffset,
        int[] info_textReplaceTextCount,
        int[] info_setSelectionSelStart,
        int[] info_setSelectionSelCount,
        String allStrings);

    static native void sendEventEndTextInputSession(long backendPtr);

    static native void onNewOrientation(long backendPtr, int newOrientation);

    static native void onFontScale(long backendPtr, long windowId, float newScale);

    // TODO: Doesn't take window ID into account.
    static native void onApplyWindowInsets(
        long backendPtr,
        int systemBarLeft,
        int systemBarRight,
        int systemBarTop,
        int systemBarBottom,
        int imeLeft,
        int imeRight,
        int imeTop,
        int imeBottom);
    static native void onNativeWindowCreated(
        long backendPtr,
        Surface surface);

    static native void onNativeWindowDestroyed(
        long backendPtr,
        Surface surface);

    static native void onInputQueueCreated(
        long backendPtr,
        DEngineActivity activity,
        long windowId,
        InputQueue queue);
    static native void onInputQueueDestroyed(
        long backendPtr,
        DEngineActivity activity,
        long windowId,
        InputQueue queue);
}
