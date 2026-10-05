#pragma once

#include <initializer_list>
#include <numbers>
#include <format>

#include <cmath>

namespace scl {
namespace math {

// --------------------------------------------------------------------------@/
// fixed-point
// --------------------------------------------------------------------------@/
uint32_t sqrti(uint64_t a_nInput) {
	// we just changed the types to uint64_t and changed one from "1uL << 30".
	uint64_t op  = a_nInput;
	uint64_t res = 0;
	uint64_t one = 1ULL << 62; // The second-to-top bit is set: use 1u << 14 for uint16_t type; use 1uL<<30 for uint32_t type

	// "one" starts at the highest power of four <= than the argument.
	while (one > op) {
		one >>= 2;
	}

	while (one != 0)
	{
		if (op >= res + one) {
			op = op - (res + one);
			res = res +  2 * one;
		}
		res >>= 1;
		one >>= 2;
	}

	// round to account for error -----------------------@/
	if(op > res) {
		res++;
	}

	return res;
}

template<std::size_t S = 12> class QFxi {
	public:
		using value_type = int32_t;
		using real_type = double;
		static constexpr size_t Shift = S;
		static constexpr int One = (1 << Shift);
		static constexpr size_t MaskFrac = One - 1;
		static constexpr size_t MaskWhole = ~MaskFrac;
		static constexpr size_t NumBits = 8 * sizeof(value_type);
		static constexpr int64_t Max = 1LL << (NumBits-1-Shift);
	public:
		value_type mData;

		constexpr QFxi() : mData(0) {}
		constexpr QFxi(value_type v, int shift) : mData(v >> shift) {}
		constexpr QFxi(int v) : mData(v * One) {}
		constexpr QFxi(const float v) : mData(v * One) {}
		constexpr QFxi(const double v) : mData(v * One) {}

		// conversion methods ---------------------------@/
		constexpr auto raw() const -> value_type {
			return mData;
		}
		constexpr auto partFrac() const -> value_type {
			return mData & MaskFrac;
		}
		constexpr auto partWhole() const -> value_type {
			// NOTE: seems to be faster than "mData / One"?
			return mData >> Shift;
		}
		constexpr auto round() const -> value_type {
			return (*this + 0.5).partWhole();
		}
		constexpr auto real() const -> real_type {
			return static_cast<real_type>(mData) / One;
		}

		// misc functions -------------------------------@/
		constexpr auto lerp(const QFxi<S>& target, const QFxi<S>& alpha) const -> QFxi<S> {
			return *this + (target-*this) * alpha;
		}
		constexpr auto bezier(const QFxi<S>& pB, const QFxi<S>& pC, const QFxi<S>& alpha) const -> QFxi<S> {
			return lerp(pB,alpha).lerp(pB.lerp(pC,alpha),alpha);
		}
		constexpr auto sqrt_old() const -> QFxi<S> {
			// http://stackoverflow.com/questions/1100090/looking-for-an-efficient-integer-square-root-algorithm-for-arm-thumb2
			// https://github.com/melonDS-emu/melonDS/blob/906e9ebb27da8c6a715cd7abab4abfe8a8d29427/src/NDS.cpp#L1876
			auto res = std::sqrt( real() );
			return QFxi<S>(res);
		}
		constexpr auto sqrt() const -> QFxi<S> {
			// NOTE:
			// -  we have to sqrt in a slightly different way, because of the
			//    fact that, for instance, if using .8 fixed representation,
			//    sqrt(256) should result in 256, and not 16.
			// -  to get the proper value, we could either:
			//    *  shift left before, using the num of fractional bits
			//    *  shift right after, using HALF amount of fractional bits
			// -  the first choice is best, but COULD be prone to overflow.
			//    fortunately, it's fine if we use 64-bit ints.
			auto input = static_cast<int64_t>(raw()) << S;
			auto res = scl::math::sqrti(input);
			return QFxi<S>(res,0);
		}
		constexpr auto pow(const QFxi<S>& exp) const -> QFxi<S> {
			auto n = std::pow(real(),exp.real());
			return QFxi<S>(n);
		}
		constexpr auto abs() const -> QFxi<S> {
			return QFxi<S>(std::abs(raw()),0);
		}

		// arithemetic operators ------------------------@/
		constexpr auto operator<<(const std::size_t amount) const -> QFxi<S> {
			return mData << amount;
		}
		constexpr auto operator>>(const std::size_t amount) const -> QFxi<S> {
			return mData >> amount;
		}
		constexpr auto operator+(const QFxi<S>& other) const -> QFxi<S> {
			return QFxi<S>(mData + other.raw(),0);
		}
		constexpr auto operator-(const QFxi<S>& other) const -> QFxi<S> {
			return QFxi<S>(mData - other.raw(),0);
		}
		constexpr auto operator-() const -> QFxi<S> {
			return QFxi<S>(-mData,0);
		}
		constexpr auto operator*(const QFxi<S>& opan) const -> QFxi<S> {
			int64_t val = ((int64_t)mData) * ((int64_t)opan.raw()) >> S;
			return QFxi<S>(val,0);
		}
		constexpr auto operator/(const QFxi<S>& opan) const -> QFxi<S> {
			int64_t val = (((int64_t)mData) << S) / ((int64_t)opan.raw());
			return QFxi<S>(val,0);
		}

		// comparison operators -------------------------@/
		constexpr auto operator>(const QFxi<S>& opan) const -> bool {
			return raw() > opan.raw();
		}
		constexpr auto operator<(const QFxi<S>& opan) const -> bool {
			return raw() < opan.raw();
		}
		constexpr auto operator>=(const QFxi<S>& opan) const -> bool {
			return raw() >= opan.raw();
		}
		constexpr auto operator<=(const QFxi<S>& opan) const -> bool {
			return raw() <= opan.raw();
		}

		// assignment operators -------------------------@/
		constexpr auto operator+=(const QFxi<S>& other) -> QFxi<S>& {
			*this = *this + other;
			return *this;
		}
		constexpr auto operator-=(const QFxi<S>& other) -> QFxi<S>& {
			*this = *this - other;
			return *this;
		}
		constexpr auto operator*=(const QFxi<S>& other) -> QFxi<S>& {
			*this = *this * other;
			return *this;
		}
		constexpr auto operator/=(const QFxi<S>& other) -> QFxi<S>& {
			*this = *this / other;
			return *this;
		}

		// static fns -----------------------------------@/
		static constexpr auto cosb(int angle, const QFxi<S> len = 1) -> QFxi<S> {
			angle &= 0xFFFF;
			auto angle_f = static_cast<real_type>(angle) * (std::numbers::pi_v<real_type> / 32768.0);
			return QFxi<S>(std::cos(angle_f)) * len;
		}
		static constexpr auto sinb(int angle, const QFxi<S> len = 1) -> QFxi<S> {
			return cosb(angle + 0x4000,len);
		}

		// misc fns -------------------------------------@/
		auto to_str() const -> std::string {
			if constexpr (S == 8) {
				return std::format("{0:d}.{1:02X}h",partWhole(),partFrac());
			} else if constexpr (S >= 9 || S <= 12) {
				return std::format("{0:d}.{1:03X}h",partWhole(),partFrac());
			} else if constexpr (S >= 13 || S <= 16) {
				return std::format("{0:d}.{1:04X}h",partWhole(),partFrac());
			} else {
				return std::format("{0:d}.{1:06X}h",partWhole(),partFrac());
			}
			/*
			*/
		}
};

template<std::size_t S> constexpr auto operator+(int a, const QFxi<S>& b) { return QFxi<S>(a) + b; }
template<std::size_t S> constexpr auto operator+(double a, const QFxi<S>& b) { return QFxi<S>(a) + b; }
template<std::size_t S> constexpr auto operator-(int a, const QFxi<S>& b) { return QFxi<S>(a) - b; }
template<std::size_t S> constexpr auto operator-(double a, const QFxi<S>& b) { return QFxi<S>(a) - b; }
template<std::size_t S> constexpr auto operator*(int a, const QFxi<S>& b) { return QFxi<S>(a) * b; }
template<std::size_t S> constexpr auto operator*(double a, const QFxi<S>& b) { return QFxi<S>(a) * b; }
template<std::size_t S> constexpr auto operator/(int a, const QFxi<S>& b) { return QFxi<S>(a) / b; }
template<std::size_t S> constexpr auto operator/(double a, const QFxi<S>& b) { return QFxi<S>(a) / b; }

template<std::size_t S> constexpr auto operator<(int a, const QFxi<S>& b) { return QFxi<S>(a) < b; }
template<std::size_t S> constexpr auto operator<(double a, const QFxi<S>& b) { return QFxi<S>(a) < b; }
template<std::size_t S> constexpr auto operator>(int a, const QFxi<S>& b) { return QFxi<S>(a) > b; }
template<std::size_t S> constexpr auto operator>(double a, const QFxi<S>& b) { return QFxi<S>(a) > b; }
template<std::size_t S> constexpr auto operator<=(int a, const QFxi<S>& b) { return QFxi<S>(a) <= b; }
template<std::size_t S> constexpr auto operator<=(double a, const QFxi<S>& b) { return QFxi<S>(a) <= b; }
template<std::size_t S> constexpr auto operator>=(int a, const QFxi<S>& b) { return QFxi<S>(a) >= b; }
template<std::size_t S> constexpr auto operator>=(double a, const QFxi<S>& b) { return QFxi<S>(a) >= b; }

typedef QFxi<> Fxi;

}; // namespace math
}; // namespace scl

