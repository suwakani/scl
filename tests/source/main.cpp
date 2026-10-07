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
using Fxi = scl::math::QFxi<14>;

namespace sample_sdzarc {
	static void fn_pack() {
		std::vector<scl::Blob> filedata_table;
		std::vector<std::string> filename_table;
		auto basepath = stdfs::path("../../../guiutil/kappamap/kmap/workdata/sprite/editor/chip");
		std::printf("foldername: %s\n",basepath.string().c_str());
		for (const auto & entry : stdfs::recursive_directory_iterator(basepath)) {
			if(entry.is_regular_file()) {
				auto path = entry.path().string();
				auto proxim = stdfs::proximate(entry.path(),basepath);
				scl::Blob filedata;
				std::printf("loading file arc::%s\n",proxim.string().c_str());
				filedata.file_load(path);
				filedata_table.push_back(filedata);
				filename_table.push_back(proxim.string());
			}
		}

		constexpr size_t pad_size = 256;
		scl::Blob blob_all;
		scl::Blob blob_segHeader;
		scl::Blob blob_segEntry;
		scl::Blob blob_segName;
		scl::Blob blob_segData;

		std::puts("writing blobs");
		for(auto const [index,data] : std::views::enumerate(filedata_table)) {
			blob_segEntry.write_u32(index);
			blob_segEntry.write_u32(data.size());
			blob_segEntry.write_u32(blob_segName.size());
			blob_segName.write_str(filename_table.at(index));
			blob_segData.write_blob(data);
		}
		std::puts("writing final");
		blob_segEntry.pad(pad_size);
		blob_segName.pad(pad_size);
		blob_segData.pad(pad_size);

		const std::size_t offset_segEntry = pad_size;
		const std::size_t offset_segName = offset_segEntry + blob_segEntry.size();
		const std::size_t offset_segData = offset_segName + blob_segName.size();
		blob_segHeader.write_str("SDA");
		blob_segHeader.write_u32(offset_segEntry);
		blob_segHeader.write_u32(offset_segName);
		blob_segHeader.write_u32(offset_segData);
		blob_segHeader.pad(pad_size);


		blob_all.write_blob(blob_segHeader)
			.write_blob(blob_segEntry)
			.write_blob(blob_segName)
			.write_blob(blob_segData);

		blob_all.file_send("workdata/out.bin");
		blob_all.compress_full().file_send("workdata/outPacked.bin");
	}
	static void fn_unpack() {
		scl::Blob filedata;
		filedata.file_load("workdata/outPacked.bin");
		auto unpacked = filedata.decompress_full();
		unpacked.file_send("workdata/outUnpacked.bin");
	}
};
namespace sample_objlist {
	class CUnit {
		public:
			int16_t mX,mY;
			int16_t hp;
			int16_t alive;

			CUnit() {
				std::printf("unit %p: constructed!\n",this);
			}
			~CUnit() {
				std::printf("unit %p: deleted!\n",this);
			}
	};

	auto start() -> void {
		auto boolstr = [](bool cond) {
			return cond ? "true" : "false";
		};
		scl::Pool<CUnit> objpool(200);

		// creating and deleting 1 ----------------------@/
		{
			auto objStat = objpool.add_status();
			auto objRef = scl::PoolRef(objStat);

			std::printf("objref.get() == objStat.get(): %s\n", boolstr(objRef.get() == objStat->get()));
			std::printf("objref.expired: %s\n", boolstr(objRef.expired()));
			objpool.del_status(objStat);
			std::printf("objref.expired: %s\n", boolstr(objRef.expired()));
		}
		// creating and deleting multiple ---------------@/
		std::vector<scl::Pool<CUnit>::TypeRef> reftable;
		for(int i=0; i<4; i++) {
			reftable.push_back(objpool.add_ref());
		}
		while(!reftable.empty()) {
			auto ref = reftable.back();
			ref.del();
			reftable.pop_back();
		}
	}
};
namespace sample_fxi {
	auto start() -> void {
		std::printf("fxi size: S.%zu.%zu\n",Fxi::NumBits-1-Fxi::Shift,Fxi::Shift);
		std::printf("fxi max:  %lld\n",Fxi::Max);
		std::printf("fxi min:  %f\n",Fxi(1,0).real());
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
//	sample_sdzarc::fn_pack();
//	sample_sdzarc::fn_unpack();

	sample_objlist::start();
	sample_fxi::start();
	sample_sclarc::fn_pack();
	sample_sclarc::fn_unpack();
}

