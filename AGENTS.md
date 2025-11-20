# General coding rules for DEngine

- Implementations should always favor speed and power-efficiency over simplicity and readability. Assume that we are running on a cheap mobile 8-core CPU, and we wish to save every bit of power possible, and making sure our threads can run on the efficiency cores always. This is the most important rule in the entire repository.
- No global state is allowed whatsoever, unless it's functionally impossible or if it explicitly makes sense. One such exception is when working with the DEngine Platform backends.
- Minimal usage of STL. STL tends to rely excessively on exceptions, overly complex solutions that don't apply to this project, and makes it harder to use custom allocators. Additionally, we try to minimize compilation cost of including headers. Where it's viable, use containers from DEngine::Std such as Std::Vec and Std::Box. These are enough for most usecases. In some cases, it's fine to use std::vector and std::string for long-lived memory.
- Unique ownership is strongly preferred. Only under exceptional cases, should shared ownership semantics be used.
- Function prerequisites are always passed using parameters. Classes should not be relying on statefulness to achieve its tasks. This means have >10 parameters is fine. An acceptable alternative to this is to make a struct specifically for the parameters, so that we may fill out this struct using designated initializers.
- No dependencies. By default, we don't use any dependencies in DEngine unless we absolutely have to. Some exceptions include zlib, FreeType and Vulkan Memory Allocator. Even these are subject to be rewritten. DEngine aims to use an anarchistic licensing model where nobody has to credit any authors of any of the software used.
- Temporary allocations are strictly illegal. This means that it is not allowed to use a short-lived std::vector, std::unique_ptr or anything else. Short-lived allocations are required to use a custom allocator such as the FrameAllocator.
- Error handling is preferred by using Std::Opt and/or std::expected. Eventually we should implement a Std::Result to replace std::expected. In certain particularly exceptional usecases, throwing exceptions is allowed.
- Asserts is preferred over error-handling in the cases where a mistake is a developer-error.
- Every module of the project should have its own assert-macro so that it may individually be turned on/off on a per-module basis.
- Use references rather than pointers, in function parameters, when the input is not allowed to be null.
- The project should ideally be split up into modules with clear separations of concern. Communication between modules should usually be handled with polymorphic virtual interfaces so that modules need not know about each other. It is acceptable to duplicate some structs and data-types across modules to make this happen.
- It is better to have duplicate code rather than settling on the wrong abstraction.
- When dealing with small collections, we should prefer linear storage types. This should be preferred when we expect the cache locality to outweigh any performances gained by better algorithmic complexity. This also holds true for containers where we expect to do more iterations than lookups.
- Code comments should not be overly wordy, and we don't comment code that is self-explantory. It's better to write additional code if it becomes more self-explained. Comment language should not be very formal.

## Definition of done
- We don't have tests. Tests will be implemented when APIs are more stable and we can test real hardware.

# Design goals for DEngine::Gui and DEngine::Gui::StdWidgets

Some overall design notes may be found in the Gui::Widget class definiton in Widget.hpp.

Some of these goals are not achieved yet.

- The API of GUI components is never set in stone. The entire module is subject to complete rewrites at any given time.
- The GUI module is strictly not designed to be easy-to-use. The GUI module is an experiment to explore UI programming pattern, data structures and algorithms. The goal is to find out what is strictly required for the GUI module to function within the requirements given.
- The GUI should be used to power the game engine editor of DEngine, as well as any in-game UI. The UI must work as a traditional desktop/mobile UI as well as support being hosted inside a 3D game scene.
- The GUI module must support multiple OS windows
- The GUI module must have no global state, so that we can run multiple instances of GUI inside the same process, in paralell.
- The UI tree consists of UI nodes. These may in turn contain child-nodes.
- UI nodes may not contain pointers that move up the UI tree. This is strictly forbidden.
- The UI tree is resolved into rectangles laid out on a plane.
- UI nodes should preferably be allocated in arena allocators, so that their memory may be reused and compacted.
- Certain operations on the UI tree must be immutable. This includes rendering and navigation.
- The Gui module may never know what platform it is running on. By default we are targeting Windows and Android, with primary focus on Android.
- The RectCollection should be designed to contain information of the UI tree after it has been measured and layout phase is finished.
- The GUI module may never know how it is rendered. The GUI module should only emit abstract drawing commands. Currently we are using Vulkan but this is an external implementation detail as far as the UI is concerned.
- The GUI module may never know what text-engine is powering it. It relies on an abstract TextEngine object that it uses to measure text. Currently we are using FreeType but this is an external implementation detail as far as the UI is concerned.
- Virtual interfaces and event dispatching should have clear rules for how they should be implemented, how they should behave, when effects should happen. This should be documented using comments in the code.

## Deprecated patterns

The GUI module is currently under reconstruction. Particularly, the Layer and Context classes are deprecated and should not be used by any new implementations.