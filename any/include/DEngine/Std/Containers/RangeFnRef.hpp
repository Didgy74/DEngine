#pragma once

namespace DEngine::Std {

	template<class T>
	class RangeFnRef {
	public:
		RangeFnRef() {};
		template<class Callable>
		RangeFnRef(int length, Callable const& callable) :
			_size{ length }
		{
			this->actualFn = (void const*)&callable;
			this->wrapperFn = [](void const* actualFnIn, int i) {
				auto& fn = *static_cast<Callable const*>(actualFnIn);
				return fn(i);
			};
		}

		[[nodiscard]] T Invoke(int i) const {
			DENGINE_IMPL_ASSERT(i < Size());
			return wrapperFn(actualFn, i);
		}

		[[nodiscard]] auto Size() const { return _size; }

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
				return currentIndex >= original->_size;
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
		int _size = 0;
		void const* actualFn = nullptr;
		using WrapperFnT = T(*)(void const*, int);
		WrapperFnT wrapperFn = nullptr;
	};

}