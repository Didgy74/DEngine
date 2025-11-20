module;

#include <DEngine/Math/impl/Assert.hpp>

export module DEngine.Math.Vector;

import DEngine.FixedWidthTypes;

export namespace DEngine::Math {
	template<uSize length, typename T>
	struct Vector {
		using ValueType = T;

		T data[length];

		[[nodiscard]] constexpr auto& operator[](uSize index) noexcept { return data[index]; }
		[[nodiscard]] constexpr auto operator[](uSize index) const noexcept { return data[index]; }

		[[nodiscard]] static constexpr Vector SingleValue(T input) noexcept {
			Vector temp;
			for (int i = 0; i < length; i += 1) {
				temp.data[i] = input;
			}
			return temp;
		}
	};

	using Vec2Int = Vector<2, i32>;
	template<>
	struct Vector<2, i32> {
		using ValueType = i32;

		i32 x;
		i32 y;

		[[nodiscard]] constexpr Vector<3, i32> AsVec3(i32 zValue = 0) const;
		[[nodiscard]] constexpr Vector<4, i32> AsVec4(i32 zValue = 0, i32 wValue = 0) const;

		[[nodiscard]] f32 Magnitude() const;
		[[nodiscard]] constexpr f32 MagnitudeSqrd() const;
		[[nodiscard]] Vector<2, f32> GetNormalized() const;

		[[nodiscard]] constexpr i32* Data();
		[[nodiscard]] constexpr i32 const* Data() const;

		[[nodiscard]] static constexpr i32 Dot(Vector const& lhs, Vector const& rhs);

		[[nodiscard]] static constexpr Vector SingleValue(i32 const& input);
		[[nodiscard]] static constexpr Vector Zero();
		[[nodiscard]] static constexpr Vector One();
		[[nodiscard]] static constexpr Vector Up();
		[[nodiscard]] static constexpr Vector Down();
		[[nodiscard]] static constexpr Vector Left();
		[[nodiscard]] static constexpr Vector Right();

		[[nodiscard]] constexpr i32& operator[](uSize i) noexcept;
		[[nodiscard]] constexpr i32 operator[](uSize i) const noexcept;

		constexpr Vector& operator+=(Vector const& rhs);
		constexpr Vector& operator-=(Vector const& rhs);
		constexpr Vector& operator*=(i32 const& rhs);
		[[nodiscard]] constexpr Vector operator+(Vector const& rhs) const;
		[[nodiscard]] constexpr Vector operator-(Vector const& rhs) const;
		[[nodiscard]] constexpr Vector operator-() const;
		[[nodiscard]] constexpr bool operator==(Vector const& rhs) const;
		[[nodiscard]] constexpr bool operator!=(Vector const& rhs) const;
	};

	constexpr Vector<3, i32> Vector<2, i32>::AsVec3(i32 zValue) const {
		return Vector<3, i32>{ x, y, zValue };
	}

	constexpr Vector<4, i32> Vector<2, i32>::AsVec4(i32 zValue, i32 wValue) const {
		return Vector<4, i32>{ x, y, zValue, wValue };
	}

	constexpr f32 Vector<2, i32>::MagnitudeSqrd() const {
		return f32((x * x) + (y * y));
	}

	constexpr i32* Vector<2, i32>::Data() {
		return &x;
	}

	constexpr i32 const* Vector<2, i32>::Data() const {
		return &x;
	}

	constexpr i32 Vector<2, i32>::Dot(Vector const& lhs, Vector const& rhs) {
		return (lhs.x * rhs.x) + (lhs.y * rhs.y);
	}

	constexpr Vector<2, i32> Vector<2, i32>::SingleValue(i32 const& input) {
		return Vector{ input, input };
	}
	constexpr Vector<2, i32> Vector<2, i32>::Zero() {
		return Vector{ i32(0), i32(0) };
	}
	constexpr Vector<2, i32> Vector<2, i32>::One() {
		return Vector{ i32(1), i32(1) };
	}
	constexpr Vector<2, i32> Vector<2, i32>::Up() {
		return Vector{ i32(0), i32(1) };
	}
	constexpr Vector<2, i32> Vector<2, i32>::Down() {
		return Vector{ i32(0), i32(-1) };
	}
	constexpr Vector<2, i32> Vector<2, i32>::Left() {
		return Vector{ i32(-1), i32(0) };
	}
	constexpr Vector<2, i32> Vector<2, i32>::Right() {
		return Vector{ i32(1), i32(0) };
	}

