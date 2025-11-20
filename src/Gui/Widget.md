# The Widget API

This document describes `DEngine::Gui::Widget`, the single polymorphic interface every UI node implements. It is a per-function reference, plus notes on how to implement a Widget correctly.

The header `include/DEngine/Gui/Widget.hpp` is the source of truth for signatures. This document is the source of truth for behaviour and intent. When they disagree, one of them is a bug, fix it.

Navigation, activation, back and axis input have grown big enough to live in their own document, see Navigation.md. Overall design and the layouting pipeline are in Design.md.

---

## How to implement a Widget correctly

A Widget never reaches up the tree. Everything is handed to you through the event parameters. Don't store a pointer to the parent, and don't rely on some external state being set up before the event runs. If we ever need something that isn't in the parameters, we should change the API to include that info.

Keep the read-only events read-only. Measuring, layouting, rendering and navigation must not change the tree, and must not mutate the Widget in a way that affects the result. We want these to be safe to run without side-effects, and some of them we want to run off the main thread. If you need to remember something you computed, put it in the Widget's RectCollection custom data, don't stash it in a field you mutate mid-traversal.

If you want to change the tree, or do anything else with side-effects, don't do it during the event. Queue it on the deferred job queue and it runs once the dispatch is done.

When you are a Layout, you own your children. You measure them, you assign them their rects, and you forward events to them. Even children that end up off-screen still need a rect assigned, because the UI anchor may later translate them into view. When an event is meant to keep propagating, don't swallow it early unless the rules for that specific event say you can.

If your Widget is just a plain focusable button, use `Widget_DefaultNavigationHandler` and you're done, see Navigation.md. Only write your own navigation handling if your Widget arranges children in a way the default can't express.

## Rules every event obeys

**Dispatch starts at the root.** Unless a function's reference below says otherwise, an event is invoked on the root Widget and the Widget (usually a Layout) is responsible for forwarding it to its children.

**Consumption.** An event is *consumed* when a Widget did something meaningful with it: occluded the point, triggered logic, or started gesture tracking. A Widget consuming an event signals an intent that other Widgets in the UI tree should not do anything meaningful with that event. Several events still need to be dispatched to the rest of the UI tree even if it was consumed in the middle of dispatch. This is because some Widgets may expect the event to happen as part of gesture tracking, or they need to know the user did something outside some Widget that considers itself active.

**Point events never stop early.** `CursorPress`, `TouchMove`, `TouchPress` and friends must be delivered *everywhere*, even after something consumes them. Widgets rely on seeing an *already-consumed* event so they can **cancel** their own in-progress gesture tracking. The classic example: a `ScrollArea` does not consume the initial `TouchPress`, so its child may start tracking a tap; once the finger moves far enough the `ScrollArea` consumes the `TouchMove` and starts scrolling; the child then sees a consumed `TouchMove` and cancels its tap. Point events are also not guaranteed to land inside any Widget's rect.

**Immutability where it matters.** Rendering and navigation must be immutable and thread-safe operations on the tree. Effectful work is either done in an explicitly mutable event (`Activate`, `AxisInput`) or deferred.

**Effects that change the tree are deferred.** A Widget cannot restructure the tree while the tree is being iterated. Spawning/removing layers, or any structural change, is queued via the `WidgetEvent_DeferredJobQueue` and run after dispatch completes.

## Rendering

### `WidgetEvent_Render` (const)

Emits abstract draw commands through the `DrawEngine` in `Render_Params`. The GUI never knows how it is actually rendered (currently Vulkan, but that is an external detail). `Render_Params::focusedWidget` identifies the Widget currently in navigation focus so a Widget can draw a focus highlight.

Rendering is immutable: it must not change tree state.

## Pointer input

### `CursorMove` → `bool`

Returns `true` if the Widget **occludes** the cursor point for this event — meaning Widgets behind it in Z should not react (no hover/highlight underneath).

