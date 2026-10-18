#define SCL_USE_ZLIB
#include <scl/container/blob.hpp>
#include <scl/container/pool.hpp>
#include <scl/io/archive.hpp>
#include <scl/math/fixed.hpp>
#include <scl/math/vector.hpp>
#include <scl/thread/task.hpp>

#include <format>
#include <functional>
#include <iostream>

namespace sclarc = scl::io::archive;
namespace sclpool = scl::pool;
namespace scltask = scl::task;
using Fxi = scl::math::QFxi<14>;
using Vec2fxi = scl::math::Vec<Fxi,2>;
using Vec3fxi = scl::math::Vec<Fxi,3>;

namespace sample_objlist {
	class CUnit {
		public:
			Vec3fxi mPos;
			int32_t mHP;
			int32_t mAlive;
			sclpool::Ref<CUnit> mRef;
			std::array<uint8_t,40> mWork;

			auto ref_assign(const sclpool::Ref<CUnit>& ref) -> void {
				mRef = ref;
				std::cout << std::format("\tref: {0}\n", mRef.to_str());
			}

			auto hello() -> void {
				std::printf("hello, from object %p! hp=%d.\n", this,mHP);
			}

			CUnit() {
				mPos = { 0,0,0 };
				mHP = 0;
				mAlive = true;
				std::printf("unit %p: constructed!\n",this);
			}
			CUnit(int hp) {
				mPos = { 0,0,0 };
				mHP = hp;
				mAlive = true;
				std::printf("unit %p: constructed EX!\n", this);
			}
			~CUnit() {
				std::printf("unit %p: deleted!\n",this);
			}
	};

	auto start() -> void {
		sclpool::Manager<CUnit> objpool(64);

		// creating and deleting multiple ---------------@/
		std::vector<sclpool::Ref<CUnit>> reftable;

		for(int i=0; i<4; i++) {
			auto objRef = objpool.add_ref();
			reftable.push_back(objRef);
		}
		while(!reftable.empty()) {
			auto objRef = reftable.back();
			objRef.del();
			reftable.pop_back();
		}

		// swapping & object creation -------------------@/
		auto objRef_a = objpool.add_ref();
		auto objRef_b = objpool.add_ref();
		std::swap(objRef_a,objRef_b);
		auto objRef_c = objpool.add_ref(40);
			objRef_c.lock()->ref_assign(objRef_c);

		// slow shared_ptr-like usage -------------------@/
		auto objSh_ref = objpool.add_ref();
		objSh_ref->hello();
		std::printf("hp: %d\n", objSh_ref->mHP);

		std::cout << std::format("objRef_a: {0}\n", objRef_a.to_str());
		std::cout << std::format("objRef_b: {0}\n", objRef_b.to_str());

		std::cout << std::format("objpool ({0} objs)'s size: {1}KB(obj) + {2}KB(header)\n",
			objpool.max_size(),
			(float)(objpool.totalsize_object()) / 1024.0,
			(float)(objpool.totalsize_header()) / 1024.0
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
		for(int i=0; i<=20; i++) {
			auto n = Fxi(i) / 20;
			std::cout << std::format("{0}/20: {1} (actual: {2})\n", i, n.real(), (float)(i) / 20);
		}
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
namespace sample_tasks {
	auto start() -> void {
		scltask::Manager taskmgr(32);

		scltask::Callback tskfn_main = [](scltask::Unit& task) {
			std::puts("hi... Okay");
		};
		scltask::Callback tskfn_sub = [](scltask::Unit& task) {
			std::puts("sub task... children will be killed.");
			task.del();
		};
		scltask::Callback tskfn_worker = [](scltask::Unit& task) {
			std::printf("worker...! (%p)\n", &task);
		};
		scltask::Callback tskfn_worker2 = [](scltask::Unit& task) {
			std::printf("worker...2!!!! (%p)\n", &task);
		};

		auto add_workers = [&](scltask::UnitRef& parentRef) {
			taskmgr.unit_add(0x03, parentRef, tskfn_worker2, "worker2");
			taskmgr.unit_add(0x00, parentRef, tskfn_worker, "worker");
			taskmgr.unit_add(0x00, parentRef, tskfn_worker, "worker");
			taskmgr.unit_add(0x00, parentRef, tskfn_worker, "worker");
			taskmgr.unit_add(0x00, parentRef, tskfn_worker, "worker");
		};

		// prio, parent, callback, name
		auto tsk_main = taskmgr.unit_addOrphan(tskfn_main, "main");
		auto tsk_sub = taskmgr.unit_add(0x00, tsk_main, tskfn_sub, "sub");
		add_workers(tsk_main);
		add_workers(tsk_sub);

		std::cout << std::format("task tree:\n{0}\n",
			tsk_main->tostr_tree()
		);
		
		tsk_main->step();
		tsk_main->del();

		// prev: 160
		// now: 
		std::cout << std::format("task size: {0} ({1}KB)\n",
			sizeof(scltask::Unit),(double)(taskmgr.totalsize_unit()) / 1024.0
		);
	}
};

int main(int argc, const char* argv[]) {
	sample_objlist::start();
	sample_fxi::start();
	sample_sclarc::fn_pack();
	sample_sclarc::fn_unpack();
	sample_tasks::start();
}