	constexpr i32& Vector<2, i32>::operator[](uSize i) noexcept {
		DENGINE_IMPL_MATH_ASSERT_MSG(
			i < 2,
			"Attempted to index into a Vec<2, i32> with an index out of bounds.");
		return (&x)[i];
	}
	constexpr i32 Vector<2, i32>::operator[](uSize i) const noexcept {
		DENGINE_IMPL_MATH_ASSERT_MSG(
			i < 2,
			"Attempted to index into a Vec<2, i32> with an index out of bounds.");
		return (&x)[i];
	}

	constexpr Vector<2, i32>& Vector<2, i32>::operator+=(Vector const& rhs) {
		x += rhs.x;
		y += rhs.y;
		return *this;
	}
	constexpr Vector<2, i32>& Vector<2, i32>::operator-=(Vector const& rhs) {
		x -= rhs.x;
		y -= rhs.y;
		return *this;
	}
	constexpr Vector<2, i32>& Vector<2, i32>::operator*=(i32 const& rhs) {
		x *= rhs;
		y *= rhs;
		return *this;
	}
	[[nodiscard]] constexpr Vector<2, i32> Vector<2, i32>::operator+(Vector const& rhs) const {
		return Vector{ x + rhs.x, y + rhs.y };
	}
	[[nodiscard]] constexpr Vector<2, i32> Vector<2, i32>::operator-(Vector const& rhs) const {
		return Vector{ x - rhs.x, y - rhs.y };
	}
	[[nodiscard]] constexpr Vector<2, i32> Vector<2, i32>::operator-() const {
		return Vector{ -x, -y };
	}
	[[nodiscard]] constexpr bool Vector<2, i32>::operator==(Vector const& rhs) const {
		return x == rhs.x && y == rhs.y;
	}
	[[nodiscard]] constexpr bool Vector<2, i32>::operator!=(Vector const& rhs) const {
		return x != rhs.x || y != rhs.y;
	}

	constexpr Vector<2, i32> operator*(Vector<2, i32> const& lhs, i32 const& rhs) {
		return Vector<2, i32>{ lhs.x* rhs, lhs.y* rhs };
	}

	constexpr Vector<2, i32> operator*(i32 const& lhs, Vector<2, i32> const& rhs) {
		return Vector<2, i32>{ lhs* rhs.x, lhs* rhs.y };
	}

	// Vector<2, u32> has no out-of-line definitions; its members are declared for use
	// at call sites that instantiate them.
	using Vec2UInt = Vector<2, u32>;
	template<>
	struct Vector<2, u32> {
		using ValueType = u32;

		u32 x;
		u32 y;

		[[nodiscard]] constexpr Vector<3, u32> AsVec3(u32 zValue = 0) const;
		[[nodiscard]] constexpr Vector<4, u32> AsVec4(u32 zValue = 0, u32 wValue = 0) const;

		[[nodiscard]] f32 Magnitude() const;
		[[nodiscard]] constexpr f32 MagnitudeSqrd() const;
		[[nodiscard]] Vector<2, f32> GetNormalized() const;

		[[nodiscard]] constexpr u32* Data();
		[[nodiscard]] constexpr u32 const* Data() const;

		[[nodiscard]] static constexpr u32 Dot(Vector const& lhs, Vector const& rhs);

		[[nodiscard]] static constexpr Vector SingleValue(u32 const& input);
		[[nodiscard]] static constexpr Vector Zero();
		[[nodiscard]] static constexpr Vector One();
		[[nodiscard]] static constexpr Vector Up();
		[[nodiscard]] static constexpr Vector Right();

		constexpr Vector& operator+=(Vector const& rhs);
		constexpr Vector& operator-=(Vector const& rhs);
		constexpr Vector& operator*=(u32 const& rhs);
		[[nodiscard]] constexpr Vector operator+(Vector const& rhs) const;
		[[nodiscard]] constexpr Vector operator-(Vector const& rhs) const;
		[[nodiscard]] constexpr Vector operator-() const;
		[[nodiscard]] constexpr bool operator==(Vector const& rhs) const;
		[[nodiscard]] constexpr bool operator!=(Vector const& rhs) const;
	};

