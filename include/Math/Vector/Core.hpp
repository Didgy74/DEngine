#pragma once

#include "../Common.hpp"
#include "../Matrix/Matrix.hpp"
#include "../Vector/Vector.hpp"

#include <array>
#include <cassert>
#include <initializer_list>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>

namespace Math
{
	template<size_t length, typename T>
	struct Vector;

	namespace detail
	{
		template<size_t length, typename T>
		struct VectorBase;

		namespace Vector
		{
			template<size_t length, typename T>
			class Iterator
			{
				VectorBase<length, T>& obj;
				size_t index;
			public:
				constexpr Iterator(VectorBase<length, T>& obj, size_t index = 0) noexcept : obj(obj), index(index) {}

				static constexpr Iterator<length, T> End(VectorBase<length, T>& obj) { return Iterator<length, T>(obj, length); }

				[[nodiscard]] constexpr T& operator*() { assert(index < length); return obj[index]; }
				constexpr Iterator<length, T>& operator++() { index++; return *this; }
				constexpr Iterator<length, T>& operator--() { index--; return *this; }
				constexpr bool operator!=(const Iterator<length, T>& right) const { return &obj != &right.obj || index != right.index; }
				constexpr bool operator==(const Iterator<length, T>& right) const { return &obj == &right.obj && index == right.index; }
			};

			template<size_t length, typename T>
			class ConstIterator
			{
				const VectorBase<length, T>& obj;
				size_t index;
			public:
				constexpr ConstIterator(const VectorBase<length, T>& obj, size_t index = 0) noexcept : obj(obj), index(index) {}

				static constexpr ConstIterator<length, T> End(const VectorBase<length, T>& obj) { return ConstIterator<length, T>(obj, length); }

				[[nodiscard]] constexpr const T& operator*() { assert(index < length); return obj[index]; }
				constexpr ConstIterator<length, T>& operator++() { index++; return *this; }
				constexpr ConstIterator<length, T>& operator--() { index--; return *this; }
				constexpr bool operator!=(const ConstIterator<length, T>& right) const { return &obj != &right.obj || index != right.index; }
				constexpr bool operator==(const ConstIterator<length, T>& right) const { return &obj == &right.obj && index == right.index; }
			};
		}

		template<size_t length, typename T>
		struct VectorBaseStruct
		{
		private:
			std::array<T, length> data;
		public:
			[[nodiscard]] constexpr T& operator[](size_t index) { return const_cast<T&>(std::as_const(*this)[index]); }
			[[nodiscard]] constexpr const T& operator[](size_t index) const { return data[index]; }
		};

		template<typename T>
		struct VectorBaseStruct<2, T>
		{
			T x = T{};
			T y = T{};
			[[nodiscard]] constexpr T& operator[](size_t index) { return const_cast<T&>(std::as_const(*this)[index]); }
			[[nodiscard]] constexpr const T& operator[](size_t index) const
			{
				assert(index < 2);
				switch (index)
				{
				case 0:
					return x;
				case 1:
					return y;
				default:
					return x;
				}
			}
		};

		template<typename T>
		struct VectorBaseStruct<3, T>
		{
			T x = T{};
			T y = T{};
			T z = T{};
			[[nodiscard]] constexpr T& operator[](size_t index) { return const_cast<T&>(std::as_const(*this)[index]); }
			[[nodiscard]] constexpr const T& operator[](size_t index) const
			{
				assert(index < 3);
				switch (index)
				{
				case 0:
					return x;
				case 1:
					return y;
				case 2:
					return z;
				default:
					return x;
				}
			}
		};

		template<typename T>
		struct VectorBaseStruct<4, T>
		{
			T x = T{};
			T y = T{};
			T z = T{};
			T w = T{};
			[[nodiscard]] constexpr T& operator[](size_t index) { return const_cast<T&>(std::as_const(*this)[index]); }
			[[nodiscard]] constexpr const T& operator[](size_t index) const
			{
				assert(index < 4);
				switch (index)
				{
				case 0:
					return x;
				case 1:
					return y;
				case 2:
					return z;
				case 3:
					return w;
				default:
					return x;
				}
			}
		};

		template<size_t length, typename T>
		struct VectorBase : public VectorBaseStruct<length, T>
		{
			using ParentType = VectorBaseStruct<length, T>;
			using LengthType = size_t;
			using ValueType = T;
			using Iterator = Vector::Iterator<length, T>;
			using ConstIterator = Vector::ConstIterator<length, T>;

			[[nodiscard]] static constexpr size_t GetLength();

			[[nodiscard]] auto Magnitude() const;
			[[nodiscard]] auto MagnitudeSqrd() const;

			[[nodiscard]] auto GetNormalized() const;
			void Normalize();

			template<typename T2>
			[[nodiscard]] static constexpr auto Dot(const VectorBase<length, T>& left, const VectorBase<length, T2>& right);

			[[nodiscard]] static constexpr Math::Vector<length, T> SingleValue(const T& input);
			[[nodiscard]] static constexpr Math::Vector<length, T> Zero();
			[[nodiscard]] static constexpr Math::Vector<length, T> One();

			[[nodiscard]] std::string ToString() const;

			template<typename T2>
			[[nodiscard]] constexpr auto operator+(const VectorBase<length, T2>& right) const;
			template<typename T2>
			[[nodiscard]] constexpr auto operator-(const VectorBase<length, T2>& right) const;
			[[nodiscard]] constexpr auto operator-() const;

			template<typename T2>
			[[nodiscard]] constexpr bool operator==(const VectorBase<length, T2>& right) const;
			template<typename T2>
			[[nodiscard]] constexpr bool operator!=(const VectorBase<length, T2>& right) const;

