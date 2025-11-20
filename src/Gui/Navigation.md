# UI Navigation

This document covers UI navigation and axis input.

The UI solution is designed to have good support for navigating the UI with gamepads and keyboards, without having to rely on touch and mouse. The point is to allow having a "focused Widget" and then switch the focus around in the UI.

> Note! All of UI navigation is currently WIP and we are actively working on figuring it out.

# Goals for UI navigation

The goal is to make it quite good and expansive. Specifically, the behaviors we want to support:

- When in layouts, we want to be able to use joystick, or keyboard arrows, or tab/shift+tab in order to switch between focusable Widgets.
- Switching focusable Widgets should ideally be intuitive, we move towards the one we point at. Ideally it should be possible to navigate through all items on screen this way. Some Layouts are exempt where it makes sense. For instance, it's not quite feasible to make this work perfectly with DockArea.
- Modify sliders, using both stepped and smooth motion.
  - TODO: We need to determine whether adjusting a slider to it's end and beyond should result in a navigation. I.e, with a stepped horizontal slider, does moving it all the way to the end and then attempting to do another step, mean we make the right neighboring Widget in focus?
  - TODO: I think it should be possible to use left joystick to adjust the slider and not have it change focus, but maybe we need to activate the slider first, so that it has some kind of "input focus"? If so, does such input focus require another event?
- Inside a ScrollArea, the regular focus-navigation stick should still move focus between the children, and whenever focus lands on a child, the ScrollArea should scroll it into view automatically.
- The ScrollArea should also be smoothly scrollable — either by re-using the left stick, or by a separate stick such as the right stick, or stepped with the D-pad.
  - A ScrollArea can be configured to only respond in steps, even when driven by an analog stick. Note that we don't have to implement this behavior right away, but the Widget API design must enable it
  - TODO: Ordinarily you want to use the joystick and it scrolls smoothly. It doesn't care if the joystick crosses a threshold where you might want to consider doing a "step". But you DO if you're using a D-pad, you want to use steps if it's a D-pad. So maybe distinguish if the axis source is a "binary" type? Where we give a hint that the source of this axis can never be in between 0-1. 
- A slider should be able to "steal" the input once activated, so you can either drag it smoothly or step it with the D-pad, optionally snapping to a fixed number of stops.
  - TODO: What event can be used to drive this behavior once it has been activated?
  - I think the "back" event should be used to go out of this slider-focused mode
- We need to support "repeat" events for stepped motion. So if we hold a D-pad on a Slider, it may have a small delay then it starts repeating. 
- If an axis event fires and there is no focus Widget, it is acceptable to do nothing (Probably?).
- ScrollArea is focusable when it has any focusable children, or the ScrollArea has content big enugh to warrant an enabled scrollbar.

Things I think we need to do:

- Some kind of axis event in the Widget API
- An axis event should be fired as often as possible (i.e every frame) if the axis is outside the deadzone
- Settings such as deadzone, step-trigger-threshold, repeat-delay and repeat-frequency should all be driven from UI host. Preferably the Widgets never know about this. These values should preferably be grabbed from the system if possible.
- We will need to pass deltaTime into the event so that the Widget knows how far to adjust when handling continuous motion.
- Attached to the event, we should likely have a `i32 getSteps()` method, that tells us if the axis crossed a step-trigger threshold this frame.
  - Should it be a int or bool? We need to make sure we catch edge-cases where we don't lose inputs even if the UI hangs
  - Whether the event really gets interpreted as a step is based on the Widget we are currently operating on, it should only ever be a hint.

# Current state

Here are the primary ways in which we can navigate:

- Directional navigation
- Forward/back navigation
- Axis navigation input

In addition, we have Activate that allow us to trigger whatever we are focusing. We also have Back events that allow us to either close the app, or i.e go out of a dialog.

*Directional navigation* is a type of event that takes any unit 2D vector, tries to project it onto the UI plane and then tries to move the focus in that direction.

Navigation is done recursively. We always try to dispatch the navigation as deep into the UI tree as possible. If a Layout type consumes the navigation event, we can early out. However, often we encounter situations where a Layout would like to handle the event, but the direction hits the boundary of the Layout. This means a parent has to handle the navigation instead. A Layout may signal that it has found the focus Widget but hit a boundary when trying to handle the navigation event, it can then propagate this information up the call stack. In many cases, it is sufficient to check if the input vector falls within a specific quadrant or octant, and then apply the behavior accordingly. Take a horizontal StackLayout as an example. If the input vector falls within the right quadrant, then we know we need to try navigate to any of the StackLayouts children to the right.

Navigation can only ever end in focusing a Widget that is navigation-focusable. When looking for a best-fit for newly focused item, we should iterate the best-fits until we find one that is focusable. This means it's often not enough to just select a neighboring Widget, but rather gather up all candidates, and scoring them by proximity + angle compared to input vector, and then choosing the best one. Angle should be scored heavier than proximity.