	// The three f32 specializations reference each other (AsVec2/AsVec3/AsVec4), so all
	// of their struct bodies are declared before any out-of-line definitions.
	using Vec2 = Vector<2, f32>;
	template<>
	struct Vector<2, f32> {
		using ValueType = f32;

		f32 x;
		f32 y;

		[[nodiscard]] constexpr Vector<3, f32> AsVec3(f32 zValue = 0.f) const noexcept;
		//[[nodiscard]] constexpr explicit operator Vector<3, f32>() const noexcept;
		[[nodiscard]] constexpr Vector<4, f32> AsVec4(f32 zValue = 0.f, f32 wValue = 0.f) const noexcept;

		[[nodiscard]] f32 Magnitude() const;
		[[nodiscard]] constexpr f32 MagnitudeSqrd() const noexcept;

		[[nodiscard]] Vector GetNormalized() const;
		void Normalize();

		// Returns this vector rotated 90 degrees around the Z axis.
		// True means counterclockwise, false is clockwise.
		[[nodiscard]] constexpr Vector GetRotated90(bool counterClock) const noexcept;
		// Returns this vector rotated radians amount around the Z axis.
		[[nodiscard]] Vector GetRotated(f32 radians) const noexcept;

		[[nodiscard]] constexpr f32* Data() noexcept;
		[[nodiscard]] constexpr f32 const* Data() const noexcept;

		[[nodiscard]] static constexpr f32 Dot(Vector const& a, Vector const& b) noexcept;
		// Result in radians.
		// Input must be normalized!
		[[nodiscard]] static f32 Angle(Vector const& a, Vector const& b) noexcept;
		// Result in radians.
		// Input must be normalized!
		[[nodiscard]] static f32 SignedAngle(Vector const& a, Vector const& b) noexcept;

		[[nodiscard]] static constexpr Vector SingleValue(f32 input) noexcept;
		[[nodiscard]] static constexpr Vector Zero() noexcept;
		[[nodiscard]] static constexpr Vector One() noexcept;
		[[nodiscard]] static constexpr Vector Up() noexcept;
		[[nodiscard]] static constexpr Vector Down() noexcept;
		[[nodiscard]] static constexpr Vector Left() noexcept;
		[[nodiscard]] static constexpr Vector Right() noexcept;

		[[nodiscard]] constexpr f32& operator[](uSize i) noexcept;
		[[nodiscard]] constexpr f32 operator[](uSize i) const noexcept;
		constexpr Vector& operator+=(Vector const& rhs) noexcept;
		constexpr Vector& operator-=(Vector const& rhs) noexcept;
		constexpr Vector& operator*=(f32 rhs) noexcept;
		[[nodiscard]] constexpr Vector operator+(Vector const& rhs) const noexcept;
		[[nodiscard]] constexpr Vector operator-(Vector const& rhs) const noexcept;
		[[nodiscard]] constexpr Vector operator-() const noexcept;
		[[nodiscard]] constexpr bool operator==(Vector const& rhs) const noexcept;
		[[nodiscard]] constexpr bool operator!=(Vector const& rhs) const noexcept;
	};

	static_assert(sizeof(Vector<2, f32>) == sizeof(f32[2]), "Size of Vec2 is not as expected.");
	static_assert(alignof(Vector<2, f32>) == alignof(f32[2]), "Alignment of Vec2 is not as expected.");

	using Vec3 = Vector<3, f32>;
	using Vec3Int = Vector<3, i32>;
	template<>
	struct Vector<3, f32> {
		f32 x;
		f32 y;
		f32 z;

		[[nodiscard]] constexpr Vector<2, f32> AsVec2() const;
		[[nodiscard]] constexpr Vector<4, f32> AsVec4(f32 const& wValue = f32()) const;

		[[nodiscard]] f32 Magnitude() const;
		[[nodiscard]] constexpr f32 MagnitudeSqrd() const noexcept;
		[[nodiscard]] Vector GetNormalized() const;
		void Normalize();