The returned value feeds the `occluded` parameter of every subsequent Widget in the dispatch. Layout Widgets must therefore keep an OR-accumulated copy of `occluded` and pass the running value down: once `occluded` is `true`, later return values no longer matter. Transparent overlays return `false`.

### `CursorPress2` → `bool`

Returns `true` if the press was consumed. If the event was **already** consumed before reaching this Widget, the return value can be ignored. You may not end dispatch early — pass all presses everywhere (see the point-event rule above).

### `WidgetEvent_TouchPress` → `TouchEventConsumption`

Returns `{ bool consumed, bool claimDragPriority }`. `claimDragPriority` lets a Widget signal that it wants ownership of the drag gesture for this touch id.

### `WidgetEvent_TouchMove` → `TouchEventConsumption`

If a Widget receives a **consumed** (currently also signalled via `occluded`) move for a finger id it is tracking, it must cancel that in-progress gesture. This is the mechanism that lets a `ScrollArea` "steal" a gesture from its child mid-drag.

> Current caveat: `occluded` is being overloaded to mean "consumed" here. This is expected to change.

### `CursorExit`

The cursor left the window/region. Clear hover state.

## Text input

Used mainly by text-editing Widgets. Text is ASCII (or extended ASCII).

- **`WidgetEvent_TextInput`** — replace the substring `[start, start+count)` with `newText` (which may be null/empty to delete). After replacement, assume the selection collapses to the end of the inserted text.
- **`WidgetEvent_TextSelection`** — set the selection to `[start, start+count)`.
- **`WidgetEvent_EndTextInputSession`** — the session ended.

Currently these are broadcast to the whole tree and the Widget that owns the active session acts on them; Layouts just propagate. In future they may be delivered directly to the target Widget.

See also `Widget_TextInputSession` below.

## Accessibility

### `AccessibilityTest` (const)

Walks the tree and pushes accessibility elements (`{ rect, text range, isClickable }`) via an `AccessibilityInfoPusher`, building a flat description of the UI for assistive technology. Immutable.

## Host-side interfaces handed to Widgets

These are thin, short-lived interfaces the host implements and passes into event params. Unless noted, they are valid **only for the duration of one event dispatch** — never store a long-lived reference. (The one exception is `AxisInputSource`, which is alive across frames; it lives in Navigation.md.)

### `WidgetEvent_WindowHandler`

Lets a Widget ask the host to manipulate layers and text input mid-dispatch. Recreated per layer; do not store.

- `SetFrontmostLayer(rootWidget, relativePos, preferredExtent, focusItem) → bool` — push a new layer (dialog / child window) in front. `focusItem`, if given, must be a Widget inside `rootWidget`'s subtree. `preferredExtent` must not be `(0,0)`; the real extent may differ (e.g. a child window hitting the display edge).
- `QueueRemoveCurrentLayer()` — queue removal of the current layer. It must only be deleted **after** all dispatching for all layers is done, otherwise iteration would run over a deleted layer. Calling it twice in one dispatch, or on the rearmost layer, is a bug.
- `TakeTextInputConnection(widget, filter, currentText, selStart, selCount) → Box<Widget_TextInputSession>` — open a platform text-input session (soft keyboard / IME). The returned session is **long-lived** even though the handler is not.

### `Widget_TextInputSession`

Represents ownership of the active platform text-input session. **Destroying it cancels the session** from the platform's perspective. Stored by the owning Widget as `Std::Opt<Std::Box<Widget_TextInputSession>>`; destroyed with the Widget, which implicitly cancels the session. The platform data backing it must outlive it (guaranteed while the platform owns the tree).

### `WidgetEvent_DeferredJobQueue`

Posts a movable callable to run **after** the current dispatch completes, so the tree can be modified without being mutated mid-iteration. Each job runs exactly once. Having many deferred jobs is considered code-smell — prefer to design the effect out.