*Forward/back navigation*: Forward and not forward events mean we are not directional. These are intended to map to keys such as tab and shift+tab. Forward and not forward can mean different things based on the Layout in question, but in general, forward should mean right, down or in negative Z direction out of the screen, towards the viewer.

For Back events, we need to hold a FIFO stack of focusable items, such that we may know which one to fallback to. This event usually maps to the Android back gesture, the Android back button, keyboard escape button, or gamepad "B" button.

TODO: We need better terminology to separate the Back event and the "forward and not forward".

## `WidgetEvent_Navigation` (immutable, thread-safe)

Finds the next Widget to focus. **Navigation must be an immutable, thread-safe operation on the tree** — it only reports *which* Widget should become focused; it does not act. (It does, however, return a mutable `Widget*` for the new focus, so `Activate` can later be called on it — this is the one concession to mutability, and the reason for the awkward split between Navigation and Activate.)

> TODO: Figure out how we can make this have nicer constness in the future

How to implement this function:

Conceptually, the goal is to receive an input 2D vector direction, place the input vector such that it has origin in the currently focused Widget. From then on, you follow the direction and try to find the best alternative to navigate to. In the current implementation, this is usually a matter of gathering all candidates that are focusable, and ranking them based on angle between the input direction and direction to the candidate.

Navigation events should be dispatched to the innermost possible child first, to give it a chance to consume it. Commonly, navigation happens in these steps:

If we have a focus Widget already at the time:
-  Search the tree, depth first, until we find the focus Widget
- The focus Widget may choose to consume it, in which case we can early return the entire event dispatch
- If the focus Widget does not consume it, it should mark the event-state as "HitBoundary" and fill in the preferred focus center point of the Widget.
- The parent Layout will then try to handle the event and consume it. If it can't keep propagating up the HitBoundary state

In some cases it is easier to determine what should be considered the next Widget in a direction. For instance, a horizontal StackLayout may use quadrants or octants. So if the input vector is in the direction of right, it might mean it just needs to look at the next Widget in the child list.

If there is no current focus, it often means we want to treat the input direction as if it's coming outside the UI plane and pointing in. So a input vector pointing left, would turn the far right Widget in focus.

Input (`Widget_NavigationParams`):

- `directional` — `true` for vector navigation, `false` for forward/back navigation.
- `angle` — the navigation direction when directional. `0` points up and increases counter-clockwise, `1` being a full revolution. Helpers `IsDirectionQuadrant{Up,Down,Left,Right}()` classify it.
- `forward` — direction for non-directional navigation.
- `currentFocusOpt` — the currently focused Widget, if any.

Output is written into `Widget_NavigationState`, a small state machine with three states:

- **`NotConsumed`** — still searching. With no current focus, the first navigable Widget consumes. With a current focus, we are still looking for it in the tree.
- **`HitBoundary`** — the focused Widget was found but cannot itself consume the move; the parent must cross the local boundary and look for a new focus. Carries `sourceRectHitBoundary`, the rect of the item that hit the boundary — a child's output rect becomes the next sibling's origin rect.
- **`Consumed`** — a new focus Widget was found; stop searching.

Mutation only happens through `Consume(widget, secondaryIndex)` and `HitBoundary(rect)`, which assert their valid predecessor states.

### `Widget_DefaultNavigationHandler`

The trivial implementation for a Widget that behaves as a plain focusable button: if nothing is focused it consumes; if it *is* the current focus it reports `HitBoundary`; on an incoming `HitBoundary` from a sibling it consumes. Use this unless your Widget needs custom internal navigation.

## `WidgetEvent_Activate` → `bool`

Invokes the focused Widget's action. **Called directly on a single Widget**, not dispatched from the root. This is a mutable/effectful event. Returns `true` if it did something.

## `WidgetEvent_Back` → `bool`