		[[nodiscard]] constexpr f32* Data() noexcept;
		[[nodiscard]] constexpr f32 const* Data() const noexcept;

		[[nodiscard]] static constexpr Vector Cross(Vector const& lhs, Vector const& rhs) noexcept;
		[[nodiscard]] static constexpr f32 Dot(Vector const& lhs, Vector const& rhs) noexcept;

		[[nodiscard]] static constexpr Vector SingleValue(f32 const& input) noexcept;
		[[nodiscard]] static constexpr Vector Zero() noexcept;
		[[nodiscard]] static constexpr Vector One() noexcept;
		[[nodiscard]] static constexpr Vector Up() noexcept;
		[[nodiscard]] static constexpr Vector Down() noexcept;
		[[nodiscard]] static constexpr Vector Left() noexcept;
		[[nodiscard]] static constexpr Vector Right() noexcept;
		[[nodiscard]] static constexpr Vector Forward() noexcept;
		[[nodiscard]] static constexpr Vector Back() noexcept;

		[[nodiscard]] constexpr f32& operator[](uSize index) noexcept;
		[[nodiscard]] constexpr f32 operator[](uSize index) const noexcept;
		constexpr Vector& operator+=(Vector const& rhs) noexcept;
		constexpr Vector& operator-=(Vector const& rhs) noexcept;
		constexpr Vector& operator*=(f32 const& rhs) noexcept;
		[[nodiscard]] constexpr Vector operator+(Vector const& rhs) const noexcept;
		[[nodiscard]] constexpr Vector operator-(Vector const& rhs) const noexcept;
		[[nodiscard]] constexpr Vector operator-() const noexcept;
		[[nodiscard]] constexpr bool operator==(Vector const& rhs) const noexcept;
		[[nodiscard]] constexpr bool operator!=(Vector const& rhs) const noexcept;
	};

	[[nodiscard]] constexpr Vector<3, f32> Cross(Vector<3, f32> const& lhs, Vector<3, f32> const& rhs) noexcept;
	[[nodiscard]] constexpr f32 Dot(Vector<3, f32> const& lhs, Vector<3, f32> const& rhs) noexcept;

	static_assert(sizeof(Vector<3, f32>) == sizeof(f32[3]), "Size of Vec3 is not as expected.");
	static_assert(alignof(Vector<3, f32>) == alignof(f32[3]), "Alignment of Vec3 is not as expected.");

	using Vec4 = Vector<4, f32>;
	using Vec4Int = Vector<4, i32>;
	template<>
	struct Vector<4, f32> {
		f32 x;
		f32 y;
		f32 z;
		f32 w;

		[[nodiscard]] constexpr Vector<2, f32> AsVec2() const noexcept;
		[[nodiscard]] explicit constexpr operator Vector<2, f32>() const noexcept;
		[[nodiscard]] constexpr Vector<3, f32> AsVec3() const noexcept;
		[[nodiscard]] explicit constexpr operator Vector<3, f32>() const noexcept;

		[[nodiscard]] constexpr f32& At(uSize index) noexcept;
		[[nodiscard]] constexpr f32 At(uSize index) const noexcept;

		[[nodiscard]] static constexpr f32 Dot(Vector const& lhs, Vector const& rhs) noexcept;

		[[nodiscard]] constexpr f32* Data() noexcept;
		[[nodiscard]] constexpr f32 const* Data() const noexcept;

		[[nodiscard]] f32 Magnitude() const;
		[[nodiscard]] constexpr f32 MagnitudeSqrd() const noexcept;
		[[nodiscard]] Vector GetNormalized() const;

		[[nodiscard]] Vector ClampAll(f32 min, f32 max) const noexcept;

		void Normalize();

		[[nodiscard]] constexpr f32* begin() noexcept;
		[[nodiscard]] constexpr f32 const* begin() const noexcept;
		[[nodiscard]] constexpr f32* end() noexcept;
		[[nodiscard]] constexpr f32 const* end() const noexcept;

		[[nodiscard]] static constexpr Vector SingleValue(f32 input) noexcept;
		[[nodiscard]] static constexpr Vector Zero() noexcept;
		[[nodiscard]] static constexpr Vector One() noexcept;

