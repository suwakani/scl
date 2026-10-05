#pragma once

/*
	*	enhanced:tm: archive format.
	*	unlike the previous method, we want to be able to load metadata from
		.json files.

	*	to create an archive, we need a Metadata object to store all the
		working information about the archive, such as all the filenames that
		will be used.
	*	we then enumerate all of the folders, assigning an ID to each.
	*	next, we create two tables for each folder:
		-	one for the sub-folders
		-	one for the files
		*	each containing IDs for all of them.
	
	*	the .SAR file will contain the following sections:
		-	folder table
		-	file table
		-	folder ID tables
		-	file ID tables
		-	filenames
		-	filedata
*/

#include <scl/container/blob.hpp>

#include <map>
#include <memory>
#include <string>
#include <cstdio>

namespace scl {
namespace io {
namespace archive {

namespace FolderID {
	enum {
		Root = 0,
		Null = 0xFFFFFF,
	};
};

using Hash = uint32_t;

// ==========================================================================@/
// metadata                                                                  @/
// ==========================================================================@/
class MetadataFile {
	public:
		std::string mName;
		std::string mSourceFilename;
		std::size_t mID;
		std::size_t mParentID;
		Hash mHashName;
		Hash mHashFullpath;
		
		MetadataFile()
			: mName("_UNNAMED"),mSourceFilename(),mID(0),mParentID(FolderID::Null) {}
		MetadataFile(std::string name, std::string source) 
			: mName(name),mSourceFilename(source),mID(0),mParentID(FolderID::Null) {}

		auto ID() const -> std::size_t { return mID; }
		auto name() const -> std::string { return mName; }
		auto source() const -> std::string { return mSourceFilename; }
		auto hashName() const -> Hash { return mHashName; }
		auto hashFullpath() const -> Hash { return mHashFullpath; }
};
class MetadataFolder {
	public:
		std::string mName;
		std::vector<MetadataFile> mFiles;
		std::vector<MetadataFolder> mFolders;
		std::size_t mID;
		std::size_t mParentID;
		Hash mHashName;
		Hash mHashFullpath;

		MetadataFolder()
			: mFiles(),mFolders(),mID(0),mParentID(FolderID::Null) {}
		MetadataFolder(std::string name)
			: mName(name),mFiles(),mFolders(),mID(0),mParentID(FolderID::Null) {}
		
		auto name() const -> std::string { return mName; }
		auto ID() const -> std::size_t { return mID; }
		auto parentID() const -> std::size_t { return mParentID; }
		auto hashName() const -> Hash { return mHashName; }
		auto hashFullpath() const -> Hash { return mHashFullpath; }

		auto add_file(MetadataFile file) -> void;
		auto add_folder(MetadataFolder fldr) -> void;
};
class Metadata {
	public:
		MetadataFolder root_folder;
};

// ==========================================================================@/
// file structure                                                            @/
// ==========================================================================@/
struct SARFile_Header {
	char magic[4];
	uint32_t offset_segTblFolder;
	uint32_t offset_segTblFile;
	uint32_t offset_segTblFolderID;
	uint32_t offset_segTblFileID;
	uint32_t offset_segString;
	uint32_t offset_segFiledata;

	uint32_t num_files;
	uint32_t num_folders;
};
struct SARFile_EntryFolder {
	uint32_t parentID;
	uint32_t childID_file;
	uint32_t childID_folder;
	uint32_t num_files;
	uint32_t num_folders;
	uint32_t name_idx;
	uint32_t name_len;
	uint32_t hashName;
	uint32_t hashFullpath;
};
struct SARFile_EntryFile {
	uint32_t data_len;
	uint32_t data_idx;
	uint32_t name_idx;
	uint32_t name_len;
	uint32_t hashName;
	uint32_t hashFullpath;
};

// ==========================================================================@/
// record                                                                    @/
// ==========================================================================@/
class Record;
class RecordInfo_File;

class RecordInfo_File {
	public:
		std::size_t mID;
		std::size_t mParentID;
		std::string mName;
		std::size_t mDataIdx;
		std::size_t mDataLen;
		Hash mHashName;
		Hash mHashFullpath;

