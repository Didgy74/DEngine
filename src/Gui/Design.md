# DEngine::Gui design notes

These are the overall design notes for the Gui module. They cover the core concepts, the layouting pipeline, coordinates, the RectCollection, and the reasoning behind the bigger design decisions.

This is one of several documents:

- Design.md (this file) — overall design and rationale.
- Widget.md — the Widget API, and how to implement a Widget correctly.
- Navigation.md — UI navigation and axis input.

The header `include/DEngine/Gui/Widget.hpp` is the source of truth for the actual API. These documents are the source of truth for behaviour and intent. When they disagree, one of them is a bug, fix it. We also keep notes on design rationale here, so that we understand why things are the way they are and avoid running into old design mistakes again.

### Some design goals

> The GUI module is deliberately not designed to be easy to use. It is an experiment in UI data structures, algorithms and dispatch patterns. The API is never set in stone and any part of it may be rewritten at any time.

- Complexity over simplicity and usability
  - I don't care if nobody ever uses this UI solution, not even if it means I end up never using it. We never simplify any APIs if it means we lose flexibility or potential performance improvement. Every API usage should always consider every possible edge-case.
  - I'm making something so that I can learn, not trying to solve someone else's problem. 
- Fast and lightweight
  - The UI solution should trade away usability if it allows us to use less memory and/or improve processing speed / runtime performance
  - The goal is to make UI processing so fast we can run easily run it on the order of micro-seconds on cheap ARM CPUs with the UI workload pinned to "little" cores. We should be so fast they never need to clock themselves higher. A good minimum target hardware for now is Raspberry Pi 5. 
- No batteries included
  - The UI solution never provides tools to interact with the platform, track input, create windows etc. This should always be provided by the UI host (read: application that hosts the UI).
  - The UI solution never provides tools for rendering. This is entirely up to the UI host.
  - Ideally, the UI solution never knows what platform it is running on, nor does it know if it's running headless or not.
- A "what you see is what you get" approach to implementation design
  - No hidden costs. It should be fairly obvious if a method will take a long time to execute. No hidden cache updates or anything like that.
- No global state
  - Strictly forbidden in any shape or form. It is a hard requirement that it must be possible to run multiple instances of the UI system concurrently on multiple threads without them interacting with each other at all.
- Use custom memory allocators
  - The intention is to normalize heavy use of custom allocators where appopriate, so that we always tightly pack memory, reuse memory and generally avoid system calls such as malloc which can be heavy. 
  - Ideally, we never rely on the global allocator at any point. In practice, exceptions tend to be made for long-lived objects.
  - Short-lived allocations are strictly prohibited. One such example is creating a std::vector in the middle of UI tree traversal. These types of allocation *must* use the frame-allocator passed down in every Widget API.
  - Long-lived allocation should *preferably* live in bucket-allocators that make it fast to reuse memory.
    - In particular, this pattern should be used for Widgets. This has not yet been implemented. But in general, Widget pointers should not count as unique identifiers, memory might be reused for new Widget instances.

---

# Core concepts

## Widgets and the UI tree

The UI is a directed N-tree of **Widgets**. A Widget is a visual and/or interactable item that occupies a 2D subregion of the UI plane. It may or may not contain child Widgets.

The tree is *largely* a directed acyclic graph, with a few deliberate exceptions (for example a Widget that holds a text-input session).

A hard rule: **a Widget may never hold a pointer that moves up the tree.** Parents know their children; children never know their parents. Every prerequisite a Widget needs is handed to it through event parameters instead — this is what keeps the tree movable, arena-allocatable and safe to process immutably.

## Layout items

A **Layout** is a Widget that contains other Widgets and is responsible for positioning and sizing them relative to itself, and for propagating events to them correctly.

- A **pure Layout** has no logic or visuals of its own. It only arranges its children and forwards events. With no children it does nothing useful. Example: `StackLayout`.
- A **hybrid Layout** also has real logic or visuals of its own, and can do or show something even with no children. Example: `DockArea`.