		[[nodiscard]] constexpr f32& operator[](uSize index) noexcept;
		[[nodiscard]] constexpr f32 operator[](uSize index) const noexcept;
		constexpr Vector& operator+=(Vector const& rhs) noexcept;
		constexpr Vector& operator-=(Vector const& rhs) noexcept;
		constexpr Vector& operator*=(f32 rhs) noexcept;
		[[nodiscard]] constexpr Vector operator+(Vector const& rhs) const noexcept;
		[[nodiscard]] constexpr Vector operator-(Vector const& rhs) const noexcept;
		[[nodiscard]] constexpr Vector operator-() const noexcept;
		[[nodiscard]] constexpr bool operator==(Vector const& rhs) const noexcept;
		[[nodiscard]] constexpr bool operator!=(Vector const& rhs) const noexcept;
	};

	static_assert(sizeof(Vector<4, f32>) == sizeof(f32[4]), "Size of Vec4 is not as expected.");
	static_assert(alignof(Vector<4, f32>) == alignof(f32[4]), "Alignment of Vec4 is not as expected.");

	// ----- Vector<2, f32> definitions -----

	[[nodiscard]] constexpr Vector<2, f32> operator*(Vector<2, f32> const& lhs, f32 rhs) noexcept {
		return Vector<2, f32>{ lhs.x * rhs, lhs.y * rhs };
	}

	[[nodiscard]] constexpr Vector<2, f32> operator*(f32 lhs, Vector<2, f32> const& rhs) noexcept {
		return Vector<2, f32>{ lhs * rhs.x, lhs * rhs.y };
	}

	constexpr Vector<3, f32> Vector<2, f32>::AsVec3(f32 zValue) const noexcept {
		return Vector<3, f32>{ x, y, zValue };
	}
	/*
	constexpr Vector<2, f32>::operator Vector<3, f32>() const noexcept
	{
		return Vector<3, f32>{ x, y, 0.f };
	}
	*/

	constexpr Vector<4, f32> Vector<2, f32>::AsVec4(f32 zValue, f32 wValue) const noexcept {
		return Vector<4, f32>{ x, y, zValue, wValue };
	}

	constexpr f32 Vector<2, f32>::MagnitudeSqrd() const noexcept {
		return (x * x) + (y * y);
	}

	constexpr Vector<2, f32> Vector<2, f32>::GetRotated90(bool counterClock) const noexcept {
		// Multiply the vector with -1 if input is false
		// Didgy: I tried doing this branchless. Might be stupid, idk
		return Vector{ -y, x } * (f32)(((i8)counterClock * 2) - 1);
	}

	constexpr f32* Vector<2, f32>::Data() noexcept {
		return &x;
	}

	constexpr f32 const* Vector<2, f32>::Data() const noexcept {
		return &x;
	}

	constexpr f32 Vector<2, f32>::Dot(Vector const& lhs, Vector const& rhs) noexcept {
		return (lhs.x * rhs.x) + (lhs.y * rhs.y);
	}

	constexpr Vector<2, f32> Vector<2, f32>::SingleValue(f32 input) noexcept {
		return Vector{ input, input };
	}

	constexpr Vector<2, f32> Vector<2, f32>::Zero() noexcept {
		return Vector{ f32(0), f32(0) };
	}

	constexpr Vector<2, f32> Vector<2, f32>::One() noexcept {
		return Vector{ f32(1), f32(1) };
	}

	constexpr Vector<2, f32> Vector<2, f32>::Up() noexcept {
		return Vector{ f32(0), f32(1) };
	}

	constexpr Vector<2, f32> Vector<2, f32>::Down() noexcept {
		return Vector{ f32(0), f32(-1) };
	}

	constexpr Vector<2, f32> Vector<2, f32>::Left() noexcept {
		return Vector{ f32(-1), f32(0) };
	}

	constexpr Vector<2, f32> Vector<2, f32>::Right() noexcept {
		return Vector{ f32(1), f32(0) };
	}

	constexpr f32& Vector<2, f32>::operator[](uSize index) noexcept {
		switch (index) {
		case 0:
			return x;
		case 1:
			return y;
		default:
			DENGINE_IMPL_MATH_UNREACHABLE();
		}
	}

