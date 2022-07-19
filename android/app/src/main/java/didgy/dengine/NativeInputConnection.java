package didgy.dengine;

import android.os.Bundle;
import android.os.Handler;
import android.text.SpannableStringBuilder;
import android.text.TextUtils;
import android.util.Log;
import android.view.KeyEvent;
import android.view.inputmethod.CompletionInfo;
import android.view.inputmethod.CorrectionInfo;
import android.view.inputmethod.CursorAnchorInfo;
import android.view.inputmethod.ExtractedText;
import android.view.inputmethod.ExtractedTextRequest;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.InputContentInfo;
import android.view.inputmethod.InputMethodManager;

import java.util.ArrayList;

class NativeInputConnection implements InputConnection {
    public DEngineActivity mActivity = null;
    public NativeView mView = null;
    public InputMethodManager mIMM = null;
    public SpannableStringBuilder mEditable = null;
    public NativeInputFilter mInputFilter;

    public int mSelStart = 0;
    public int mSelCount = 0;
    public int mPreBatchSelStart = 0;
    public int mPreBatchSelCount = 0;
    public int mBatchEditCounter = 0;
    public static class ArrayRange {
        int start = -1;
        int count = -1;
        ArrayRange(int start, int count) {
            this.start = start;
            this.count = count;
        }
    }
    public ArrayRange mComposingRange = null;

    static class ReplaceTextJob {
        int start;
        int count;
        CharSequence text;
        ReplaceTextJob(int start, int count, CharSequence text) {
            this.start = start;
            this.count = count;
            this.text = text;
        }
    }
    public ArrayList<ReplaceTextJob> replaceTextJobs = new ArrayList<>();
    public void flushReplaceTextJobs() {
        var nativeJobs = new ArrayList<DEngineActivity.NativeTextInputJob>();
        for (var item : replaceTextJobs) {
            assert item.start <= mEditable.length();
            assert item.count != 0 || item.text.length() != 0;

            mEditable.replace(item.start, item.start + item.count, item.text);
            nativeJobs.add(
                    new DEngineActivity.NativeTextInputJob(
                            item.start,
                            item.count,
                            item.text));
        }
        if (replaceTextJobs.size() > 0) {
            Log.d("DEngine", "Current text: " + mEditable);
        }
        replaceTextJobs.clear();

        if (nativeJobs.size() > 0) {
            var tempJobList = new DEngineActivity.NativeTextInputJob[nativeJobs.size()];
            nativeJobs.toArray(tempJobList);
            mActivity.nativeOnTextInputWrapper(tempJobList);
        }
    }
    public void pushReplaceTextJob(int start, int count, CharSequence text) {
        assert start >= 0;
        assert count >= 0;
        assert count != 0 || text.length() != 0;

        replaceTextJobs.add(new ReplaceTextJob(start, count, text));
        if (mBatchEditCounter == 0) {
            flushReplaceTextJobs();
        }
    }

    public NativeInputConnection(
            NativeView targetView,
            BeginInputSessionInfo inputInfo)
    {
        mActivity = inputInfo.mActivity;
        mView = inputInfo.mView;
        mIMM = inputInfo.mIMM;
        mEditable = new SpannableStringBuilder(inputInfo.mText);
        mInputFilter = inputInfo.inputFilter;

        mSelStart = inputInfo.getSelStart();
        mSelCount = inputInfo.getSelCount();
    }

    private int getSelStart() { return mSelStart; }
    private int getSelEnd() { return mSelStart + mSelCount; }
    private int getSelCount() { return mSelCount; }

    private void signalNewSelectionToIMM() {
        if (mBatchEditCounter == 0) {
            mIMM.updateSelection(
                    mView,
                    getSelStart(),
                    getSelEnd(),
                    -1,
                    -1);
        }
    }

