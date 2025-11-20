export module DEngine.Gui.TextInputType;

export namespace DEngine::Gui {
	enum class TextInputType : char {
		NoFilter,
		SignedFloat,
		UnsignedFloat,
		SignedInteger,
		UnsignedInteger
	};
}