			[[nodiscard]] constexpr Vector::Iterator<length, T> begin() { return Vector::Iterator(*this); }
			[[nodiscard]] constexpr Vector::ConstIterator<length, T> begin() const { return Vector::ConstIterator(*this); }
			[[nodiscard]] constexpr Vector::Iterator<length, T> end() { return Vector::Iterator<length, T>::End(*this); }
			[[nodiscard]] constexpr Vector::ConstIterator<length, T> end() const { return Vector::ConstIterator<length, T>::End(*this); }
		};

		template<size_t length, typename T>
		constexpr size_t VectorBase<length, T>::GetLength() { return length; }

		template<size_t length, typename T>
		auto VectorBase<length, T>::Magnitude() const { return Math::Sqrt(MagnitudeSqrd()); }

		template<size_t length, typename T>
		auto VectorBase<length, T>::MagnitudeSqrd() const
		{
			T sum = T();
			for (const auto& item : (*this))
				sum += Math::Sqrd(item);
			return sum;
		}

		template<size_t length, typename T>
		auto VectorBase<length, T>::GetNormalized() const
		{
			auto magnitude = Magnitude();
			using ReturnType = decltype((*this)[0] / magnitude);
			Math::Vector<length, ReturnType> returnVector;
			for (size_t i = 0; i < length; i++)
				returnVector[i] = (*this)[i] / magnitude;
			return returnVector;
		}

		template<size_t length, typename T>
		void VectorBase<length, T>::Normalize()
		{
			auto magnitude = Magnitude();
			for (auto& item : (*this))
				item /= magnitude;
		}

		template<size_t length, typename T>
		constexpr Math::Vector<length, T> VectorBase<length, T>::SingleValue(const T& input)
		{
			Math::Vector<length, T> newVector;
			for (auto& item : newVector)
				item = input;
			return newVector;
		}

		template<size_t length, typename T>
		constexpr Math::Vector<length, T> VectorBase<length, T>::Zero() { return Math::Vector<length, T>::SingleValue(0); }

		template<size_t length, typename T>
		constexpr Math::Vector<length, T> VectorBase<length, T>::One() { return Math::Vector<length, T>::SingleValue(1); }

		template<size_t length, typename T>
		std::string VectorBase<length, T>::ToString() const
		{
			std::ostringstream stream;
			stream << '(';
			for (size_t i = 0; i < length; i++)
			{
				if constexpr (std::is_same<char, T>() || std::is_same<unsigned char, T>())
					stream << +(*this)[i];
				else
					stream << (*this)[i];
				if (i < length - 1)
					stream << ", ";
			}
			stream << ')';
			return stream.str();
		}

		template<size_t length, typename T>
		template<typename T2>
		constexpr auto VectorBase<length, T>::Dot(const VectorBase<length, T>& left, const VectorBase<length, T2>& right)
		{
			using ReturnType = decltype(left[0] * right[0]);
			ReturnType dotProduct = ReturnType();
			for (size_t i = 0; i < length; i++)
				dotProduct += left[i] * right[i];
			return dotProduct;
		}

		template<size_t length, typename T>
		template<typename T2>
		constexpr auto VectorBase<length, T>::operator+(const VectorBase<length, T2>& right) const
		{
			using ReturnType = decltype((*this)[0] + right[0]);
			Math::Vector<length, ReturnType> newVector;
			for (size_t i = 0; i < length; i++)
				newVector[i] = (*this)[i] + right[i];
			return newVector;
		}

		template<size_t length, typename T>
		template<typename T2>
		constexpr auto VectorBase<length, T>::operator-(const VectorBase<length, T2>& right) const
		{
			using ReturnType = decltype((*this)[0] - right[0]);
			Math::Vector<length, ReturnType> newVector;
			for (size_t i = 0; i < length; i++)
				newVector[i] = (*this)[i] - right[i];
			return newVector;
		}

		template<size_t length, typename T>
		constexpr Math::Vector<length, T> operator*(const Math::Vector<length, T>& left, const T& right)
		{
			Math::Vector<length, T> newVec{};
			for (size_t i = 0; i < length; i++)
				newVec[i] = left[i] * right;
			return newVec;
		}

		template<size_t length, typename T>
		constexpr Math::Vector<length, T> operator*(const T& left, const Math::Vector<length, T>& right)
		{
			Math::Vector<length, T> newVec;
			for (size_t i = 0; i < length; i++)
				newVec[i] = left * right[i];
			return newVec;
		}

		template<size_t length, typename T>
		template<typename T2>
		constexpr bool VectorBase<length, T>::operator==(const VectorBase<length, T2>& right) const
		{
			for (size_t i = 0; i < length; i++)
			{
				if ((*this)[i] != right[i])
					return false;
			}
			return true;
		}

		template<size_t length, typename T>
		template<typename T2>
		constexpr bool VectorBase<length, T>::operator!=(const VectorBase<length, T2>& right) const
		{
			for (size_t i = 0; i < length; i++)
			{
				if ((*this)[i] != right[i])
					return true;
			}
			return false;
		}

		template<size_t length, typename T>
		constexpr auto VectorBase<length, T>::operator-() const
		{
			constexpr bool unsignedTest = std::is_unsigned<T>::value;
			using ReturnType = typename std::conditional<unsignedTest, T, T>::type;
			Math::Vector<length, ReturnType> newVector;
			for (size_t i = 0; i < length; i++)
			{
				if constexpr (unsignedTest)
					newVector[i] = -static_cast<ReturnType>((*this)[i]);
				else
					newVector[i] = -(*this)[i];
			}
			return newVector;
		}
	}
}