    // This is meant to be called after a replace operation.
    // For the second argument you will want to pass
    // `replaceStart + text.length() - 1
    private void setRelativeCursorPos(int newCursorPos, int replaceEnd) {
        // Update the cursor
        // If >0 the pos is relative to text.length() - 1
        // Otherwise it is relative to text beginning.
        if (newCursorPos > 0)
            mSelStart = replaceEnd + newCursorPos;
        else
            mSelStart += newCursorPos;
        mSelCount = 0;
    }

    interface DoSomething {
        // Returns whether we did anything.
        boolean run(NativeInputConnection ic);
    }
    private void doStuffAndUpdateSel(DoSomething[] jobs) {
        assert(jobs != null);

        var didAnything = false;
        for (var item : jobs) {
            var result = item.run(this);
            didAnything = didAnything || result;
        }

        if (mBatchEditCounter == 0 && didAnything) {
            flushReplaceTextJobs();
            //signalNewSelectionToIMM();
        }
    }

    @Override
    public CharSequence getTextBeforeCursor(int n, int flags) {
        int start = Math.max(0, getSelStart() - n);
        var temp = mEditable.subSequence(start, getSelStart());
        return temp;
    }

    @Override
    public CharSequence getTextAfterCursor(int n, int flags) {
        //assert(flags != GET_TEXT_WITH_STYLES);
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
        return TextUtils.getCapsMode(mEditable, getSelStart(), reqModes);
    }

    @Override
    public boolean requestCursorUpdates(int cursorUpdateMode) {
        var builder = new CursorAnchorInfo.Builder();
        builder.setSelectionRange(getSelStart(), getSelEnd());
        var done = builder.build();
        mIMM.updateCursorAnchorInfo(mView, done);
        return true;
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
        var jobs = new ArrayList<DoSomething>();

        if (beforeLength > 0) {
            jobs.add((ic) -> {
                assert ic.getSelStart() - beforeLength >= 0;

                int replaceStart = ic.getSelStart() - beforeLength;
                // There is potentially functionality we need to fix here.
                if (mComposingRange != null) {
                    mComposingRange.start -= beforeLength;
                }
                // Then move the cursor
                mSelStart -= beforeLength;

                pushReplaceTextJob(replaceStart, beforeLength, "");
                return true;
            });
        }

        if (afterLength > 0) {
            assert(false);
        }

        if (jobs.size() > 0) {
            var tempList = new DoSomething[jobs.size()];
            jobs.toArray(tempList);
            doStuffAndUpdateSel(tempList);
        }

        return true;
    }

    @Override
    public boolean deleteSurroundingTextInCodePoints(int beforeLength, int afterLength) {
        assert false;
        return false;
    }

    @Override
    public boolean setComposingText(CharSequence inputText, int newCursorPos) {
        DoSomething job = (ic) -> {
            var replaceStart = getSelStart();
            var replaceCount = getSelCount();
            if (mComposingRange != null) {
                replaceStart = mComposingRange.start;
                replaceCount = mComposingRange.count;
            }

            var filteredText = mInputFilter.filterText(
                    mEditable.toString(),
                    replaceStart,
                    replaceCount,
                    inputText);
            if (filteredText.length() == 0 && inputText.length() != 0)
                return false;

            if (filteredText.length() == 0) {
                mComposingRange = null;
            } else {
                mComposingRange = new ArrayRange(replaceStart, filteredText.length());
            }

            setRelativeCursorPos(newCursorPos, replaceStart + filteredText.length() - 1);
            // This function might be called with no text to replace,
            // and no text to insert, just to remove our composing span.
            // So if that happens, we don't want to submit it.
            if (replaceCount != 0 || filteredText.length() != 0)
                pushReplaceTextJob(replaceStart, replaceCount, filteredText);
            return true;
        };

        doStuffAndUpdateSel(new DoSomething[]{ job });
        return true;
    }

