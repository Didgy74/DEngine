package didgy.dengine

import android.graphics.Rect
import android.os.Bundle
import android.util.Log
import android.view.View
import android.view.accessibility.AccessibilityNodeInfo
import android.view.accessibility.AccessibilityNodeInfo.AccessibilityAction
import android.view.accessibility.AccessibilityNodeProvider

data class WidgetId_VirtualAccessChild_Pair(
    val widgetId: Long,
    val virtualAccessChildId: Int)
data class VirtualAccessibilityData(
    val focusedId: Int?,
    val accessFocusedId: Int?,
    val children: Map<WidgetId_VirtualAccessChild_Pair, VirtualAccessibilityElement>,)
{
    fun getChildEntryFromVirtualViewId(virtualViewId: Int) =
        children.asSequence().find { it.key.virtualAccessChildId == virtualViewId }
}

data class VirtualAccessibilityElement(
    val posX: Int,
    val posY: Int,
    val width: Int,
    val height: Int,
    val clickable: Boolean,
    val text: String,)
{
    fun buildRect() = Rect(
        posX,
        posY,
        posX + width,
        posY + height)
}

class CustomAccessibilityNodeProvider(val view: NativeView):  AccessibilityNodeProvider()  {
    val owningActivity get() = view.dengineActivity
    val accessibilityData get() = view.dengineActivity.accessibilityData
    val children get() = accessibilityData.children

    override fun createAccessibilityNodeInfo(virtualViewId: Int): AccessibilityNodeInfo? {
        if (virtualViewId == HOST_VIEW_ID)
            return nodeInfoForRoot(view)

        return nodeInfoForChild(view, virtualViewId)
    }

    override fun performAction(virtualViewId: Int, action: Int, arguments: Bundle?): Boolean {
        if (virtualViewId == HOST_VIEW_ID) {
            Log.d("Didgy", "Received performAction on host-view.")
            return false
        }

        /*
        if (!children.containsKey(virtualViewId)) {
            Log.d("Didgy", "Received performAction with virtualViewId not set to host or a valid virtual child")
            return false
        }

        when (action) {
            AccessibilityNodeInfo.ACTION_ACCESSIBILITY_FOCUS -> {
                Log.d("DEngine", "Action accessibility focus id: $virtualViewId")
                if (virtualViewId != accessibilityData.accessFocusedId) {
                    // TODO: This should be done through UI event propagation in C++
                    view.postDelayed({
                        view.activity.accessibilityData = view.activity.accessibilityData
                            .copy(accessFocusedId = virtualViewId)
                        val event = AccessibilityEvent()
                        event.setSource(view, virtualViewId)
                        event.eventType = AccessibilityEvent.TYPE_VIEW_ACCESSIBILITY_FOCUSED
                        val sendSuccess = view.parentForAccessibility.requestSendAccessibilityEvent(view, event)
                        if (!sendSuccess)
                            Log.d("DEngine", "Couldn't send accessibility event")
                        view.invalidate()
                    }, 0)

                    return true
                }
            }
            AccessibilityNodeInfo.ACTION_CLEAR_ACCESSIBILITY_FOCUS -> {
                Log.d("DEngine", "Action clear accessibility focus id: $virtualViewId")
                if (virtualViewId != accessibilityData.accessFocusedId) {
                    Log.d("DEngine", "Received clear access focus action on virtualViewId $virtualViewId that is not currently access focused.")
                }
                return true
            }

            AccessibilityNodeInfo.ACTION_FOCUS -> {
                Log.d("DEngine", "Action  focus id: $virtualViewId")
                // TODO: This should be done through UI event propagation in C++
                view.postDelayed({
                    view.activity.accessibilityData = view.activity.accessibilityData
                        .copy(focusedId = virtualViewId)
                    val event = AccessibilityEvent()
                    event.setSource(view, virtualViewId)
                    event.eventType = AccessibilityEvent.TYPE_VIEW_FOCUSED
                    val sendSuccess = view.parentForAccessibility.requestSendAccessibilityEvent(view, event)
                    if (!sendSuccess)
                        Log.d("DEngine", "Couldn't send accessibility event")
                    view.invalidate()
                }, 0)
                return true
            }

            AccessibilityNodeInfo.ACTION_CLEAR_FOCUS -> {
                Log.d("DEngine", "Action clear accessibility focus id: $virtualViewId")
                if (virtualViewId != accessibilityData.focusedId) {
                    Log.d("DEngine", "Received clear focus action on virtualViewId $virtualViewId that is not currently focused.")
                }
                return true
            }

            AccessibilityNodeInfo.ACTION_CLICK -> {
                Log.d("DEngine", "Action click id: $virtualViewId")
                return true
            }
        }
         */

        return super.performAction(virtualViewId, action, arguments)
    }