		constexpr auto ID() const -> size_t { return mID; }
		constexpr auto name() const -> std::string { return mName; }
		constexpr auto data_len() const -> size_t { return mDataLen; }
		constexpr auto data_lenMB() const -> double { return ((double)mDataLen) / (1024.0*1024.0); }
		constexpr auto hashName() const -> Hash { return mHashName; }
		constexpr auto hashFullpath() const -> Hash { return mHashFullpath; }

		RecordInfo_File()
			: mID(0),mParentID(FolderID::Null),mName(),mDataIdx(0),mDataLen(0) {}
		RecordInfo_File(size_t ID, std::string name)
			: mID(ID),mParentID(FolderID::Null),mName(name),mDataIdx(),mDataLen() {}
};
class RecordInfo_Folder {
	public:
		std::size_t mID;
		std::size_t mParentID;
		std::string mName;
		std::vector<RecordInfo_File> mTableFile;
		std::vector<RecordInfo_Folder> mTableFolder;
		bool mReadonly;

		auto add_file(const RecordInfo_File& file) -> void;
		auto add_folder(const RecordInfo_Folder& folder) -> void;
		auto mark_readonly() -> void;

		constexpr auto ID() const -> size_t { return mID; }
		constexpr auto parentID() const -> size_t { return mParentID; }
		constexpr auto name() const -> std::string { return mName; }

		RecordInfo_Folder()
			: mID(FolderID::Null),mParentID(FolderID::Null),mReadonly(false) {}
		RecordInfo_Folder(size_t ID, std::string name)
			: mID(ID),mParentID(FolderID::Null),mName(name),mReadonly(false) {}
};
class RecordFile {
	public:
		Record* mRecordCurrent;
		RecordInfo_File* mInfoFile;
		std::FILE* hFile;
		std::size_t mOffset;

		auto seek(int offset, int origin = SEEK_SET) -> void;
		auto read(size_t len) -> scl::Blob;
		auto read(void* output, size_t len) -> void;
		auto close() -> void;
		auto open(Record& record, const std::string& filename) -> void;

		constexpr auto tell() -> size_t { return mOffset; }
		constexpr auto filesize() const -> size_t {
			SCL_ASSERT_MSG(mInfoFile,"Record %p: no file",this);
			return mInfoFile->mDataLen;
		}
		constexpr auto name() const -> std::string {
			SCL_ASSERT_MSG(mInfoFile,"Record %p: no file",this);
			return mInfoFile->name();
		}

		RecordFile() 
			: mRecordCurrent(NULL),mInfoFile(NULL),hFile(NULL),mOffset(0) {}
		RecordFile(Record& record, const std::string& filename);
		~RecordFile();
};

class Record {
	public:
		RecordInfo_Folder mFolderRoot;
		RecordInfo_Folder* mFolderCurrent;
		std::vector<RecordInfo_File*> mArrayFile;
		std::vector<RecordInfo_Folder*> mArrayFolder;
		std::map<Hash,RecordInfo_File*> mHashFile;
		std::string mRecordFilename;
		std::size_t mFiledataOffset;
		int mNumFilehandles;

		static auto from_file(const std::string& filename, bool strict=true) -> std::shared_ptr<Record>;

		auto load_file(const std::string& src_filename, bool strict=true) -> void;
		auto file_open(const std::string& filename) -> RecordFile;
		auto file_find(const std::string& filename) -> RecordInfo_File*;

		constexpr auto filehandle_add() -> void { mNumFilehandles += 1; }
		constexpr auto filehandle_sub() -> void { mNumFilehandles -= 1; }
		constexpr auto filehandle_isAny() -> bool { return mNumFilehandles > 0; }

		Record() : mFolderCurrent(NULL),mNumFilehandles(0) {}
		~Record();
};

// ==========================================================================@/
// misc fns                                                                  @/
// ==========================================================================@/
scl::Blob create_file(const std::string& src_filename);
static inline Hash create_hash(const std::string& str) {
	uint64_t hash = 0x811C9DC5;
	for(const char c : str) {
		hash = ((hash ^ static_cast<uint8_t>(c)) * 0x1000193) & 0xFFFFFFFF;
	}
	return static_cast<Hash>(hash);
}

}; // namespace archive
}; // namespace io
}; // namespace scl

