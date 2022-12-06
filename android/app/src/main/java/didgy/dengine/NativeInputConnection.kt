package didgy.dengine

import android.os.Bundle
import android.os.Handler
import android.os.LocaleList
import android.text.SpannableStringBuilder
import android.text.TextUtils
import android.util.Log
import android.view.KeyEvent
import android.view.inputmethod.*
import java.util.*

// Returns true if we did anything.
typealias NativeInputConnection_Job = (ic: NativeInputConnection) -> Boolean

class NativeInputConnection(
    var targetView: NativeView,
    val beginInfo: BeginInputSessionInfo) : InputConnection
{
    val activity get() = targetView.activity
    val editable = SpannableStringBuilder(beginInfo.text)
    val inputFilter = beginInfo.inputFilter
    private var selStart = beginInfo.selStart
    private var selCount = beginInfo.selCount
    private val selEnd get() = selStart + selCount
    var mPreBatchSelStart = 0
    var mPreBatchSelCount = 0
    var mBatchEditCounter = 0

    fun setEditorInfo(outAttrs: EditorInfo) {
        outAttrs.initialSelStart = beginInfo.selStart
        outAttrs.initialSelEnd = beginInfo.selEnd
        outAttrs.imeOptions = EditorInfo.IME_ACTION_DONE
        outAttrs.inputType = beginInfo.inputFilter.toAndroidInputType()
        outAttrs.hintLocales = LocaleList(Locale.ENGLISH)
    }

    data class ArrayRange(
        var start: Int,
        var count: Int)

    var mComposingRange: ArrayRange? = null

    data class ReplaceTextJob(
        var start: Int,
        var count: Int,
        var text: CharSequence) {
        val end get() = start + count
    }

    private var replaceTextJobs = mutableListOf<ReplaceTextJob>()
    fun flushReplaceTextJobs() {
        for (item in replaceTextJobs) {
            editable.replace(
                item.start,
                item.end,
                item.text)
        }
        val nativeJobs = replaceTextJobs.map {
            DEngineActivity.NativeTextInputJob(
                it.start,
                it.count,
                it.text.toString())
        }
        if (replaceTextJobs.size > 0) {
            Log.d("DEngine", "Current text: $editable")
        }
        replaceTextJobs.clear()
        activity.nativeOnTextInput(nativeJobs)
    }

    fun pushReplaceTextJob(start: Int, count: Int, text: CharSequence) {
        replaceTextJobs.add(ReplaceTextJob(start, count, text))
        if (mBatchEditCounter == 0) {
            flushReplaceTextJobs()
        }
    }

    private fun signalNewSelectionToIMM() {
        if (mBatchEditCounter == 0) {
            targetView.mIMM.updateSelection(
                targetView,
                selStart,
                selEnd,
                -1,
                -1)
        }
    }

    // This is meant to be called after a replace operation.
    // For the second argument you will want to pass
    // `replaceStart + text.length() - 1
    private fun setRelativeCursorPos(newCursorPos: Int, replaceEnd: Int) {
        // Update the cursor
        // If >0 the pos is relative to text.length() - 1
        // Otherwise it is relative to text beginning.
        if (newCursorPos > 0)
            selStart = replaceEnd + newCursorPos
        else
            selStart += newCursorPos
        selCount = 0
    }

    private fun doStuffAndUpdateSel(jobs: List<NativeInputConnection_Job>) {
        var didAnything = false
        for (item in jobs) {
            val result = item(this)
            didAnything = didAnything || result
        }
        if (mBatchEditCounter == 0 && didAnything) {
            flushReplaceTextJobs()
            //signalNewSelectionToIMM();
        }
    }

    private fun doStuffAndUpdateSel(job: NativeInputConnection_Job) = doStuffAndUpdateSel(listOf(job))

    override fun getTextBeforeCursor(n: Int, flags: Int): CharSequence? {
        val start = 0.coerceAtLeast(selStart - n)
        return editable.subSequence(start, selStart)
    }

    override fun getTextAfterCursor(n: Int, flags: Int): CharSequence? {
        //assert(flags != GET_TEXT_WITH_STYLES);
        val end = editable.length.coerceAtMost(selEnd + n)
        return editable.subSequence(selEnd, end)
    }

    override fun getSelectedText(flags: Int): CharSequence {
        return editable.subSequence(selStart, selEnd)
    }

    override fun getCursorCapsMode(reqModes: Int): Int {
        return TextUtils.getCapsMode(editable, selStart, reqModes)
    }

    override fun requestCursorUpdates(cursorUpdateMode: Int): Boolean {
        val builder = CursorAnchorInfo.Builder()
        builder.setSelectionRange(selStart, selEnd)
        val done = builder.build()
        targetView.mIMM.updateCursorAnchorInfo(targetView, done)
        return true
    }

    override fun getHandler(): Handler? {
        return null
    }

    override fun closeConnection() {}
    override fun commitContent(
        inputContentInfo: InputContentInfo,
        flags: Int,
        opts: Bundle?
    ): Boolean {
        return false
    }

    override fun getExtractedText(request: ExtractedTextRequest, flags: Int): ExtractedText {
        val returnVal = ExtractedText()
        returnVal.text = editable.toString()
        returnVal.selectionStart = selStart
        returnVal.selectionEnd = selEnd
        return returnVal
    }

    override fun deleteSurroundingText(beforeLength: Int, afterLength: Int): Boolean {
        val jobs = mutableListOf<NativeInputConnection_Job>()
        if (beforeLength > 0) {
            jobs.add lambda@{ ic ->
                val replaceStart = ic.selStart - beforeLength
                // There is potentially functionality we need to fix here.
                if (mComposingRange != null) {
                    mComposingRange!!.start -= beforeLength
                }
                // Then move the cursor
                selStart -= beforeLength
                pushReplaceTextJob(replaceStart, beforeLength, "")
                return@lambda true
            }
        }
        if (afterLength > 0) {
            throw RuntimeException("Not implemented.")
        }
        if (jobs.size > 0) {
            doStuffAndUpdateSel(jobs)
        }
        return true
    }

    override fun deleteSurroundingTextInCodePoints(beforeLength: Int, afterLength: Int): Boolean {
        throw RuntimeException("Not implemented")
    }

    override fun setComposingText(inputText: CharSequence, newCursorPos: Int): Boolean {
        doStuffAndUpdateSel lambda@{
            var replaceStart = selStart
            var replaceCount = selCount
            if (mComposingRange != null) {
                replaceStart = mComposingRange!!.start
                replaceCount = mComposingRange!!.count
            }
            val filteredText = inputFilter.filterText(
                editable.toString(),
                replaceStart,
                replaceCount,
                inputText
            )
            if (filteredText.isEmpty() && inputText.isNotEmpty()) return@lambda false
            mComposingRange = if (filteredText.isEmpty()) {
                null
            } else {
                ArrayRange(
                    replaceStart,
                    filteredText.length
                )
            }
            setRelativeCursorPos(newCursorPos, replaceStart + filteredText.length - 1)
            // This function might be called with no text to replace,
            // and no text to insert, just to remove our composing span.
            // So if that happens, we don't want to submit it.
            if (replaceCount != 0 || filteredText.isNotEmpty()) pushReplaceTextJob(
                replaceStart,
                replaceCount,
                filteredText
            )
            true
        }
        return true
    }

    override fun setComposingRegion(start: Int, end: Int): Boolean {
        mComposingRange = if (start - end == 0) {
            null
        } else {
            ArrayRange(start, end - start)
        }
        return true
    }

    override fun finishComposingText(): Boolean {
        if (mComposingRange != null)
            throw RuntimeException("Error?")
        mComposingRange = null
        return true
    }

    override fun commitText(inputText: CharSequence, newCursorPos: Int): Boolean {
        doStuffAndUpdateSel label@{
            var replaceStart = selStart
            var replaceCount = selCount
            if (mComposingRange != null) {
                replaceStart = mComposingRange!!.start
                replaceCount = mComposingRange!!.count
            }
            val filteredText = inputFilter.filterText(
                editable.toString(),
                replaceStart,
                replaceCount,
                inputText
            )
            if (filteredText.isEmpty() && inputText.isNotEmpty()) return@label false // We didn't do anything so return false?
            mComposingRange = null
            setRelativeCursorPos(newCursorPos, replaceStart + filteredText.length - 1)
            pushReplaceTextJob(replaceStart, replaceCount, filteredText)
            true
        }
        return true
    }

    override fun commitCompletion(text: CompletionInfo): Boolean {
        return false
    }

    override fun commitCorrection(correctionInfo: CorrectionInfo): Boolean {
        return false
    }

    override fun setSelection(start: Int, end: Int): Boolean {
        if (start != selStart || end != selEnd) {
            selStart = start
            selCount = end - start
            signalNewSelectionToIMM()
        }
        return true
    }

    override fun performEditorAction(editorAction: Int): Boolean {
        activity.nativeSendEventEndTextInputSession()
        return true
    }

    override fun performContextMenuAction(id: Int): Boolean {
        return false
    }

    override fun beginBatchEdit(): Boolean {
        if (mBatchEditCounter == 0) {
            mPreBatchSelStart = selStart
            mPreBatchSelCount = selCount
        }
        mBatchEditCounter += 1
        return true
    }

    override fun endBatchEdit(): Boolean {
        if (mBatchEditCounter <= 0) return false
        mBatchEditCounter -= 1
        if (mBatchEditCounter == 0) {
            // Run the batch edits
            flushReplaceTextJobs()
            if (mPreBatchSelStart != selStart || mPreBatchSelCount != selCount) {
                signalNewSelectionToIMM()
            }
        }
        return mBatchEditCounter > 0
    }

    private fun pushNumpadNumber(unicode: Char) {
        doStuffAndUpdateSel lambda@ { ic: NativeInputConnection ->
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
                return@lambda false
            selStart += 1
            selCount = 0
            pushReplaceTextJob(replaceStart, replaceCount, outString)
            true
        }
    }

    override fun sendKeyEvent(event: KeyEvent): Boolean {
        val isNumpad = inputFilter.isNumpadInput
        val unicode = event.unicodeChar
        val action = event.action
        val keycode = event.keyCode
        if (isNumpad && action == KeyEvent.ACTION_DOWN && unicode != 0) {
            // It's a valid unicode, push it.
            pushNumpadNumber(unicode.toChar())
            return true
        }
        if (action == KeyEvent.ACTION_DOWN && keycode == KeyEvent.KEYCODE_DEL) {
            // It's a valid unicode, push it.
            doStuffAndUpdateSel { ic: NativeInputConnection ->
                var replaceStart = ic.selStart
                var replaceCount = ic.selCount
                if (replaceCount == 0 && replaceStart > 0) {
                    // Delete one character behind the cursor
                    replaceStart -= 1
                    replaceCount = 1
                    selStart -= 1
                }
                ic.selCount = 0
                if (replaceCount != 0) {
                    pushReplaceTextJob(replaceStart, replaceCount, "")
                }
                true
            }
            return true
        }
        return true
    }

    override fun clearMetaKeyStates(states: Int): Boolean {
        return false
    }

    override fun reportFullscreenMode(enabled: Boolean): Boolean {
        return false
    }

    override fun performPrivateCommand(action: String, data: Bundle): Boolean {
        return false
    }
}