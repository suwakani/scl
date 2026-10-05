#pragma once

#include <initializer_list>
#include <numeric>
#include <string>

#include <scl/math/fixed.hpp>

namespace scl {
namespace math {

// --------------------------------------------------------------------------@/
// vector class
// --------------------------------------------------------------------------@/
template<typename T, std::size_t N = 4> class Vec {
	public:
		std::array<T,N> mData;
		constexpr Vec() : mData{} {}
		template<typename ...T0> constexpr Vec(T0... v) : mData{v...} {}

		// access methods -------------------------------@/
		constexpr auto operator[](const std::size_t idx) -> T& {
			return mData[idx];
		}
		constexpr auto operator[](const std::size_t idx) const -> const T& {
			return mData[idx];
		}
		constexpr auto at(const std::size_t idx) -> T& {
			return mData.at(idx);
		}
		constexpr auto at(const std::size_t idx) const -> const T& {
			return mData.at(idx);
		}

		constexpr auto x() -> T& { return mData[0]; }
		constexpr auto y() -> T& { return mData[1]; }
		constexpr auto z() -> T& { static_assert(N > 2); return mData[2]; }

		// misc fns -------------------------------------@/
		constexpr auto size() const -> std::size_t {
			return N;
		}
		constexpr auto sum() const -> T {
			T res = 0;
			for(std::size_t i=0; i<N; i++) {
				res += mData[i];
			}
			return res;
		}
		constexpr auto lerp(const Vec<T,N>& other,const T alpha) const -> Vec<T,N> {
			Vec<T,N> res;
			for(std::size_t i=0; i<N; i++) {
				res[i] = mData[i] + (other[i]-mData[i]) * alpha;
			}
			return res;
		}
		constexpr auto dot(const Vec<T,N>& other) const -> T {
			T res = 0;
			for(std::size_t i=0; i<N; i++) {
				res += mData[i] * other[i];
			}
			return res;
		}
		constexpr auto magnitude() const -> T {
			T res = 0;
			for(std::size_t i=0; i<N; i++) {
				res += mData[i] * mData[i];
			}

			// specialization for fixed-point types ------@/
			if constexpr (std::is_same<T,Fxi>::value) {
				return res.sqrt();
			} else {
				return std::sqrt(res);
			}
			return res;
		}
		constexpr auto distance(const Vec<T,N>& other) const ->  T {
			return (other - *this).magnitude();
		}
		constexpr auto unit() const -> Vec<T,N> {
			return *this / magnitude();
		}

		// arith operators ------------------------------@/
		constexpr auto operator+(const Vec<T,N>& other) const -> Vec<T,N> {
			Vec<T,N> res;
			for(std::size_t i=0; i<N; i++) {
				res[i] = mData[i] + other[i];
			}
			return res;
		}
		constexpr auto operator-() const -> Vec<T,N> {
			Vec<T,N> res;
			for(std::size_t i=0; i<N; i++) {
				res[i] = -mData[i];
			}
			return res;
		}
		constexpr auto operator-(const Vec<T,N>& other) const -> Vec<T,N> {
			Vec<T,N> res;
			for(std::size_t i=0; i<N; i++) {
				res[i] = mData[i] - other[i];
			}
			return res;
		}
		constexpr auto operator/(const Vec<T,N>& other) const -> Vec<T,N> {
			Vec<T,N> res;
			for(std::size_t i=0; i<N; i++) {
				res[i] = mData[i] / other[i];
			}
			return res;
		}
		constexpr auto operator/(const T opan) const -> Vec<T,N> {
			Vec<T,N> res;
			for(std::size_t i=0; i<N; i++) {
				res[i] = mData[i] / opan;
			}
			return res;
		}
		constexpr auto operator*(const Vec<T,N>& other) const -> Vec<T,N> {
			Vec<T,N> res;
			for(std::size_t i=0; i<N; i++) {
				res[i] = mData[i] * other[i];
			}
			return res;
		}
		constexpr auto operator*(const T opan) const -> Vec<T,N> {
			Vec<T,N> res;
			for(std::size_t i=0; i<N; i++) {
				res[i] = mData[i] * opan;
			}
			return res;
		}
		constexpr auto operator*(const float other) const -> Vec<T,N> {
			return *this * other;
		}

		// assignment operators -------------------------@/
		constexpr auto operator=(const Vec<T,N>& other) -> Vec<T,N>& = default;
		
		auto to_str() const -> std::string {
			std::string str = "[ ";
			for(std::size_t i=0; i<N; i++) {
				if constexpr(std::is_same<T,Fxi>::value) {
					str += std::to_string(mData[i].real());
				} else {
					str += std::to_string(mData[i]);
				}

				// comma, but not at end ----------------@/
				if(i != N-1) {
					str += ", ";
				}
			}
			str += " ]";
			return str;
		}
};

typedef Vec<Fxi,2> Vec2fxi;
typedef Vec<float,2> Vec2f;
typedef Vec<int32_t,2> Vec2i;

typedef Vec<Fxi,3> Vec3fxi;
typedef Vec<float,3> Vec3f;
typedef Vec<int32_t,3> Vec3i;

typedef Vec<Fxi,4> Vec4fxi;
typedef Vec<float> Vec4f;
typedef Vec<int32_t> Vec4i;

typedef Vec4f Vecf;
typedef Vec4i Veci;
typedef Vec4fxi Vecfxi;

}; // namespace math
}; // namespace scl

