#pragma once

#include <DEngine/Std/Trait.hpp>

namespace DEngine::Std {

	class ConstAnyRef;
    class AnyRef {
    public:
        AnyRef() = default;
        AnyRef(AnyRef const& in) = default;
		// The template constraint makes it not get confused
		// with other constructors
        template<class T> requires (!Trait::isSame<T, AnyRef>)
        explicit AnyRef(T& in) : data{ &in } {
            // TODO: Set typehash
        }

        template<class T>
        T* Get() const { return static_cast<T*>(data); }

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
		explicit ConstAnyRef(T& in) : data{ &in } {
			// TODO: Set typehash
		}

		template<class T>
		T const* Get() const { return static_cast<T const*>(data); }

	protected:
		void const* data = nullptr;
		unsigned long long typeHash = 0;
	};

	inline ConstAnyRef AnyRef::ToConst() const { return ConstAnyRef{ *this }; }

}