A semantic Back / Cancel request (e.g. Android's back gesture, or Escape). Effectful, and dispatched **frontmost layer → rear**. A Widget handles it by performing a meaningful cancel (such as closing a popup layer without accepting its focused item). A `true` return means consumed — on Android the platform back action should then not continue.

## Continuous axis input (`WidgetEvent_AxisInput`)

Continuous / analog navigation: joystick, D-pad or arrow-key motion. This is the newest event and differs from every other one in several ways, so it gets its own section.

### What a control is: `AxisId`

Input is expressed as an **opaque logical control**, `AxisId`. The GUI never interprets these. The **host** maps physical inputs (analog sticks, D-pad, arrow keys, a wheel…) onto `AxisId`s, and the **application** configures each Widget with the id it should respond to.

A control is 2D by nature; a purely 1D control (arrow-key pair, wheel) simply leaves one component at rest. The default vocabulary is `Primary` (main directional control) and `Secondary` (auxiliary); applications may remap Widgets to other ids or define their own values past `COUNT`.

This indirection is what keeps the GUI honest about its rule that *it may never know what platform it runs on*. Nothing below the host knows a "stick" exists.

### How it is dispatched

- **Per frame.** Unlike every other event, this is dispatched every frame that a control is active (outside its deadzone, or a discrete source held for auto-repeat). It is the **only per-frame event** in the GUI — the first place the UI gains a notion of frames.
- **Mutable.** It moves scroll positions and slider values, so it is dispatched on the main thread like `Activate`, and is **not** part of the immutable navigation pass.
- **Broadcast, like `CursorPress2`.** It goes to the whole tree; do not end dispatch early. A Widget selects itself from the passed-down focus data: a leaf Widget checks `focusedWidget.widget == this`; a container learns focus is inside its subtree from the result state.
- **One control per pass.** A single dispatch handles exactly one `AxisId`. When several controls are active in a frame, the host runs one pass per control, each with its own fresh result state. This keeps the state scalar and unambiguous — a `Slider` taking `Primary` and a `ScrollArea` taking `Secondary` in the same frame are just two passes.

### Where time lives

Time never enters a Widget. The host owns all elapsed-time state inside the `AxisInputSource` and advances it once per frame. Widgets only ever read already-integrated results (`GetAnalogDelta`) or discrete step counts (`TakeSteps`) — never a `dt` or a clock. The source itself is described at the end of this document.

### The result state: `WidgetEvent_AxisInputState`

Threaded up the call chain, one per pass. It carries the tree-structural facts the flat `AxisInputSource` cannot express. Members are private and move only through transition methods, so invariants can't be broken by writing a raw field.

- **`Passed`** — nothing in this subtree acted on the motion.
- **`Consumed`** — a descendant used the motion (an activated `Slider` took the control, or a `ScrollArea` scrolled). Ancestors must not act on it again, and the host skips focus-navigation for this control this frame.
- **`MarkFocusWithinSubtree()` / `FocusWithinSubtree()`** — set-only flag that bubbles up once dispatch passes through the focused Widget, so an ancestor `ScrollArea` knows to scroll the focused rect into view even when it consumed no motion itself.

#### Rationale: overscrolling was considered and dropped

An earlier design had a third state, `Overscrolled`, plus a residual value: an inner `ScrollArea` that hit its scroll limit would hand the leftover motion up to an ancestor scroll-container (the direct analog of navigation's `HitBoundary`), so a single stick push could scroll the inner list to its end and then continue scrolling the outer one — Android-style nested scrolling.

We dropped it. It only earns its keep when you have a `ScrollArea` nested inside another, driven by *smooth* motion, and expect crossing the inner edge to seamlessly continue the outer in one gesture — a niche interaction. Everything else is already covered: a single `ScrollArea` at its limit just clamps, and moving *between* nested regions is handled by focus-following + scroll-into-view. Cutting it collapses the result state to `Passed` / `Consumed` (+ the focus flag), which is far easier to reason about. Because the state is an enum plus an optional field, it can be reintroduced cheaply and non-breakingly if a real nested-smooth-scroll case ever shows up.

### Navigation suppression (why there is no "claim")

After a control's pass, the host inspects the returned **root** state. The immutable navigation pass runs for that control **only if the pass came back `Passed`**. A `Consumed` result means a Widget used the control this frame, so focus-navigation is skipped for it.

This is deliberate: it means "consumed" alone is enough, and there is **no separate claim mechanism**. An activated `Slider` steals the stick simply by consuming its pass; a `ScrollArea` in left-stick-smooth mode consumes and suppresses focus movement; a `ScrollArea` in left-stick-navigate mode leaves the pass `Passed`, lets navigation run, and reacts to `FocusWithinSubtree()` to scroll the new focus into view. These are configured modes, never simultaneous, which is exactly why one bit of information suffices.

## `AxisInputSource`

The resolved source of continuous-input controls. Unlike the other host interfaces (see Widget.md), this one is owned by the host and stays alive across frames, since it holds the elapsed-time state. Physical input is already resolved into `AxisId` controls here.

- `GetAnalog(id) → Vec2` — instantaneous, deadzone-corrected value in `[-1, 1]` per component (radial deadzone for a 2D control). Side-effect-free; reading never suppresses navigation. `Y` positive points down.
- `GetAnalogDelta(id) → Vec2` — the control's analog integrated over the current frame (deflection-seconds, i.e. `analog * dt`). A Widget multiplies by its own rate constant for frame-rate-independent smooth motion, so no `dt` crosses into the Widget.
- `TakeSteps(id) → Vec2Int` — signed discrete steps this frame, per component. Fed by both discrete sources (arrow keys, D-pad) and analog controls crossing the host's threshold, so a step is identical either way. Threshold and repeat cadence are host-global and internal. Usually `0` or `±1`; larger only on a long frame — kept a count so held repeat stays frame-rate-independent instead of dropping steps on a hitch.