	constexpr f32 Vector<2, f32>::operator[](uSize index) const noexcept {
		switch (index) {
		case 0:
			return x;
		case 1:
			return y;
		default:
			DENGINE_IMPL_MATH_UNREACHABLE();
		}
	}

	constexpr Vector<2, f32>& Vector<2, f32>::operator+=(Vector const& rhs) noexcept {
		x += rhs.x;
		y += rhs.y;
		return *this;
	}

	constexpr Vector<2, f32>& Vector<2, f32>::operator-=(Vector const& rhs) noexcept {
		x -= rhs.x;
		y -= rhs.y;
		return *this;
	}

	constexpr Vector<2, f32>& Vector<2, f32>::operator*=(f32 rhs) noexcept {
		x *= rhs;
		y *= rhs;
		return *this;
	}

	constexpr Vector<2, f32> Vector<2, f32>::operator+(Vector const& rhs) const noexcept {
		return Vector{ x + rhs.x, y + rhs.y };
	}

	constexpr Vector<2, f32> Vector<2, f32>::operator-(Vector const& rhs) const noexcept {
		return Vector{ x - rhs.x, y - rhs.y };
	}

	constexpr Vector<2, f32> Vector<2, f32>::operator-() const noexcept {
		return Vector{ -x, -y };
	}

	constexpr bool Vector<2, f32>::operator==(Vector const& rhs) const noexcept {
		return x == rhs.x && y == rhs.y;
	}

	constexpr bool Vector<2, f32>::operator!=(Vector const& rhs) const noexcept {
		return x != rhs.x || y != rhs.y;
	}

	// ----- Vector<3, f32> definitions -----

	constexpr Vector<2, f32> Vector<3, f32>::AsVec2() const {
		return Vector<2, f32>{ x, y };
	}

	constexpr Vector<4, f32> Vector<3, f32>::AsVec4(f32 const& wValue) const {
		return Vector<4, f32>{ x, y, z, wValue };
	}

	constexpr f32 Vector<3, f32>::MagnitudeSqrd() const noexcept {
		return (x * x) + (y * y) + (z * z);
	}

	constexpr f32* Vector<3, f32>::Data() noexcept {
		return &x;
	}

	constexpr f32 const* Vector<3, f32>::Data() const noexcept {
		return &x;
	}

	constexpr Vector<3, f32> Vector<3, f32>::Cross(Vector const& lhs, Vector const& rhs) noexcept {
		return { lhs.y * rhs.z - lhs.z * rhs.y, lhs.z * rhs.x - lhs.x * rhs.z, lhs.x * rhs.y - lhs.y * rhs.x };
	}

	constexpr f32 Vector<3, f32>::Dot(Vector const& lhs, Vector const& rhs) noexcept {
		return (lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z * rhs.z);
	}

	constexpr Vector<3, f32> Vector<3, f32>::SingleValue(f32 const& input) noexcept {
		return { input, input, input };
	}
	constexpr Vector<3, f32> Vector<3, f32>::Zero() noexcept {
		return Vector{ 0.f, 0.f, 0.f };
	}
	constexpr Vector<3, f32> Vector<3, f32>::One() noexcept {
		return { 1.f, 1.f, 1.f };
	}
	constexpr Vector<3, f32> Vector<3, f32>::Up() noexcept {
		return { 0.f, 1.f, 0.f };
	}
	constexpr Vector<3, f32> Vector<3, f32>::Down() noexcept {
		return { 0.f, -1.f, 0 };
	}
	constexpr Vector<3, f32> Vector<3, f32>::Left() noexcept {
		return { -1.f, 1.f, 0.f };
	}
	constexpr Vector<3, f32> Vector<3, f32>::Right() noexcept {
		return { 1.f, 0.f, 0.f };
	}
	constexpr Vector<3, f32> Vector<3, f32>::Forward() noexcept {
		return { 0.f, 0.f, 1.f };
	}
	constexpr Vector<3, f32> Vector<3, f32>::Back() noexcept {
		return { 0.f, 0.f, -1.f };
	}

