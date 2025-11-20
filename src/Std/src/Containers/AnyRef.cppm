export module DEngine.Std.AnyRef;

import DEngine.Std.Trait;

export namespace DEngine::Std {

	namespace impl {
		// A per-type sentinel byte. Taking its address yields a unique
		// compile-time identity for T with no RTTI and no runtime cost.
		// Const-qualifiers are stripped so AnyRef and ConstAnyRef agree
		// on identity for the same underlying type.
		template<class> struct AnyRefTypeTag { static char value; };
		template<class T> char AnyRefTypeTag<T>::value = 0;

		template<class T>
		[[nodiscard]] unsigned long long anyRefTypeHashOf() {
			using Bare = Trait::RemoveCVRef<T>;
			return reinterpret_cast<unsigned long long>(&AnyRefTypeTag<Bare>::value);
		}
	}

	class ConstAnyRef;
    class AnyRef {
    public:
        AnyRef() = default;
        AnyRef(AnyRef const& in) = default;
		// The template constraint makes it not get confused
		// with other constructors
        template<class T> requires (!Trait::isSame<T, AnyRef>)
        explicit AnyRef(T& in) : data{ &in }, typeHash{ impl::anyRefTypeHashOf<T>() } {}

        template<class T>
        T* Get() const {
        	if (typeHash != impl::anyRefTypeHashOf<T>()) {
        		return nullptr;
        	}
            return static_cast<T*>(data);
        }

		[[nodiscard]] ConstAnyRef ToConst() const;

		[[nodiscard]] void* Data() const { return data; }
		[[nodiscard]] auto TypeHash() const { return typeHash; }

    protected:
        void* data = nullptr;
        unsigned long long typeHash = 0;
    };

	class ConstAnyRef {
	public:
		ConstAnyRef() = default;
		ConstAnyRef(ConstAnyRef const& other) = default;
		explicit ConstAnyRef(AnyRef const& in) : data{ in.Data() }, typeHash{ in.TypeHash() } {}
		// The template constraint makes it not get confused
		// with other constructors
		template<class T> requires (!Trait::isSame<T, ConstAnyRef>)
		explicit ConstAnyRef(T const& in) : data{ &in }, typeHash{ impl::anyRefTypeHashOf<T>() } {}

		template<class T>
		T const* Get() const {
			if (typeHash != impl::anyRefTypeHashOf<T>()) {
				return nullptr;
			}
			return static_cast<T const*>(data);
		}

	protected:
		void const* data = nullptr;
		unsigned long long typeHash = 0;
	};

	inline ConstAnyRef AnyRef::ToConst() const { return ConstAnyRef{ *this }; }

}
