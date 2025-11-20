module;

#include <DEngine/Std/Containers/impl/Assert.hpp>

export module DEngine.Std.RangeFnRef;

import DEngine.Std.Trait;

export namespace DEngine::Std {

	// An range type that takes a delegate in order to generate a new value for every item.
	// This delegate receives an index and returns the value type.
	//
	// This type is best suited when you have a source array, and you want to map the array into a simple type
	// and then pass the resulting range into some other function.
	template<class T>
	class RangeFnRef {
	public:
		RangeFnRef() {}
		RangeFnRef(RangeFnRef const&) = delete;
		template<class Callable> requires Std::Trait::IsInvocable<T, Callable const&, int>
		RangeFnRef(int length, Callable const& callable) :
			m_size{ length }
		{
			this->actualFn = (void const*)&callable;
			this->wrapperFn = [](void const* actualFnIn, int i) -> T {
				auto& fn = *static_cast<Callable const*>(actualFnIn);
				return fn(i);
			};
		}

		RangeFnRef& operator=(RangeFnRef const&) = delete;

		[[nodiscard]] T Invoke(int i) const {
			DENGINE_IMPL_CONTAINERS_ASSERT(i < m_size);
			return wrapperFn(actualFn, i);
		}

		[[nodiscard]] auto Size() const { return m_size; }

		class EndIterator {};

		class Iterator {
		public:
			Iterator(RangeFnRef const& in) : original{ &in } {}

			[[nodiscard]] T Deref() const {
				return this->original->Invoke(currentIndex);
			}
			[[nodiscard]] T operator*() const { return this->Deref(); }

			void Increment() { currentIndex++; }
			void operator++() { this->Increment(); }

			[[nodiscard]] auto Equals(EndIterator const&) const {
				return currentIndex >= original->m_size;
			}
			[[nodiscard]] bool operator==(EndIterator const& in) const { return Equals(in); }
			[[nodiscard]] bool operator!=(EndIterator const& in) const { return !Equals(in); }

		private:
			int currentIndex = 0;
			RangeFnRef const* original = nullptr;
		};

		[[nodiscard]] auto begin() const { return Iterator(*this); }
		[[nodiscard]] auto end() const { return EndIterator{}; }

	private:
		int m_size = 0;
		void const* actualFn = nullptr;
		using WrapperFnT = T(*)(void const*, int);
		WrapperFnT wrapperFn = nullptr;
	};

}