## The UI plane, the UI host, and layers

The UI is projected onto a bounded 2D **UI plane**; bounded by a width and height, in pixels. There is no `UIPlane` class, the surrounding system that implements that concept is called the **UI host**. The surrounding system in this case is the application that integrates the GUI solution, it is not provided by this GUI solution. `Gui::Context` is one example of a host; a 3D scene placing UI planes in world space is another. A UI plane is conceptually just a 2D square holding the root Widget, plus layers. The UI host is intended to be implemented by whatever application is integrating the UI solution. `Gui::Context` is an optional helper class designed to make it easier to integrate it when under simple application-UI conditions, where you have regular displays and windows (i. not a complex 3D scene).

A plane may also carry **layers**: popups, menus or dialogs floating in front of the back plane. The terms back plane, base plane, or "rearmost layer" means the actual square containing the root Widget. "Layer" is a concept, not a class. The host positions and sizes them.  **Events are dispatched to the frontmost layer first and travel toward the rear**, so a popup gets first refusal on input before the plane behind it. Events keep propagating to the base plane if the layers in front of it do not consume the event.

> Note: Currently, there is only really support for a single layer in front of the base layer, but this is intended to be redesigned.

Every layer that is not the base plane, can potentially float outside the base plane's bounds. Whether it can actually float outside or not, depends on the UI host. In effect, they may or may not be bounded. Conceptually, they are placed along the same plane as the baseplane, but just offset within that 2D space. The intention is that Layers may be implemented either "virtually" inside a single OS window, or each layer may be represented as it's own "child window". For desktop OSes, we prefer to implement it as child windows, on mobile they are implemented as virtual usually. When Widgets want to spawn a new layer, they provide their desired size and position, but it's ultimately up to the UI host to determine where they will exist. 

### Rationale

A Widget explicitly does not know anything about any other UI planes. In earlier designs, the Gui::Context class was required always and all UI planes had to be hosted inide one. It assumed all UI planes would be placed on the same 2D plane. This is true for desktop and mobile, but not when you have UI hosted in 3D space. When dealing with UI hosted in 3D space, you may have different UIs existing in different "universes" and so you can never assume that all planes share some global state or exist on the same 2D plane.

# Layouting and events

The GUI solution is strongly designed to facilitate automatic and reactive layouting. The goal is to always have an algorithmic and mathematical description of the entire UI in all its shapes and forms. Then once we know the actual window surface, DPI etc. then we can calculate where everything is meant to be. This means we can, in theory, support any window size (within reasonable limits).

The layouting pipeline is not finalized, and details may change in the future. But generally speaking it follows 3 steps:

1. Measuring
2. Screenspace assignment
3. Event dispatching

The "measuring" stage traverses the N-tree in a bottom-up fashion. The goal is to measure what is the ideal / minimum size for a given Widget. A measurement results in a SizeHint object, which contains hints on how the given Widget should be sized to be presented properly. This is a read-only operation on the UI tree.

The "screenspace assignment" traverses the N-tree in a top-down fashion. The root Widget (commonly a Layout type) will adopt the size of the UI plane in which it lives (i.e the window), and then it will start assigning screenspace within that UI plane to it's children. Once all (visible) Widgets have been asssigned screenspace, it marks the end of actual layouting. Once this is done, we must set the UI anchor correctly (most commonly as a result of the on-screen keyboard appearing). This is a read-only operation on the UI tree.

Even Widgets that end up off-screen must still be assigned a local rect during this stage. The UI anchor is applied afterwards, and it can translate an off-screen Widget back into view, the classic case being a text-field pushed up above an on-screen keyboard. If those Widgets had no rect, we would have nothing to translate. So the "(visible)" above is a simplification: we assign rects to all Widgets, we just don't do much more with the ones that stay off-screen.

Once these steps are done, we can start doing something useful. An event tends to describe something happening, and the UI needs to respond. An event can be the user performing touch press on the UI, or using a joystick to interact with the UI. Event "dispatching" means that we are traversing the UI tree, commonly top-down, to inform each Widget so that they may do something useful as a response to the event.

