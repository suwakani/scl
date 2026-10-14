#define SCL_USE_ZLIB
#include <scl/container/blob.hpp>
#include <scl/container/pool.hpp>
#include <scl/math/fixed.hpp>
#include <scl/math/vector.hpp>
#include <scl/io/archive.hpp>

#include <ranges>
#include <filesystem>
#include <format>
#include <iostream>

namespace stdfs = std::filesystem;
namespace sclarc = scl::io::archive;
namespace sclpool = scl::pool;
using Fxi = scl::math::QFxi<12>;
using Vec2fxi = scl::math::Vec<Fxi,2>;
using Vec3fxi = scl::math::Vec<Fxi,3>;

namespace sample_objlist {
	class CUnit {
		public:
			Vec3fxi pos;
			int32_t hp;
			int32_t alive;

			std::array<uint8_t,40> work;

			CUnit() {
				std::printf("unit %p: constructed!\n",this);
			}
			~CUnit() {
				std::printf("unit %p: deleted!\n",this);
			}
	};

	auto start() -> void {
		sclpool::Manager<CUnit> objpool(1000);

		// creating and deleting multiple ---------------@/
		std::vector<sclpool::Manager<CUnit>::TypeRef> reftable;

		for(int i=0; i<4; i++) {
			auto objRef = objpool.add_ref();
			reftable.push_back(objRef);
		}
		while(!reftable.empty()) {
			auto objRef = reftable.back();
			objRef.lock();
			objRef.del();
			reftable.pop_back();
		}

		auto objRef_a = objpool.add_ref();
		auto objRef_b = objpool.add_ref();

		std::swap(objRef_a,objRef_b);

		std::cout << std::format("objRef_a: {0}\n", objRef_a.to_str());
		std::cout << std::format("objRef_b: {0}\n", objRef_b.to_str());

		std::cout << std::format("objpool's size: {0}KB(obj) + {1}KB(status)\n",
			(float)(objpool.max_size() * sizeof(CUnit)) / 1024.0,
			(float)(objpool.max_size() * sizeof(sclpool::Status<CUnit>)) / 1024.0
		);
	}
};
namespace sample_fxi {
	auto start() -> void {
		std::cout << std::format("fxi size: S.{0}.{1}\n",Fxi::NumBits-1-Fxi::Shift,Fxi::Shift);
		std::cout << std::format("fxi max:  {0}\n",Fxi::Max);
		std::cout << std::format("fxi min:  {0}\n",Fxi::smallest().real());
		std::printf("fxi sqrt (2)/2: %f (expected: %f)\n",
			(Fxi(2).sqrt() / 2).real(),
			std::sqrtf(2.0)/2
		);

		// check square roots ---------------------------@/
		const std::vector<double> tbl_sqrttest = {
			2,4,3,
			404'003.3290,
			42'023.31997 * 1,
			42'023.31997 * 5,
		};
		auto fxi_testSqrt_old = [](const double input) {
			auto res = Fxi(input).sqrt_old();
			auto res_f = res.real();
			auto res_expected = std::sqrt(input);
			auto error_amt = res_f - res_expected;
			
			std::cout << std::format("fxi sqrt.old ({0:f}): {2:08X}h,{1:f} (expected: {3:f}, error: {4:+.6f})\n",
				input,
				res_f,res.raw(),
				res_expected,error_amt
			);
		};
		auto fxi_testSqrt_new = [](const double input) {
			auto res = Fxi(input).sqrt();
			auto res_f = res.real();
			auto res_expected = std::sqrt(input);
			auto error_amt = res_f - res_expected;
			
			std::cout << std::format("fxi sqrt.new ({0:f}): {2:08X}h,{1:f} (expected: {3:f}, error: {4:+.6f})\n",
				input,
				res_f,res.raw(),
				res_expected,error_amt
			);
		};
		auto fxi_testSqrt = [&](const double input) {
			fxi_testSqrt_old(input);
			fxi_testSqrt_new(input);
			std::puts("");
		};
		for(const auto& num : tbl_sqrttest) {
			fxi_testSqrt(num);
		}

		// check arith ----------------------------------@/
		Fxi val = 0;
		val += 4;  // 4
		val -= 2;  // 2
		val *= 16; // 32
		val /= 3;  // 10.666?
		val = 3 + val;

		Fxi div_x = 5;
		Fxi div_xRes = 1 / div_x;
		Fxi div_60 = Fxi(1.0 / 60.0);
		Fxi printtest = 34290.29138;
		std::printf("fxi res (arith): %f\n",val.real());
		std::cout << std::format("1/x: {0} ({1})\n",div_xRes.real(),div_xRes.to_str());
		std::cout << std::format("num: {0} ({1})\n",printtest.real(),printtest.to_str());
		std::cout << std::format("num: {0} ({1})\n",div_60.real(),1.0 / 60);
		std::cout << std::format("2 ^ (1/6):       {0}\n",Fxi(2).pow(1.0 / 6).real());
		std::cout << std::format("(2 ^ (1/6)) ^ 6: {0}\n",Fxi(2).pow(1.0 / 6).pow(6).real());
		std::cout << std::format("abs(-4): {0}\n", Fxi(-4).abs().real());
		std::cout << std::format("abs(4):  {0}\n", Fxi(4).abs().real());

		std::cout << std::format("1.544 > 1.545:  {0}\n",Fxi(1.544) > Fxi(1.545));
		std::cout << std::format("1.544 < 1.545:  {0}\n",Fxi(1.544) < Fxi(1.545));
		std::cout << std::format("1.544 >= 1.545: {0}\n",Fxi(1.544) >= Fxi(1.545));
		std::cout << std::format("1.544 <= 1.545: {0}\n",Fxi(1.544) <= Fxi(1.545));

		std::cout << std::format("1 < 1.544:  {0}\n", 1 < Fxi(1.544));
		std::cout << std::format("1 > 1.544:  {0}\n", 1 > Fxi(1.544));
		std::cout << std::format("1 <= 1.544: {0}\n",1 <= Fxi(1.544));
		std::cout << std::format("1 >= 1.544: {0}\n",1 >= Fxi(1.544));
		std::cout << std::format("0 == 1.544: {0}\n",0 == Fxi(1.544));
		std::cout << std::format("0 == 1.544: {0}\n",0.0 == Fxi(1.544));

		std::cout << std::format("++2: {0}\n",(++Fxi(2)).real());
		std::cout << std::format("--2: {0}\n",(--Fxi(2)).real());
		for(int i=0; i<4; i++,val++) {
			val++;
			val--;
		}

		Vec2fxi v { 3,4 };
		std::cout << std::format("v:     {0}\n", v.to_str());
		std::cout << std::format("v.len: {0}\n", v.magnitude().real());
	}
};
namespace sample_sclarc {
	static void fn_pack() {
		auto archive = sclarc::create_file("../../../guiutil/kappamap/kmap/workdata/");
		archive.file_send("workdata/out_scl.bin");
		std::puts("sclarc: packed!");
	}
	static void fn_unpack() {
		auto record = sclarc::Record::from_file("workdata/out_scl.bin");

		// read file from archive into single file ------@/
		{ auto file = record->file_open("/script/test/hello.lua");
			std::cout << std::format("file's name: {0}\n",file.name());

			// send to workdata -------------------------@/
			file.read(file.filesize()).file_send("workdata/str.txt");

			file.close();
		}
		std::puts("sclarc: unpacked!");
	}
};

int main(int argc, const char* argv[]) {
	sample_objlist::start();
	sample_fxi::start();
	sample_sclarc::fn_pack();
	sample_sclarc::fn_unpack();
}

