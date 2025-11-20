export module DEngine.Std.Range;

export namespace DEngine::Std {
	template<typename Iterator>
	struct Range {
		Iterator begin;
		Iterator end;
	};
}