Note that in general we should *never* modify the UI tree during layouting or event dispatching. Modifying the tree while we are traversing it can have unpredictable consequences. When we want to modify the tree, or otherwise have side-effects, we mostly want to queue up our functions and execute them after the traversal is complete. There are few exceptions to this rule, they should be carefully considered.

Rendering counts mostly as a special type of read-only event.

In most cases, we need to relayout in between every event.

### Rationale

It is critical that measurement happens in the bottom-up order. This is because the SizeHint of Layout types tend to be a sum of their children's SizeHints. For example, a StackLayout's SizeHint will be all the children's SizeHints stacked next to each other. So all children must be measured, before we can successfully measure the StackLayout.

We perform screenspace assignment only once all measuring is done. This is important because we usually need to know the SizeHints of all immediate children in order to determine how to subdivide our screenspace between them. This is because screenspace is finite, but the collective measured size of children may be infinite. Therefore, we must make a compromise on how much size each child gets, based on the measured size of all the immediate children.

Not all events require a layouting, but most do. Most events cannot do anything useful until we have determined where everything is on screen, and that means it has to happen after all layouting is done.

The reason we don't allow UI tree changes during layout/dispatching is because it is extremely convoluted to track changes that happen while traversing (commonly done recursively). It also makes it very hard to ensure some operations are truly read-only and thread-safe. When traversing, we want to traverse things in the same order as everything was at the time when traversal started. I initially prototyped some Widget solutions where each Widget would track if they're currently being iterated, and track modifications to their list of children, and compare against it while they were looping over their children. But additionally, we need the order of traversal to be maintained across measuring, layouting *and* event dispatches, which means all recursion context is lost in between anyways. 

# Coordinates

The window origin is `(0, 0)`. The UI is anchored at some offset relative to the plane origin (the host decides where; you can offset everything to move the origin). The anchor is strictly applied after all layouting is done. 

- The coordinate system used everywhere is right-handed, with X facing the right, Y facing down, and Z pointing into the screen away from the viewer. 
- **Point events** (cursor, touch) carry coordinates relative to the **window**, not the UI anchor.
- If a pointer was pressed inside the plane, its move events keep being delivered even when it leaves the window — so a drag/scroll gesture survives a finger sliding off the edge.

### Rationale

We need to have some way to offset the entire UI *after* layouting is done. The most important example here is when you have text input and an on-screen keyboard. Let's assume you have a smartphone, and a text-field at the bottom of the screen. If you press this text-field, the on-screen keyboard will show up to cover the bottom of the screen, obscuring the text-field. An important feature of this anchor point solution is that we may then offset the entire UI wholesale such that we can see the text-field.

# The RectCollection

The RectCollection is a container that holds information on the resolved screenspace positions and sizes for all Widgets in a UI plane, for a given layouting procedure. The RectCollection is valid for exactly one layout, and any changes to the UI tree invalidates the RectCollection. Additionally, the RectCollection tends to hold basic metadata for each Widget as well, such as whether a Widget should be considered for navigation.

For layouting, we need to separate a Widget's rect, and the viewport through which it is visible. The terms for these are "widgetRect" and "visibleRect". The RectCollection stores both of these for every Widget enty. As an example, assume a button inside a scrollable view. The scrollable view may have adjusted itself such that only half the button is visible. The button still has the same "physical" size on screen, but it has been occluded and we don't want the occluded parts to still be interactable. This is also useful for doing culling.

Each Widget entry in the RectCollection may optionally hold custom data. This can be any type, defined by the specific Widget itself. The point of this custom data is to provide a way for the Widget to "remember" information in between layouting stages. Under normal circumstances, any info the Widget processed during the measuring stage, will be forgotten and will have to be re-processed during the "screenspace assignment" stage. By attaching custom data to the RectCollection, it can save CPU time.