    override fun findFocus(focus: Int): AccessibilityNodeInfo? {
        if (focus == AccessibilityNodeInfo.FOCUS_ACCESSIBILITY) {
            if (accessibilityData.accessFocusedId == null)
                return null
            else
                return nodeInfoForChild(view, accessibilityData.accessFocusedId!!)
        }
        /*
        if (focus == AccessibilityNodeInfo.FOCUS_INPUT) {
            if (view.mInputFocused == CustomView.NO_FOCUS) {
                return null
            }
            return nodeInfoForChild(view, view.mInputFocused)
        }
         */
        return null
    }
}

fun nodeInfoForRoot(view: NativeView): AccessibilityNodeInfo {
    val children = view.dengineActivity.accessibilityData.children

    val out = AccessibilityNodeInfo(view)
    out.setParent(view.parentForAccessibility as View)
    out.isEnabled = true
    // Root never has any visuals
    out.isVisibleToUser = false

    for (key in children.keys) {
        out.addChild(view, key.virtualAccessChildId)
    }

    out.className = NativeView::class.qualifiedName

    run {
        val temp = IntArray(2)
        //view.getLocationOnScreen(temp)
        out.setBoundsInScreen(Rect(
            temp[0],
            temp[1],
            temp[0] + view.width,
            temp[1] + view.height
        ))
    }
    run {
        val temp = IntArray(2)
        //view.getLocationInWindow(temp)
        out.setBoundsInWindow(Rect(
            temp[0],
            temp[1],
            temp[0] + view.width,
            temp[1] + view.height
        ))
    }

    return out
}

// Returns true on success
fun populateNodeInfoForChild(root: NativeView, virtualViewId: Int, out: AccessibilityNodeInfo): Boolean {

    // First check if this child is alive and attached
    val accessibilityData = root.dengineActivity.accessibilityData

    val temp = accessibilityData.getChildEntryFromVirtualViewId(virtualViewId)
    if (temp == null)
        return false

    val child = temp.value

    val isAccessFocused = virtualViewId == accessibilityData.accessFocusedId
    val isFocused = virtualViewId == accessibilityData.focusedId

    out.setSource(root, virtualViewId)
    out.setParent(root)
    //out.uniqueId =  root.id.toString() + ":" + virtualViewId.toString()
    out.className = NativeView::class.qualifiedName + "Child"

    if (child.text.isNotEmpty())
        out.text = child.text
    else
        out.text = null

    out.isVisibleToUser = true
    out.isEnabled = true
    out.isImportantForAccessibility = true
    //out.isScreenReaderFocusable = true

    run {
        val temp = IntArray(2)
        //root.getLocationOnScreen(temp)
        val childRect = child.buildRect().apply {
            offset(temp[0], temp[1])
        }
        out.setBoundsInScreen(childRect)
    }
    run {
        val temp = IntArray(2)
        //root.getLocationInWindow(temp)
        val childRect = child.buildRect().apply {
            offset(temp[0], temp[1])
        }
        out.setBoundsInWindow(childRect)
    }

    //out.isFocusable = true
    out.isFocused = isFocused
    if (out.isFocusable) {
        if (isFocused) {
            out.addAction(AccessibilityAction.ACTION_CLEAR_FOCUS)
        } else {
            out.addAction(AccessibilityAction.ACTION_FOCUS)
        }
    }

    out.isAccessibilityFocused = isAccessFocused
    if (isAccessFocused) {
        out.addAction(AccessibilityAction.ACTION_CLEAR_ACCESSIBILITY_FOCUS)
    } else {
        // TODO: Should probably add boolean to set whether the item is
        // access-focusable
        out.addAction(AccessibilityAction.ACTION_ACCESSIBILITY_FOCUS)
    }

    //out.isEditable = child.isEditable

    val hasActionId = { id: Int ->
        out.actionList.map { it.id }.contains(id)
    }

    /*
    if (child.isEditable) {
        out.inputType = child.inputType.androidTextInputType
        out.isTextSelectable = true
        out.isContextClickable = true
        out.addAction(AccessibilityAction(AccessibilityNodeInfo.ACTION_CLICK, "Edit ${out.contentDescription}"))
        out.addAction(AccessibilityAction.ACTION_COPY)
        out.addAction(AccessibilityAction.ACTION_SET_SELECTION)
        out.addAction(AccessibilityAction.ACTION_CLEAR_SELECTION)
    }
    */

    out.isClickable = child.clickable
    if (out.isClickable && !hasActionId(AccessibilityNodeInfo.ACTION_CLICK)) {
        out.addAction(AccessibilityAction(AccessibilityNodeInfo.ACTION_CLICK, out.contentDescription))
    }

    return true
}

fun nodeInfoForChild(root: NativeView, virtualViewId: Int): AccessibilityNodeInfo? {
    val out = AccessibilityNodeInfo()
    val success = populateNodeInfoForChild(root, virtualViewId, out)
    if (!success)
        return null
    return out
}