	constexpr f32& Vector<3, f32>::operator[](uSize index) noexcept {
		switch (index) {
		case 0:
			return x;
		case 1:
			return y;
		case 2:
			return z;
		default:
			DENGINE_IMPL_MATH_UNREACHABLE();
		}
	}

	constexpr f32 Vector<3, f32>::operator[](uSize index) const noexcept {
		switch (index) {
		case 0:
			return x;
		case 1:
			return y;
		case 2:
			return z;
		default:
			DENGINE_IMPL_MATH_UNREACHABLE();
		}
	}

	constexpr Vector<3, f32>& Vector<3, f32>::operator+=(Vector const& rhs) noexcept {
		x += rhs.x;
		y += rhs.y;
		z += rhs.z;
		return *this;
	}
	constexpr Vector<3, f32>& Vector<3, f32>::operator-=(Vector const& rhs) noexcept {
		x -= rhs.x;
		y -= rhs.y;
		z -= rhs.z;
		return *this;
	}
	constexpr Vector<3, f32>& Vector<3, f32>::operator*=(f32 const& rhs) noexcept {
		x *= rhs;
		y *= rhs;
		z *= rhs;
		return *this;
	}
	constexpr Vector<3, f32> Vector<3, f32>::operator+(Vector const& rhs) const noexcept {
		return Vector{ x + rhs.x, y + rhs.y, z + rhs.z };
	}
	constexpr Vector<3, f32> Vector<3, f32>::operator-(Vector const& rhs) const noexcept {
		return Vector{ x - rhs.x, y - rhs.y, z - rhs.z };
	}
	constexpr Vector<3, f32> Vector<3, f32>::operator-() const noexcept {
		return Vector{ -x, -y, -z };
	}
	constexpr bool Vector<3, f32>::operator==(Vector const& rhs) const noexcept {
		return x == rhs.x && y == rhs.y && z == rhs.z;
	}
	constexpr bool Vector<3, f32>::operator!=(Vector const& rhs) const noexcept {
		return x != rhs.x || y != rhs.y || z != rhs.z;
	}

	constexpr Vector<3, f32> operator*(Vector<3, f32> const& lhs, f32 const& rhs) noexcept {
		return Vector<3, f32>{ lhs.x * rhs, lhs.y * rhs, lhs.z * rhs };
	}

	constexpr Vector<3, f32> operator*(f32 const& lhs, Vector<3, f32> const& rhs) noexcept {
		return Vector<3, f32>{ lhs * rhs.x, lhs * rhs.y, lhs * rhs.z };
	}

	constexpr Vector<3, f32> Cross(Vector<3, f32> const& lhs, Vector<3, f32> const& rhs) noexcept {
		return Vector<3, f32>::Cross(lhs, rhs);
	}

	constexpr f32 Dot(Vector<3, f32> const& lhs, Vector<3, f32> const& rhs) noexcept {
		return Vector<3, f32>::Dot(lhs, rhs);
	}

	// ----- Vector<4, f32> definitions -----

	constexpr Vector<2, f32> Vector<4, f32>::AsVec2() const noexcept {
		return Vector<2, f32>{ x, y };
	}

	inline constexpr Vector<4, f32>::operator Vector<2, f32>() const noexcept {
		return AsVec2();
	}

	constexpr Vector<3, f32> Vector<4, f32>::AsVec3() const noexcept {
		return Vector<3, f32>{ x, y, z };
	}

	inline constexpr Vector<4, f32>::operator Vector<3, f32>() const noexcept {
		return AsVec3();
	}

	constexpr f32& Vector<4, f32>::At(uSize index) noexcept {
		switch (index) {
		case 0:
			return x;
		case 1:
			return y;
		case 2:
			return z;
		case 3:
			return w;
		default:
			DENGINE_IMPL_MATH_UNREACHABLE();
		}
	}

	constexpr f32 Vector<4, f32>::At(uSize index) const noexcept {
		switch (index) {
		case 0:
			return x;
		case 1:
			return y;
		case 2:
			return z;
		case 3:
			return w;
		default:
			DENGINE_IMPL_MATH_UNREACHABLE();
		}
	}

	constexpr f32 Vector<4, f32>::Dot(Vector const& lhs, Vector const& rhs) noexcept {
		return (lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z * rhs.z) + (lhs.w * rhs.w);
	}