    @Override
    public boolean setComposingRegion(int start, int end) {
        assert start >= 0;
        assert start <= end;
        if (start - end == 0) {
            mComposingRange = null;
        } else {
            mComposingRange = new ArrayRange(start, end - start);
        }
        return true;
    }

    @Override
    public boolean finishComposingText() {
        //assert(mComposingRange != null);
        mComposingRange = null;
        return true;
    }

    @Override
    public boolean commitText(CharSequence inputText, int newCursorPos) {
        DoSomething job = (NativeInputConnection ic) -> {
            var replaceStart = getSelStart();
            var replaceCount = getSelCount();
            if (mComposingRange != null) {
                replaceStart = mComposingRange.start;
                replaceCount = mComposingRange.count;
            }
            var filteredText = mInputFilter.filterText(
                    mEditable.toString(),
                    replaceStart,
                    replaceCount,
                    inputText);
            if (filteredText.length() == 0 && inputText.length() != 0)
                return false; // We didn't do anything so return false?

            mComposingRange = null;
            setRelativeCursorPos(newCursorPos, replaceStart + filteredText.length() - 1);
            pushReplaceTextJob(replaceStart, replaceCount, filteredText);
            return true;
        };

        doStuffAndUpdateSel(new DoSomething[]{ job });
        return true;
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
        if (start != getSelStart() || end != getSelEnd()) {
            mSelStart = start;
            mSelCount = end - start;
            signalNewSelectionToIMM();
        }
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
        if (mBatchEditCounter == 0) {
            mPreBatchSelStart = mSelStart;
            mPreBatchSelCount = mSelCount;
        }

        mBatchEditCounter += 1;

        return true;
    }

    @Override
    public boolean endBatchEdit() {
        if (mBatchEditCounter <= 0)
            return false;

        mBatchEditCounter -= 1;

        if (mBatchEditCounter == 0) {
            // Run the batch edits
            flushReplaceTextJobs();
            if (mPreBatchSelStart != getSelStart() || mPreBatchSelCount != mSelCount) {
                signalNewSelectionToIMM();
            }
        }

        return mBatchEditCounter > 0;
    }

    private void pushNumpadNumber(char unicode) {
        DoSomething job = (NativeInputConnection ic) -> {
            var replaceStart = ic.getSelStart();
            var replaceCount = ic.getSelCount();
            assert mComposingRange == null;

            var outString = mInputFilter.filterText(
                    mEditable.toString(),
                    replaceStart,
                    replaceCount,
                    String.valueOf(unicode));
            if (outString.length() == 0)
                return false;

            mSelStart += 1;
            mSelCount = 0;

            pushReplaceTextJob(replaceStart, replaceCount, outString);
            return true;
        };
        doStuffAndUpdateSel(new DoSomething[]{ job });
    }

    @Override
    public boolean sendKeyEvent(KeyEvent event) {
        boolean isNumpad = mInputFilter.isNumpadInput();

        int unicode = event.getUnicodeChar();
        int action = event.getAction();
        int keycode = event.getKeyCode();

        if (isNumpad && action == KeyEvent.ACTION_DOWN && unicode != 0) {
            // It's a valid unicode, push it.
            pushNumpadNumber((char)unicode);
            return true;
        }

        if (action == KeyEvent.ACTION_DOWN && keycode == KeyEvent.KEYCODE_DEL) {
            // It's a valid unicode, push it.
            DoSomething job = (NativeInputConnection ic) -> {
                var replaceStart = ic.getSelStart();
                var replaceCount = ic.getSelCount();

                if (replaceCount == 0 && replaceStart > 0) {
                    // Delete one character behind the cursor
                    replaceStart -= 1;
                    replaceCount = 1;
                    mSelStart -= 1;
                }
                ic.mSelCount = 0;
                if (replaceCount != 0) {
                    pushReplaceTextJob(replaceStart, replaceCount, "");
                }
                return true;
            };
            doStuffAndUpdateSel(new DoSomething[]{ job });
            return true;
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
}