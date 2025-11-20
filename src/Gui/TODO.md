# Gui TODO

# Experiment: Polymorphic stack-like tree iterator

Right now every Layout has to implement every event function (Measure, Layout, Render, CursorMove, navigation, and so on) and forward it to its children by hand. Every time we add a new event or API to the Widget interface, every Layout has to be updated to forward it too. That's a lot of boilerplate, and it's easy to forget one.

The idea is to have a polymorphic, stack-like iterator type that can walk the entire UI tree. A Layout would then only have to implement the iterator, i.e how to enumerate its children and step into them, instead of implementing each event. Once we have that, we could dispatch most events generically by driving the iterator from the outside, so introducing a new event API wouldn't require touching every Layout, we'd get it automatically.