	constexpr f32* Vector<4, f32>::Data() noexcept { return &x; }
	constexpr f32 const* Vector<4, f32>::Data() const noexcept { return &x; }

	constexpr f32 Vector<4, f32>::MagnitudeSqrd() const noexcept {
		return (x * x) + (y * y) + (z * z) + (w * w);
	}

	inline Vector<4, f32> Vector<4, f32>::ClampAll(f32 min, f32 max) const noexcept {
		auto returnVal = *this;
		for (auto& item : returnVal) {
			if (item < min) {
				item = min;
			}
			if (item > max) {
				item = max;
			}
		}
		return returnVal;
	}

	[[nodiscard]] constexpr f32* Vector<4, f32>::begin() noexcept {
		return &x;
	}

	[[nodiscard]] constexpr f32 const* Vector<4, f32>::begin() const noexcept {
		return &x;
	}

	[[nodiscard]] constexpr f32* Vector<4, f32>::end() noexcept {
		return &x + 4;
	}

	[[nodiscard]] constexpr f32 const* Vector<4, f32>::end() const noexcept {
		return &x + 4;
	}

	constexpr Vector<4, f32> Vector<4, f32>::SingleValue(f32 input) noexcept {
		return { input, input, input, input };
	}
	constexpr Vector<4, f32> Vector<4, f32>::Zero() noexcept {
		return Vector{ 0.f, 0.f, 0.f, 0.f };
	}
	constexpr Vector<4, f32> Vector<4, f32>::One() noexcept {
		return { 1.f, 1.f, 1.f, 1.f };
	}

	constexpr f32& Vector<4, f32>::operator[](uSize index) noexcept {
		switch (index) {
		case 0:
			return x;
		case 1:
			return y;
		case 2:
			return z;
		case 3:
			return w;
		default:
			DENGINE_IMPL_MATH_UNREACHABLE();
		}
	}

	constexpr f32 Vector<4, f32>::operator[](uSize index) const noexcept {
		switch (index) {
		case 0:
			return x;
		case 1:
			return y;
		case 2:
			return z;
		case 3:
			return w;
		default:
			DENGINE_IMPL_MATH_UNREACHABLE();
		}
	}

	constexpr Vector<4, f32>& Vector<4, f32>::operator+=(Vector const& rhs) noexcept {
		x += rhs.x;
		y += rhs.y;
		z += rhs.z;
		w += rhs.w;
		return *this;
	}

	constexpr Vector<4, f32>& Vector<4, f32>::operator-=(Vector const& rhs) noexcept {
		x -= rhs.x;
		y -= rhs.y;
		z -= rhs.z;
		w -= rhs.w;
		return *this;
	}

	constexpr Vector<4, f32>& Vector<4, f32>::operator*=(f32 rhs) noexcept {
		x *= rhs;
		y *= rhs;
		z *= rhs;
		return *this;
	}

	constexpr Vector<4, f32> Vector<4, f32>::operator+(Vector const& rhs) const noexcept {
		return { x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w };
	}

	constexpr Vector<4, f32> Vector<4, f32>::operator-(Vector const& rhs) const noexcept {
		return { x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w };
	}

	constexpr Vector<4, f32> Vector<4, f32>::operator-() const noexcept {
		return { -x, -y, -z, -w };
	}

	constexpr bool Vector<4, f32>::operator==(Vector const& rhs) const noexcept {
		return x == rhs.x && y == rhs.y && z == rhs.z && rhs.w;
	}

	constexpr bool Vector<4, f32>::operator!=(Vector const& rhs) const noexcept {
		return x != rhs.x || y != rhs.y || z != rhs.z || w != rhs.w;
	}

	[[nodiscard]] constexpr Vector<4, f32> operator*(Vector<4, f32> const& lhs, f32 rhs) noexcept {
		return { lhs.x * rhs, lhs.y * rhs, lhs.z * rhs, lhs.w * rhs };
	}

	[[nodiscard]] constexpr Vector<4, f32> operator*(f32 lhs, Vector<4, f32> const& rhs) noexcept {
		return { lhs * rhs.x, lhs * rhs.y, lhs * rhs.z, lhs * rhs.w };
	}
}
