package didgy.dengine;

import android.annotation.SuppressLint;

import org.jetbrains.annotations.NotNull;

import java.lang.annotation.Native;

// TODO: In the future we should try to only store the View, and not the Activity.
// It could potentially allow us to support DEngine Views inside another View type in the future
public class NativeToAndroid {
    // Tells the Activity that it should open text input session
    @SuppressLint({})
    public static void openSoftInput(
        @NotNull DEngineActivity activity,
        @NotNull String text,
        int selectionStart,
        int selectionCount,
        int softInputFilter)
    {
        activity.contentView.fromNative_beginInputSession(
            text,
            selectionStart,
            selectionCount,
            softInputFilter);
    }

    // Updates the entire string. Cancels any ongoing batch edits or whatever
    // Returns true if successful
    @SuppressLint({})
    public static boolean updateInputConnection(
        @NotNull DEngineApp app,
        @NotNull DEngineActivity activity,
        long activityWindowId,
        int selectionStart,
        int selectionCount,
        byte[] textData)
    {
        return activity.contentView.dengineFromNativeUpdateInputConnection(
            selectionStart,
            selectionCount,
            textData);
    }

    @SuppressLint({})
    public static void accessibilityUpdate(
        @NotNull DEngineApp app,
        @NotNull DEngineActivity activity,
        long activityWindowId,
        long[] widgetId,
        int[] posX,
        int[] posY,
        int[] width,
        int[] height,
        boolean[] clickable,
        int[] textStart,
        int[] textCount,
        byte[] textData)
    {
        activity.fromNative_accessibilityUpdate(
            widgetId,
            posX,
            posY,
            width,
            height,
            clickable,
            textStart,
            textCount,
            textData);
    }

    @SuppressLint({})
    public static int[] getWindowInsets(
        @NotNull DEngineApp app,
        @NotNull DEngineActivity activity)
    {
        return new int[8];
    }

}