At present, a RectCollection will hold the screenspace info for all Widgets for *all Layers* inside a UI plane. In the future, it's possible we will have separate RectCollections for each Layer instead.

In the future we may introduce diffing of UI trees in between events. Diffing can be useful for identifying, and only updating parts of the UI we know has changed. In this case, it may be useful to introduce some kind of per-Widget state that can survive across events.

The RectCollection is built up piece-by-piece during the layouting pipeline, using builder patterns so that we can push SizeHints and whatnot over time.

Each Widget entry is given an Iterator, the intention of this is to make lookup faster. The Iterator is invalidated when the RectCollection is invalidated.

As a general rule, a Widget's interactable and renderable area may not extend beyond its resolved `widgetRect`.

The intent is for as much Widget state as possible to live in the `RectCollection`, so that as many operations as possible (rendering, navigation) can be **immutable**.

> **ISSUE:** There are some issues with visibleRect is defined and implemented atm. It might make sense to use Opt<Rect> so we can represent no visible bounds. We need to clarify whether visibleRect moves with the UI anchor or not.

#### Rationale

Generally speaking, we can always know the size of a Widget by just evaluating and measuring it's children all over again. In practice, this has proven to be very slow for runtime performance (particularly when text measurement is involved). The goal of the RectCollection is essentially to cache all results from a layout to avoid doing duplicate work.

A major benefit of the RectCollection is that it allows us to move all sizes and positions out of the Widget object themselves, which means layouting can truly be a read-only operation. An interesting side-effect is also that it means we can effectively represent a UI-tree in multiple different window-configurations at the same time, though we don't actually do this for anything atm.

The reason the RectCollection is invalidated so easily, is because any modification to the UI tree, or even any content inside an item, will affect a SizeHint, which can impact the screenspace-distribution across the entire UI plane.

Access happens through `RectCollection::GetEntry` (Widget → `Iter`) and `GetLocalRect` (`Iter` → `RectPair`). Most events take `widgetRect` / `visibleRect` and a `RectCollection::Iter` so containers that already looked these up don't pay for the lookup twice.

---

## Design rationale

- **No up-tree pointers; everything by parameter.** A Widget receives all of its prerequisites through event params rather than reaching upward or relying on stored state. This is what lets the tree be arena-allocated, compacted, moved between hosts, and processed immutably. It is also why param structs are large and why the module tolerates functions with many inputs.
- **Immutable where it counts.** Rendering and navigation are immutable and thread-safe; anything effectful is either an explicitly mutable event or is deferred. The awkward Navigation/Activate split exists purely to preserve navigation's immutability while still yielding a mutable pointer to act on.
- **The host owns the platform, the time and the policy.** The GUI never learns what platform, renderer or text engine powers it, and — since `AxisInput` — never learns about time either. Deadzones, repeat cadence and the physical→logical input mapping all live in host-side interfaces (`AxisInputSource`, `DrawEngine`, `TextEngine`, `WindowHandler`), so the tree stays portable and testable.
- **One bit beats a registry.** The axis-input design keeps deliberately choosing the smaller mechanism: a single `AxisId` vector instead of per-direction ids; `Passed` vs `Consumed` instead of a claim ledger; consumption-carries-suppression instead of a separate ownership flag. Each collapse is justified by reading the actual requirements rather than the imagined ones.
- **State lives in the RectCollection when it can.** The more Widget state is pushed into the measured/laid-out `RectCollection`, the more operations can be immutable and parallel.

### Known rough edges (from the header TODOs)

- `widgetRect` / `visibleRect` are passed as separate params *and* the `RectCollection` is passed too; a `RectCollectionAccessor` that resolves to iterator, raw values, or nothing would be cleaner.
- `treeOffset` and `windowExtent` are threaded per-event and should probably move into the per-event params structs.
- Some interactions still need a reference up to the host (e.g. `Widget_TextInputSession`), which complicates moving nodes across hosts; a reset mechanism may be needed.
- `TouchMove` overloads `occluded` to mean "consumed"; this is expected to change.
