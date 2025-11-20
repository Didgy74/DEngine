package didgy.dengine

import android.graphics.RectF
import android.os.Bundle
import android.os.Handler
import android.os.HandlerThread
import android.text.SpannableStringBuilder
import android.text.TextUtils
import android.util.Log
import android.view.KeyEvent
import android.view.inputmethod.CompletionInfo
import android.view.inputmethod.CorrectionInfo
import android.view.inputmethod.CursorAnchorInfo
import android.view.inputmethod.EditorInfo
import android.view.inputmethod.ExtractedText
import android.view.inputmethod.ExtractedTextRequest
import android.view.inputmethod.InputConnection
import android.view.inputmethod.InputContentInfo
import android.view.inputmethod.InputMethodManager
import android.view.inputmethod.SurroundingText
import android.view.inputmethod.TextAttribute
import android.view.inputmethod.TextBoundsInfoResult
import android.view.inputmethod.TextSnapshot
import java.util.concurrent.CountDownLatch
import java.util.concurrent.Executor
import java.util.function.Consumer


// Returns true if we did anything.
typealias NativeInputConnection_Job = (ic: NativeInputConnection) -> Boolean

// TODO: We might want to research whether we should be using the android.text.Selection span
// I think we can get away without using it.
//
// InputConnection will often batch operations together. In the midst of batches, we must not
// update rendering. We do this by not sending operations to C++ immediately.
// Beginning and ending a batch is low-cost for us, and batches can be nested and changes only
// flushed when the last batch counter reaches 0. The InputConnection may choose to send operations
// without beginning a batch, but our design will always enclose all changes to text with a batch
// regardless, this makes for less spaghetti code, I hope.
class NativeInputConnection(
    var targetView: NativeView,
    val beginInfo: BeginInputSessionInfo,
    val mInputMethodManager: InputMethodManager,
    val mDEngineWindowId: Long,
    handlerThread: HandlerThread) : InputConnection
{
    companion object {
        const val LOG_TAG = "NativeInputConnection"
        const val DEBUG_STUFF = true
    }

    val mDEngineHandler = Handler(handlerThread.looper)
    val activity get() = targetView.dengineActivity
    val inputFilter = beginInfo.inputFilter
    var mBatchEditCounter = 0

    data class ArrayRange(
        val start: Int,
        val count: Int)
    {
        val end: Int get() { return start + count }
    }
    data class State(
        val editable: SpannableStringBuilder,
        val cursor: ArrayRange,
        val composingRange: ArrayRange?,
    ) {
        init {
            assert(cursor.start + cursor.count <= editable.length)
            if (composingRange != null) {
                assert(composingRange.start + composingRange.count <= editable.length)
            }
        }

        fun deepCopy(
            editable: SpannableStringBuilder = SpannableStringBuilder(this.editable),
            selection: ArrayRange = this.cursor.copy(),
            composingRange: ArrayRange? = this.composingRange?.copy()
        ): State {
            return copy(
                editable = editable,
                cursor = selection,
                composingRange = composingRange,
            )
        }
    }
    private var mPreBatchState: State? = null
    private var state = State(
        editable = SpannableStringBuilder(beginInfo.text),
        cursor = ArrayRange(beginInfo.selStart, beginInfo.selCount),
        composingRange = null)
    // Decides whether the IMM wants us to continually update the IMM on text changes using
    // InputMethodManager.updateExtractedText(). NOTE! If there is an on-going batch-edit, then
    // this function should be deferred until after it.
    private var immMonitorsExtractedText = false
    // Contains the token that should be passed into IMM.updateExtractedText()
    // Should only be non-null when immMonitorsExtractedText is true.
    private var immExtractedTextRequest: ExtractedTextRequest? = null
    // Decides whether the IMM wants us to continually update it on cursor changes using
    // InputMethodManager.updaterCursorAnchorInfo(). NOTE! If there is an on-going batch-edit, then
    // this function should be deferred until after it.
    //
    // This flag is only associated with the function requestCursorUpdates. Which means that, for
    // some reason, it is only associated with IMM.updateCursorAnchorInfo and NOT IMM.updateSelection
    // The documentation is not clear on why.
    private var immMonitorsCursor = false
    // Dumb IMEs (GBoard) will sometimes send events like sendKeyEvent to remove single characters,
    // instead of just calling commitText. In which case we will make this key-event modify our text
    // and we need to convey that information to the IMM. This flag lets us track whether we need to
    // force-update the IMM in the case where an event that doesn't imply text changing, actually
    // triggers a text-change.
    private var forceImmTextUpdateAtBatchEnd = false

    fun setEditorInfo(outAttrs: EditorInfo) {
        outAttrs.initialSelStart = beginInfo.selStart
        outAttrs.initialSelEnd = beginInfo.selEnd
        outAttrs.inputType = beginInfo.inputFilter.toAndroidInputType()

        // This should not commonly be used, because it will force-switch the IME to english.
        //outAttrs.hintLocales = LocaleList(Locale.ENGLISH)

        //outAttrs.initialCapsMode = getCursorCapsMode(beginInfo.inputFilter.toAndroidInputType())
        outAttrs.initialCapsMode = dengineGetCursorCapsMode()

        outAttrs.setInitialSurroundingText(state.editable)

        // The Android TextView.java source code says this is valid.
        outAttrs.imeOptions = outAttrs.imeOptions or EditorInfo.IME_ACTION_DONE
    }

    // Please don't use directly
    sealed class ToNativeBatchedTextEventItem {
        data class TextReplace(
            val start: Int,
            val count: Int,
            val text: CharSequence,
            val selStart: Int,
            val selCount: Int
        ) : ToNativeBatchedTextEventItem()

        data class SetSelection(
            val selStart: Int,
            val selCount: Int
        ) : ToNativeBatchedTextEventItem()
    }
    private var mToNativeBatchedTextEventJobs = mutableListOf<ToNativeBatchedTextEventItem>()
    private fun DEngineFlushTextJobsToNative() {
        if (mToNativeBatchedTextEventJobs.isEmpty())
            return

        val combinedString = StringBuilder()
        val eventType = mutableListOf<Int>()
        val textReplaceOldStart = mutableListOf<Int>()
        val textReplaceOldCount = mutableListOf<Int>()
        val textReplaceTextOffset = mutableListOf<Int>()
        val textReplaceTextCount = mutableListOf<Int>()
        val setSelectionSelStart = mutableListOf<Int>()
        val setSelectionSelCount = mutableListOf<Int>()
        for (item in mToNativeBatchedTextEventJobs) {
            when (item) {
                is ToNativeBatchedTextEventItem.TextReplace -> {
                    eventType.add(0)
                    textReplaceOldStart.add(item.start)
                    textReplaceOldCount.add(item.count)
                    textReplaceTextOffset.add(combinedString.length)
                    textReplaceTextCount.add(item.text.length)
                    setSelectionSelStart.add(-1)
                    setSelectionSelCount.add(-1)

                    combinedString.append(item.text)

                    // It doesn't make sense to replace empty substring with empty string
                    assert(!(item.count == 0 && item.text.isEmpty()))
                }
                is ToNativeBatchedTextEventItem.SetSelection -> {
                    eventType.add(1)
                    textReplaceOldStart.add(-1)
                    textReplaceOldCount.add(-1)
                    textReplaceTextOffset.add(-1)
                    textReplaceTextCount.add(-1)
                    setSelectionSelStart.add(item.selStart)
                    setSelectionSelCount.add(item.selCount)
                }
            }
        }
        val allLengths = listOf(
            eventType.size,
            textReplaceOldStart.size,
            textReplaceOldCount.size,
            textReplaceTextOffset.size,
            textReplaceTextOffset.size,
            setSelectionSelStart.size,
            setSelectionSelCount.size)
        assert(allLengths.all { it == eventType.size })

        ToNative.onTextInput(
            targetView.dengineApp.nativeBackendDataPtr,
            eventType.toIntArray(),
            textReplaceOldStart.toIntArray(),
            textReplaceOldCount.toIntArray(),
            textReplaceTextOffset.toIntArray(),
            textReplaceTextCount.toIntArray(),
            setSelectionSelStart.toIntArray(),
            setSelectionSelCount.toIntArray(),
            combinedString.toString())

        mToNativeBatchedTextEventJobs.clear()
    }

    private fun pushToNativeReplaceTextJob(
        start: Int,
        count: Int,
        newText: CharSequence,
        newSelStart: Int,
        newSelCount: Int)
    {
        // The current design requires all changes to enclosed in a batch.
        assert(mBatchEditCounter > 0)
        assert(!(count == 0 && newText.length == 0))
        mToNativeBatchedTextEventJobs.add(ToNativeBatchedTextEventItem.TextReplace(
            start,
            count,
            text = newText,
            selStart = newSelStart,
            selCount = newSelCount))
    }

    private fun pushToNativeSetSelectionJob(
        start: Int,
        count: Int)
    {
        assert(mBatchEditCounter > 0)
        mToNativeBatchedTextEventJobs.add(ToNativeBatchedTextEventItem.SetSelection(
            selStart = start,
            selCount = count))
    }

    fun dengineDispatchCursorAnchorInfoToImm() {
        assert(mBatchEditCounter <= 0)
        // It's possible that we call this even if we don't monitor cursor, such as when cursor is
        // requested without the monitor-cursor flag
        //assert(immMonitorsCursor)

        // NOTE: The documentation is unclear on the semantic differences between
        // IMM.updateSelection and IMM.updateCursorAnchorInfo. The latter seems superior so
        // we only call that.
        val info = CursorAnchorInfo.Builder()
        info.setSelectionRange(state.cursor.start, state.cursor.end)
        state.composingRange?.let {
            info.setComposingText(it.start, state.editable.subSequence(it.start, it.end))
        }
        mInputMethodManager.updateCursorAnchorInfo(targetView, info.build())

        // The documentation is unclear on what "candidates" parameter means, but TextView
        // implementation seems to imply it's the composition region.
        /*
        if (state.composingRange == null) {

        } else {
            mInputMethodManager.updateSelection(
                targetView,
                state.cursor.start,
                state.cursor.end,
                state.composingRange!!.start,
                state.composingRange!!.end)
        }
         */
        mInputMethodManager.updateSelection(
            targetView,
            state.cursor.start,
            state.cursor.end,
            -1,
            -1)
    }

    fun dengineBuildExtractedText(): ExtractedText {
        val text = ExtractedText()
        text.text = state.editable
        text.selectionStart = state.cursor.start
        text.selectionEnd = state.cursor.end
        text.partialStartOffset = -1
        text.partialEndOffset = -1
        text.startOffset = 0
        return text
    }

    fun dengineDispatchExtractedTextToImm() {
        assert(mBatchEditCounter <= 0)
        // This function can sometimes be called as a result of us forcing a sendKeyEvent
        // and turning it into a text-change event.
        //assert(immMonitorsExtractedText)
        //assert(immExtractedTextRequest != null)

        // When we are interpretting a sendKeyEvent as a remove-text-event, we don't necessarily
        // have an on-going ExtractedTextRequest. So instead we use invalidate-input for now,
        // because it doesn't require any token.
        val text = dengineBuildExtractedText()
        if (immExtractedTextRequest != null) {
            mInputMethodManager.updateExtractedText(targetView, immExtractedTextRequest!!.token, text)
        } else {
            //mInputMethodManager.updateExtractedText(targetView, 0, text)
            mInputMethodManager.invalidateInput(targetView)
        }
    }

    fun preMethodOp(name: CharSequence) {
        if (!DEBUG_STUFF)
            return
        Log.d(LOG_TAG, "Method was called '$name'")
    }

    override fun getTextBeforeCursor(n: Int, flags: Int): CharSequence {
        preMethodOp("getTextBeforeCursor")
        val start = (state.cursor.start - n).coerceAtLeast(0)
        return state.editable.subSequence(start, state.cursor.start)
    }

    override fun getTextAfterCursor(n: Int, flags: Int): CharSequence {
        preMethodOp("getTextAfterCursor")
        val end = state.editable.length.coerceAtMost(state.cursor.end + n)
        return state.editable.subSequence(state.cursor.end, end)
    }

    override fun getSelectedText(flags: Int): CharSequence {
        preMethodOp("getSelectedText")
        return state.editable.subSequence(state.cursor.start, state.cursor.end)
    }

    private fun dengineGetCursorCapsMode(reqModes: Int): Int {
        return TextUtils.getCapsMode(
            state.editable.subSequence(state.cursor.start, state.cursor.end),
            0,
            reqModes)
    }
    private fun dengineGetCursorCapsMode(): Int {
        return dengineGetCursorCapsMode(
            TextUtils.CAP_MODE_SENTENCES)
    }

    override fun getCursorCapsMode(reqModes: Int): Int {
        preMethodOp("getCursorCapsMode")
        return dengineGetCursorCapsMode(reqModes)
    }

    override fun requestCursorUpdates(cursorUpdateMode: Int, cursorUpdateFilter: Int): Boolean {
        preMethodOp("requestCursorUpdates")

        immMonitorsCursor = cursorUpdateMode and InputConnection.CURSOR_UPDATE_MONITOR != 0

        // Not sure if this should be deferred on the Handler, but the documentation says
        // "return true if it has been scheduled to receive cursor updates". And also
        // the TextView.onRequestCursorUpdatesInternal seems to update it deferredly.
        if (cursorUpdateMode and InputConnection.CURSOR_UPDATE_IMMEDIATE != 0) {
            dengineDispatchCursorAnchorInfoToImm()
        } else {
            //var postSuccess = mDEngineHandler.post {
            dengineDispatchCursorAnchorInfoToImm()
            //}
        }

        return true
    }

    override fun requestCursorUpdates(cursorUpdateMode: Int): Boolean {
        preMethodOp("requestCursorUpdates")

        // idk, just point it into the other overload?
        TODO()
        return true
    }

    override fun getHandler(): Handler {
        preMethodOp("getHandler")
        return mDEngineHandler
    }

    override fun closeConnection() {
        preMethodOp("closeConnection")
        // TODO: We should flag this connection as closed so we can stop receiving requestes
        Log.e(this.javaClass.name, "Connection was closed but we don't do anything")
    }
    override fun commitContent(
        inputContentInfo: InputContentInfo,
        flags: Int,
        opts: Bundle?
    ): Boolean {
        preMethodOp("commitContent")
        TODO()
        return false
    }

    override fun requestTextBoundsInfo(
        bounds: RectF,
        executor: Executor,
        consumer: Consumer<TextBoundsInfoResult?>
    ) {
        preMethodOp("requestTextBoundsInfo")
        TODO()
        super.requestTextBoundsInfo(bounds, executor, consumer)
    }

    override fun getExtractedText(request: ExtractedTextRequest, flags: Int): ExtractedText {
        preMethodOp("getExtractedText")

        // Docs lack info here, but apparently sometimes we also need to call
        // InputMethodManager.updateExtractedText whenever we need to update selection as well.
        if (flags and InputConnection.GET_EXTRACTED_TEXT_MONITOR != 0) {
            immMonitorsExtractedText = true
            immExtractedTextRequest = request
        } else if (flags and InputConnection.GET_EXTRACTED_TEXT_MONITOR == 0) {
            immMonitorsExtractedText = false
            immExtractedTextRequest = null
        }

        val returnVal = dengineBuildExtractedText()
        return returnVal
    }

    override fun getSurroundingText(
        beforeLength: Int,
        afterLength: Int,
        flags: Int): SurroundingText
    {
        preMethodOp("getSurroundingText")
        val returnValue = SurroundingText(
            state.editable,
            state.cursor.start,
            state.cursor.end,
            0)
        return returnValue
    }

    override fun deleteSurroundingText(beforeLength: Int, afterLength: Int): Boolean {
        // We probably want to start a batch internally even if the InputConnection didn't request it
        assert(mBatchEditCounter > 0)
        preMethodOp("deleteSurroundingText")
        if (beforeLength <= 0 && afterLength <= 0)
            return true

        // Dunno what to do yet when this thing hits.
        assert(state.composingRange == null)

        // These two operations should always count as one batch?
        beginBatchEditInternal()

        if (beforeLength > 0) {
            // This could maybe be used to re-use some of our other methods?
            val tempCursorStart = (state.cursor.start - beforeLength).coerceAtLeast(0)
            val tempCursorCount = state.cursor.start - tempCursorStart
            if (tempCursorCount > 0) {
                state.editable.replace(
                    tempCursorStart,
                    tempCursorStart + tempCursorCount,
                    "")

                // We don't actually want to change the selection, so we need to restore it but shift
                // it accordingly.
                state = state.copy(cursor = ArrayRange(
                    state.cursor.start - tempCursorCount,
                    state.cursor.count))

                pushToNativeReplaceTextJob(
                    tempCursorStart,
                    tempCursorCount,
                    "",
                    state.cursor.start,
                    state.cursor.count)
            }
        }
        if (afterLength > 0) {
            val tempCursorStart = state.cursor.end
            val tempCursorEnd = (state.cursor.end + afterLength).coerceAtMost(state.editable.length)
            assert(tempCursorStart <= tempCursorEnd)
            val tempCursorCount = tempCursorEnd - tempCursorStart
            if (tempCursorCount > 0) {
                state.editable.replace(
                    tempCursorStart,
                    tempCursorEnd,
                    "")

                pushToNativeReplaceTextJob(
                    tempCursorStart,
                    tempCursorCount,
                    "",
                    state.cursor.start,
                    state.cursor.count)
            }
        }

        // Notifying changes to IMM is done through endBatchEdit
        endBatchEditInternal()

        return true
    }

    override fun deleteSurroundingTextInCodePoints(beforeLength: Int, afterLength: Int): Boolean {
        preMethodOp("deleteSurroundingTextInCodePoints")
        throw RuntimeException("Not implemented")
    }

    fun setComposingTextInternal(
        inputText: CharSequence,
        newCursorPos: Int,
        textAttribute: TextAttribute?) : Boolean
    {
        // We probably want to start a batch internally even if the InputConnection didn't request it
        assert(mBatchEditCounter > 0)
        // Should be similar to commitText?
        var replaceStart = state.cursor.start
        var replaceCount = state.cursor.count
        if (state.composingRange != null) {
            val composingRange = state.composingRange!!
            replaceStart = composingRange.start
            replaceCount = composingRange.count
        }

        // TODO: idk if we should do this in C++ or here. It's useful for numbers I guess.
        val filteredText = inputFilter.filterText(
            state.editable.toString(),
            replaceStart,
            replaceCount,
            inputText)
        if (filteredText.isEmpty() && replaceCount == 0)
            return true

        // Uhhh, notify that we changed something? Something that can be tracked in endBatchEdit
        // to determine if we need to notify the IME?
        state.editable.replace(
            replaceStart,
            replaceStart + replaceCount,
            filteredText)

        // TODO: Update cursor and adjust composing region to match what we inserted
        if (newCursorPos == 1) {
            state = state.copy(
                cursor = ArrayRange(replaceStart + filteredText.length, 0),
                composingRange = ArrayRange(replaceStart, filteredText.length))
        } else {
            TODO()
        }

        pushToNativeReplaceTextJob(
            replaceStart,
            replaceCount,
            filteredText,
            state.cursor.start,
            state.cursor.count)

        return true
    }

    override fun setComposingText(
        text: CharSequence,
        newCursorPosition: Int,
        textAttribute: TextAttribute?
    ): Boolean {
        preMethodOp("setComposingText")
        beginBatchEditInternal()
        val returnVal = setComposingTextInternal(text, newCursorPosition, textAttribute)
        endBatchEditInternal()
        return returnVal
    }

    override fun setComposingText(inputText: CharSequence, newCursorPos: Int): Boolean {
        preMethodOp("setComposingText")
        beginBatchEditInternal()
        val returnVal = setComposingTextInternal(inputText, newCursorPos, null)
        endBatchEditInternal()
        return returnVal
    }

    override fun setComposingRegion(start: Int, end: Int, textAttribute: TextAttribute?): Boolean {
        preMethodOp("setComposingRegion")
        TODO()
        return true
    }

    override fun setComposingRegion(start: Int, end: Int): Boolean {
        preMethodOp("setComposingRegion")
        // TODO: Make sure we resolve indexes so that start is lower than end
        assert(start <= end)

        // Docs say that if the region is zero-sized, this should behave as if unsetting
        // the composing region.
        if (end - start == 0)
            state = state.copy(composingRange = null)
        else
            state = state.copy(composingRange = ArrayRange(start, end - start))
        return true
    }

    override fun finishComposingText(): Boolean {
        preMethodOp("finishComposingText")
        //assert(mComposingRange != null)
        state = state.copy(composingRange = null)
        return true
    }

    // forceUpdateImmText is when we want to signal that this change requires us to signal to the
    // IMM that our text changed, even if the IMM did not request to be notified of such changes.
    // This can happen i.e as a result of us treating a sendKeyEvent to change the text, even
    // if the IMM doesn't expect such an event so change the text.
    private fun commitTextInternal(
        inputText: CharSequence,
        newCursorPos: Int,
        forceUpdateImmText: Boolean): Boolean
    {
        assert(mBatchEditCounter > 0)

        var replaceStart = state.cursor.start
        var replaceCount = state.cursor.count
        if (state.composingRange != null) {
            val composingRange = state.composingRange!!
            replaceStart = composingRange.start
            replaceCount = composingRange.count
        }

        // TODO: idk if we should do this in C++ or here. It's useful for numbers I guess.
        val filteredText = inputFilter.filterText(
            state.editable.toString(),
            replaceStart,
            replaceCount,
            inputText)
        if (filteredText.isEmpty() && replaceCount == 0)
            return true

        state.editable.replace(
            replaceStart,
            replaceStart + replaceCount,
            filteredText)

        // TODO: Update cursor
        // When we are calling commitText (not setComposingText), we should always clear
        // the composing range.
        if (newCursorPos == 1) {
            state = state.copy(
                cursor = ArrayRange(replaceStart + filteredText.length, 0),
                composingRange = null)
        } else {
            TODO()
        }

        pushToNativeReplaceTextJob(
            replaceStart,
            replaceCount,
            filteredText,
            state.cursor.start,
            state.cursor.count)

        if (mBatchEditCounter > 0 && forceUpdateImmText) {
            // We need to schedule sending cursor updates to the IMM when the batch is done.
            // Also if we need to force update the text, we can do that here.
            forceImmTextUpdateAtBatchEnd = true
        }

        return true
    }

    override fun commitText(inputText: CharSequence, newCursorPos: Int): Boolean {
        preMethodOp("commitText")
        beginBatchEditInternal()
        val returnVal = commitTextInternal(inputText, newCursorPos, false)
        endBatchEditInternal()
        return returnVal
    }

    private fun replaceTextInternal(
        start: Int,
        end: Int,
        text: CharSequence,
        newCursorPosition: Int,
        textAttribute: TextAttribute?)
    {
        state = state.copy(
            cursor = ArrayRange(start, end - start),
            composingRange = null)
        commitTextInternal(text, newCursorPosition, false)
    }

    override fun replaceText(
        start: Int,
        end: Int,
        text: CharSequence,
        newCursorPosition: Int,
        textAttribute: TextAttribute?
    ): Boolean {
        preMethodOp("replaceText")
        beginBatchEditInternal()
        replaceTextInternal(start, end, text, newCursorPosition, textAttribute)
        endBatchEditInternal()
        return true
    }

    override fun commitText(
        text: CharSequence,
        newCursorPosition: Int,
        textAttribute: TextAttribute?
    ): Boolean {
        preMethodOp("commitText")
        TODO()
        return true
    }

    override fun commitCompletion(text: CompletionInfo): Boolean {
        preMethodOp("commitCompletion")
        TODO()
        return false
    }

    override fun commitCorrection(correctionInfo: CorrectionInfo): Boolean {
        preMethodOp("commitCorrection")

        beginBatchEditInternal()

        replaceTextInternal(
            correctionInfo.offset,
            correctionInfo.offset + correctionInfo.oldText.length,
            correctionInfo.newText,
            1,
            null)

        endBatchEditInternal()
        return false
    }

    private fun setSelectionInternal(start: Int, count: Int) {
        state = state.copy(cursor = ArrayRange(start, count))

        pushToNativeSetSelectionJob(state.cursor.start, state.cursor.count)
    }

    override fun setSelection(start: Int, end: Int): Boolean {
        preMethodOp("setSelection")
        assert(start <= end)

        beginBatchEditInternal()

        setSelectionInternal(start, (end - start).coerceAtLeast(0))

        endBatchEditInternal()

        return true
    }

    override fun performEditorAction(editorAction: Int): Boolean {
        preMethodOp("performEditorAction")
        assert(mBatchEditCounter == 0)
        ToNative.sendEventEndTextInputSession(targetView.dengineApp.nativeBackendDataPtr)
        return true
    }

    override fun performContextMenuAction(id: Int): Boolean {
        preMethodOp("performContextMenuAction")
        TODO()
        return true
    }

    override fun performSpellCheck(): Boolean {
        preMethodOp("performSpellCheck")
        TODO()
        return true
    }

    fun beginBatchEditInternal(): Boolean {
        if (mBatchEditCounter == 0) {
            mPreBatchState = state.deepCopy()
        }
        mBatchEditCounter += 1
        return true
    }

    override fun beginBatchEdit(): Boolean {
        preMethodOp("beginBatchEdit")
        return beginBatchEditInternal()
    }

    fun endBatchEditInternal(): Boolean {
        if (mBatchEditCounter <= 0)
            return false

        mBatchEditCounter -= 1
        if (mBatchEditCounter == 0) {
            mPreBatchState = null

            // TODO: We should probably check if we really had a change at all.
            if (forceImmTextUpdateAtBatchEnd || immMonitorsExtractedText) {
                forceImmTextUpdateAtBatchEnd = false
                dengineDispatchExtractedTextToImm()
            }

            if (immMonitorsCursor) {
                dengineDispatchCursorAnchorInfoToImm()
            }

            // Flush the changes to C++
            DEngineFlushTextJobsToNative()
        }
        return mBatchEditCounter > 0
    }

    override fun endBatchEdit(): Boolean {
        preMethodOp("endBatchEdit")
        return endBatchEditInternal()
    }

    private fun pushNumpadNumber(unicode: Char) {
        beginBatchEditInternal()

        commitTextInternal(unicode.toString(), 1, false)

        endBatchEditInternal()

        /*
        val replaceStart = ic.selStart
        val replaceCount = ic.selCount
        if (mComposingRange != null)
            throw RuntimeException("Not implemented.")

        val outString = inputFilter.filterText(
            editable.toString(),
            replaceStart,
            replaceCount,
            unicode.toString()
        )
        if (outString.isEmpty())
            return
        selStart += 1
        selCount = 0
        pushReplaceTextJob(replaceStart, replaceCount, outString)
         */
    }

    override fun sendKeyEvent(event: KeyEvent): Boolean {
        preMethodOp("sendKeyEvent")
        // It is a hard requirement that we call this function inside sendKeyEvent

        var eventHandled = false
        val isNumpad = inputFilter.isNumpadInput
        val unicode = event.unicodeChar
        val action = event.action
        val keycode = event.keyCode
        if (!eventHandled && isNumpad && action == KeyEvent.ACTION_DOWN && unicode != 0) {
            // It's a valid unicode, push it.
            pushNumpadNumber(unicode.toChar())
            eventHandled = true
        }
        // According to docs, this shouldn't really happen but GBoard sucks and we have to do this
        // dirty hack.
        if (!eventHandled && action == KeyEvent.ACTION_UP && keycode == KeyEvent.KEYCODE_DEL) {
            // If we haven't selected any range, check if we should delete preceding character
            beginBatchEditInternal()
            if (state.cursor.start > 0 && state.cursor.count == 0) {
                state = state.copy(
                    cursor = ArrayRange(state.cursor.start - 1, 1),
                    composingRange = null)
            }
            if (state.cursor.count > 0) {
                commitTextInternal("", 1, forceUpdateImmText = true)
            }
            endBatchEditInternal()

            eventHandled = true
        }

        if (!eventHandled && action == KeyEvent.ACTION_UP && keycode == KeyEvent.KEYCODE_DPAD_LEFT) {
            beginBatchEditInternal()

            if (state.cursor.count > 0) {
                setSelectionInternal(state.cursor.start, 0)
            } else {
                if (state.cursor.start > 0) {
                    setSelectionInternal(state.cursor.start - 1, 0)
                }
            }

            endBatchEditInternal()
        }

        if (!eventHandled && action == KeyEvent.ACTION_UP && keycode == KeyEvent.KEYCODE_DPAD_RIGHT) {
            beginBatchEditInternal()

            if (state.cursor.count > 0) {
                setSelectionInternal(state.cursor.end, 0)
            } else {
                if (state.cursor.start < state.editable.length) {
                    setSelectionInternal(state.cursor.start + 1, 0)
                }
            }

            endBatchEditInternal()
        }

        if (!eventHandled) {
            mInputMethodManager.dispatchKeyEventFromInputMethod(targetView, event)
        }


        // Return true if the input connection is still valid.
        return true
    }

    override fun clearMetaKeyStates(states: Int): Boolean {
        preMethodOp("clearMetaKeyStates")
        // For editor authors, the value is ignored.
        return true
    }

    override fun reportFullscreenMode(enabled: Boolean): Boolean {
        preMethodOp("reportFullscreenMode")
        // For editor authors, the value is ignored.
        return true
    }

    override fun performPrivateCommand(action: String, data: Bundle): Boolean {
        preMethodOp("performPrivateCommand")
        // Return false if we didn't recognize the action and/or didn't do anything with it.
        return false
    }

    override fun takeSnapshot(): TextSnapshot {
        preMethodOp("takeSnapshot")
        // TODO: In the future we might want to support the C++ UI only sending in part of the
        // current text-field, in the case where we are editing a large document.
        // In which case we should pass in her how far away we are from the start of the text.
        val surroundingText = SurroundingText(
            state.editable,
            state.cursor.start,
            state.cursor.end,
            0)
        if (state.composingRange != null) {
            return TextSnapshot(
                surroundingText,
                state.composingRange!!.start,
                state.composingRange!!.end,
                dengineGetCursorCapsMode())
        } else {
            return TextSnapshot(
                surroundingText,
                -1,
                -1,
                dengineGetCursorCapsMode())
        }
    }

    override fun setImeConsumesInput(imeConsumesInput: Boolean): Boolean {
        preMethodOp("setImeConsumesInput")
        // Return value is always ignored for the editor code.
        return true
    }

    // Check if the new incoming state is different from what we already had,
    // and only restart the input connection if necessary
    // Reset pretty much everything and use the data provided.
    fun dengineFromNativeUpdateInputConnection(
        selectionStart: Int,
        selectionCount: Int,
        textData: ByteArray): Boolean
    {
        // TODO: It's possible we might receive events while we are waiting for this job to execute.
        // We might want to have some kind of mutex-locked bool that says whether we should accept events
        // or not.

        val inputString = String(textData, 0, textData.size, Charsets.US_ASCII)
        assert(selectionStart + selectionCount < inputString.length)


        val latch = CountDownLatch(1)
        var jobSuccess = false

        // Run it on the same thread as input connection receives events
        val postSuccess = mDEngineHandler.post lambda@ {
            try {
                // TODO: Check if we had to change anything first, if nothing changed we don't have
                // to restart the input.
                val inputSelection = ArrayRange(selectionStart, selectionCount)
                if (state.editable.toString() == inputString &&
                    state.cursor == inputSelection)
                {
                    // They are the same. Do nothing?
                    jobSuccess = true
                    return@lambda
                }


                TODO()

                /*
                editable.clear()
                editable.append(inputString)
                selStart = selectionStart
                selCount = selectionCount
                mPreBatchSelStart = 0
                mPreBatchSelCount = 0
                mBatchEditCounter = 0
                mComposingRange = null
                replaceTextJobs.clear()

                mInputMethodManager.invalidateInput(targetView)

                jobSuccess = true
                 */
                jobSuccess = true
            } catch (e: Exception) {
                Log.e("DEngineNativeInputConnection", e.toString())
            } finally {
                latch.countDown()
            }
        }

        if (!postSuccess)
            return false

        latch.await()

        if (!jobSuccess)
            return false

        return true
    }
}