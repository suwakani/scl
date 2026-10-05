#pragma once

#include <bit>
#include <cstdint>
#include <exception>
#include <string>
#include <vector>

#include <scl/basis/errhandle.hpp>

#ifdef SCL_USE_ZLIB
#include <zlib.h>
#endif

namespace scl {

class Blob {
	public:
		std::vector<uint8_t> mData;

		auto write_blob(const Blob& source) -> Blob& {
			mData.insert(mData.end(),source.mData.begin(),source.mData.end());
			return *this;
		}
		auto write_raw(const void* source, std::size_t len) -> Blob& {
			SCL_ASSERT_MSG(source,"blob %p: attempt to write from null source",this);
			if(len == 0) return *this;

			auto src_bytes = static_cast<const uint8_t*>(source);
			mData.insert(mData.end(),src_bytes,src_bytes + len);
			return *this;
		}
		constexpr auto write_u8(uint32_t n) -> void { mData.push_back(n); }
		constexpr auto write_u16(uint32_t n) -> void { write_u8(n); write_u8(n>>8); }
		constexpr auto write_u32(uint32_t n) -> void { write_u16(n); write_u16(n>>16); }
		constexpr auto write_u64(uint64_t n) -> void { write_u32(n); write_u32(n>>32); }
		constexpr auto write_be_u16(uint16_t n) -> void { write_u16(std::byteswap(n)); }
		constexpr auto write_be_u32(uint32_t n) -> void { write_u32(std::byteswap(n)); }
		constexpr auto write_be_u64(uint64_t n) -> void { write_u64(std::byteswap(n)); }
		auto write_str(std::string_view str, bool no_terminator=false) -> void {
			for(size_t i=0; i<str.size(); i++) {
				char chr = str.at(i);
				write_u8(chr);
			}
			if(!no_terminator) {
				write_u8(0);
			}
		}

		auto pad(int divisor, int padbyte = 0xAB) -> Blob& {
			while((size()%divisor) != 0) {
				write_u8(padbyte);
			}
			return *this;
		}
		auto clear() -> void {
			mData.clear();
		}
		auto resize(size_t amt) -> void {
			mData.resize(amt);
		}

		auto file_load(const std::string& filename, bool strict=true) -> bool;
		auto file_send(const std::string& filename, bool strict=true) const -> bool;
#ifdef SCL_USE_ZLIB
		auto compress(bool include_metadata=true, const bool do_compress=true) -> Blob {
			std::vector<Bytef> comp_data(size()*2 + 32);

			// just a level below Z_BEST_COMPRESSION (9)
			// but above Z_BEST_SPEED(1)
			int compress_mode = Z_NO_COMPRESSION;
			if(do_compress) compress_mode = Z_BEST_COMPRESSION;
		//	if(do_compress) compress_mode = Z_BEST_SPEED;

			z_stream zlstrm;
			zlstrm.zalloc = Z_NULL;
			zlstrm.zfree = Z_NULL;
			zlstrm.opaque = Z_NULL;

			zlstrm.avail_in = size();
			zlstrm.next_in = data<Bytef*>();
			zlstrm.avail_out = comp_data.size();
			zlstrm.next_out = comp_data.data();

			auto compstat = deflateInit(&zlstrm,compress_mode);
			if(compstat != Z_OK) {
				std::printf("Blob::compress(): error: compression failed to init (%d)...\n",compstat);
				std::terminate();
			}
			auto succ_deflate = deflate(&zlstrm,Z_FINISH);
			if(succ_deflate == Z_STREAM_ERROR) {
				std::printf("Blob::compress(): error: deflate() failed (%d)\n",succ_deflate);
				std::terminate();
			}
			auto succ_deflateEnd = deflateEnd(&zlstrm);

			if(succ_deflateEnd != Z_OK) {
				std::printf("Blob::compress(): error: deflateEnd() failed (%d)\n",succ_deflateEnd);
				std::terminate();
			}

			Blob comp_blob;
			if(include_metadata) {
				// 4 bytes for magic, 4 bytes for original size, 4 bytes for
				// packed size
				comp_blob.write_str("SBL");
				comp_blob.pad(8);
				comp_blob.write_u64(size());
				comp_blob.write_u64(zlstrm.total_out);
			}
			comp_blob.write_raw(comp_data.data(),zlstrm.total_out);
		//	if(comp_blob.size() > comp_data.size()) {
		//		std::puts("Blob::compress(): error: compression was > orig size...");
		//		std::terminate();
		//	}

			return comp_blob;
		}
		auto compress_full(const bool do_compress=true) -> Blob {
			return compress(true,do_compress);
		}
		auto compress_raw(const bool do_compress=true) -> Blob {
			return compress(false,do_compress);
		}
		auto decompress(bool include_metadata=true) -> Blob {
		/*	MARISA_ASSERT(outbuf,"marisa_data_decompress","output buffer is null!");
			MARISA_ASSERT(srcbuf,"marisa_data_decompress","source buffer is null!");
			MARISA_ASSERT(srcsize>0,"marisa_data_decompress","source size is 0!");
			MARISA_ASSERT(outsize>0,"marisa_data_decompress","output size is 0!");*/

			z_stream zstrm;
			zstrm.zalloc = Z_NULL;
			zstrm.zfree = Z_NULL;
			zstrm.opaque = Z_NULL;

			std::vector<uint8_t> filler;
			Blob newblob;

			if(include_metadata) {
				if(size() <= 24) {
					puts("blob::decompress(): invalid metadata!");
					std::terminate();
				}
				size_t size_packed = 0;
				size_t size_unpacked = 0;
				
				size_packed = *reinterpret_cast<uint64_t*>(data<char*>() + 16);
				size_unpacked = *reinterpret_cast<uint64_t*>(data<char*>() + 8);

				filler.resize(size_unpacked);
				newblob = scl::Blob(filler);
				std::printf("unpacked size: %6zX\n",size_unpacked);
				std::printf("packed size: %6zX\n",size_packed);
				zstrm.avail_in = size_packed;
				zstrm.next_in = data<Bytef*>() + 24;
				zstrm.avail_out = size_unpacked;
				zstrm.next_out = newblob.data<Bytef*>();
			} else {
				puts("blob::decompress(): invalid data!");
				std::terminate();
			}

			inflateInit(&zstrm);
			inflate(&zstrm,Z_NO_FLUSH);
			inflateEnd(&zstrm);

			return newblob;
		}
		auto decompress_full() -> Blob {
			return decompress(true);
		}
#endif
		template<typename T=void*> auto data() -> T {
			return reinterpret_cast<T>(mData.data());
		}
		constexpr auto size() const -> std::size_t { return mData.size(); }
		constexpr auto at(const std::size_t idx) -> uint8_t& {
			return mData.at(idx); 
		}
		constexpr auto at(const std::size_t idx) const -> const uint8_t& {
			return mData.at(idx); 
		}

		Blob() {
			clear();
		}
		Blob(const Blob& orig) {
			mData = orig.mData;
		}
		Blob(const std::vector<uint8_t>& data) {
			mData = data;
		}

		constexpr auto operator=(const Blob& other) -> Blob& {
			mData = other.mData;
			return *this;
		}

		static auto from_str(const std::string& str, bool no_terminator=false) -> Blob {
			Blob strblob;
			strblob.write_str(str,no_terminator);
			return strblob;
		}
		static auto from_file(const std::string& filename, bool strict=true) -> Blob {
			Blob bl;
			bl.file_load(filename,strict);
			return bl;
		}
};

} // namespace scl


