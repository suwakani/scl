// ==========================================================================@/
//                                                                           @/
// -- object pool system --------------------------------------------------- @/
//                                                                           @/
// ==========================================================================@/
/*
	*	unlike previous implementations, each Header will hold a list of
		pointers to weak references. you may use this to safely deal with
		references to objects in the pool.

	*	this also means that the recommended usage of the library is to create
		an object & an accompanying reference via the `Manager::add_ref()`
		function, and then use the lock() funciton to safely get the object
		from the reference. this allows you to deal with objects from a pool
		without having to worry about pointers to dead objects.

	*	to add an object:
			auto objRef = pool.add_ref();  // <- this is the object reference!
			auto obj = objRef.lock();      // <- this is the actual object!
*/
#pragma once
#include <scl/basis/errhandle.hpp>
#include <cstdint>
#include <format>
#include <vector>
// ------------------------------------------------------@/
namespace scl {
namespace pool {
// ------------------------------------------------------@/
template<typename Type> class Ref;
template<typename Type> class Header;
template<typename Type> class Manager;

// ==========================================================================@/
//                                                                           @/
// -- Ref & Header --------------------------------------------------------- @/
//                                                                           @/
// ==========================================================================@/
template<typename Type> class Ref {
	public:
		Ref<Type>* mLLPrev;
		Ref<Type>* mLLNext;
		Header<Type>* mHeader;

		// modifiers ------------------------------------------@/
		auto bind(Header<Type>* header) -> void;
		auto unbind() -> void;
		constexpr auto del() -> void;

		// observers ------------------------------------------@/
		constexpr auto expired() const -> bool { 
			return mHeader == nullptr;
		}
		constexpr auto lock() -> const Type* {
			SCL_ASSERT_MSG(not expired(), "ref %p: attempted to lock null ref!", this);
			return get();
		}
		constexpr auto get() const -> Type*;

		// constructors ---------------------------------------@/
		constexpr Ref() : mHeader(NULL),mLLPrev(NULL),mLLNext(NULL) {
		}
		constexpr Ref(Header<Type>* header) {
			mHeader = NULL;
			mLLPrev = NULL;
			mLLNext = NULL;
			bind(header);
		}
		constexpr ~Ref() {
			unbind();
		}
		constexpr Ref(const Ref<Type>& other) {
			mHeader = NULL;
			mLLPrev = NULL;
			mLLNext = NULL;
			if(not other.expired()) {
				bind(other.mHeader);
			}
		}

		auto operator=(const Ref<Type>& other) -> Ref<Type>& {
			if(not other.expired()) {
				bind(other.mHeader);
			}
			return *this;
		}

		// misc -----------------------------------------------@/
		auto to_str() const -> std::string {
			return std::format("Ref(@{0:X})[ header={1:X}, prev={2:X}, next={3:X} ]",
				(size_t)this,(size_t)mHeader,(size_t)mLLPrev,(size_t)mLLNext
			);
		}
};
template<typename Type> class Header {
	public:
		uint32_t mAlive;
		uint16_t mID,mIDNext;
		Manager<Type>* mManager;
		Ref<Type>* mReflistTail;

		// observers ------------------------------------------@/
		constexpr auto alive() const -> bool { return mAlive; }
		constexpr auto id() const -> std::size_t { return mID; }
		constexpr auto get() const -> Type*;

		// modifiers ------------------------------------------@/
		constexpr auto del() -> void;
		constexpr auto reflist_clear() -> void;

		// constructors ---------------------------------------@/
		Header() : mAlive(false),mID(0),mIDNext(0),
			mManager(NULL),mReflistTail(NULL) {}
};

// reference structure's methods ------------------------@/
template<typename Type> auto Ref<Type>::bind(Header<Type>* header) -> void {
	/*
		* we should NOT be able to bind a reference twice to the same object.
		* we should also only be able to bind if not expired. if this ref is
		  already binded, it should unbind before re-binding.
	*/
	SCL_ASSERT_MSG(header, "ref %p: bind to null!", this);
	SCL_ASSERT_MSG(header->alive(), "ref %p: bind to dead pool entry!", this);
	SCL_ASSERT_MSG(header != mHeader, "ref %p: double-binded to header %p!", this, header);
	if(!expired()) unbind();
	mHeader = header;

	// create link (add to tail, if needed) -------------@/
	if(auto tailRef = mHeader->mReflistTail) {
		// this case is used if adding to existing list
		mLLPrev = tailRef;
		mLLNext = nullptr;
		tailRef->mLLNext = this;
	} else {
		// this case is used if the list must be created
		mLLPrev = nullptr;
		mLLNext = nullptr;
	}
	mHeader->mReflistTail = this;
}
template<typename Type> auto Ref<Type>::unbind() -> void {
	if(mHeader) {
		if(mHeader->mReflistTail == this) {
			mHeader->mReflistTail = mLLPrev;
		}

		// remove link ----------------------------------@/
		if(mLLPrev) { mLLPrev->mLLNext = mLLNext; }
		if(mLLNext) { mLLNext->mLLPrev = mLLPrev; }
	}
	mLLPrev = nullptr;
	mLLNext = nullptr;
	mHeader = nullptr;
}
template<typename Type> constexpr auto Ref<Type>::get() const -> Type* {
	return mHeader->get();
}
template<typename Type> constexpr auto Ref<Type>::del() -> void {
	mHeader->del();
}
// header structure's methods ---------------------------@/
template<typename Type> constexpr auto Header<Type>::get() const -> Type* {
	return &mManager->mTableObject[mID];
}
template<typename Type> constexpr auto Header<Type>::del() -> void {
	mManager->del_header(this);
}
template<typename Type> constexpr auto Header<Type>::reflist_clear() -> void {
	while(mReflistTail) {
		mReflistTail->unbind();
	}
}

// ==========================================================================@/
//                                                                           @/
// -- Manager -------------------------------------------------------------- @/
//                                                                           @/
// ==========================================================================@/
template <typename Type> class Manager {
	public:
		using TypeRef = Ref<Type>;
		using TypeHeader = Header<Type>;
	public:
		Type* mTableObject;
		TypeHeader* mTableHeader;
		bool mAutoEnable;
		bool mDidSetup;
		size_t mAliveNow;
		size_t mAliveMax;
		size_t mIdxLast;

		constexpr auto size() const -> size_t { return mAliveNow; }
		constexpr auto max_size() const -> size_t { return mAliveMax; }
		constexpr auto totalsize_object() const -> size_t { return sizeof(Type) * mAliveMax; }
		constexpr auto totalsize_header() const -> size_t { return sizeof(TypeHeader) * mAliveMax; }

		// construct/destructor -------------------------@/
		Manager() {
			mTableObject = nullptr;
			mTableHeader = nullptr;
			mIdxLast = 0;
			mAliveNow = 0;
			mAliveMax = 0;
			mAutoEnable = false;
			mDidSetup = false;
		}
		Manager(size_t max) {
			mDidSetup = false;
			setup_auto(max);
		}
		Manager(Type* tableObject, TypeHeader* tableHeader, size_t max) {
			mDidSetup = false;
			setup_manual(tableObject,tableHeader,max);
		}

		~Manager() {
			close();
		}

		// setup fns ------------------------------------------@/
		auto setup_auto(size_t max) -> void {
			if(mDidSetup) close();

			mTableObject = static_cast<Type*>(std::malloc(sizeof(Type) * max));
			mTableHeader = new TypeHeader[max];
			mIdxLast = 0;
			mAliveNow = 0;
			mAliveMax = max;
			mAutoEnable = true;

			mDidSetup = true;

			setup();
		}
		auto setup_manual(Type* tableObject, TypeHeader* tableHeader, size_t max) -> void {
			if(mDidSetup) close();

			mTableObject = tableObject;
			mTableHeader = tableHeader;
			mIdxLast = 0;
			mAliveNow = 0;
			mAliveMax = max;
			mAutoEnable = false;

			mDidSetup = true;

			setup();
		}
		auto close() -> void {
			if(!mDidSetup) return;

			// delete all objects --------------------------------@/
			for(size_t i=0; i<max_size(); i++) {
				auto& header = mTableHeader[i];
				if(header.alive()) {
					del_header(&header);
				}
			}

			// deallocate ----------------------------------------@/
			if(mAutoEnable) {
				if(mTableObject) { std::free(mTableObject); }
				if(mTableHeader) { delete[] mTableHeader; }
			}
			mTableObject = nullptr;
			mTableHeader = nullptr;

			mIdxLast = 0;
			mAliveNow = 0;
			mAliveMax = 0;
			mDidSetup = false;
		}

		// adding fns -----------------------------------------@/
		auto del_header(TypeHeader* header) {
			SCL_ASSERT_MSG(header, "pool %p: attempt to delete NULL header!", this);
			SCL_ASSERT_MSG(header->alive(), "pool %p: attempt to delete dead header %p!", this, header);
			
			// deconstruct object -----------------------@/
			mTableObject[header->mID].~Type();

			// clear references -------------------------@/
			header->reflist_clear();

			// remove from free list --------------------@/
			header->mAlive = false;
			header->mIDNext = mIdxLast;
			mAliveNow--;
			mIdxLast = header->mID;
		}
		template <typename ...Params> constexpr auto add_ref(Params&&... params) -> TypeRef {
			return TypeRef(add_header(std::forward<Params>(params)...));
		}
		template <typename ...Params> auto add_header(Params&&... params) -> TypeHeader* {
			// get free object --------------------------@/
			SCL_ASSERT_MSG(mTableObject,"pool %p: didn't setup!",this);
			SCL_ASSERT_MSG(mAliveNow != mAliveMax,"pool %p: object over!",this);
			auto last_idx = mIdxLast;

			// add to free list -------------------------@/
			auto& header = mTableHeader[last_idx];
			mIdxLast = header.mIDNext;
			mAliveNow++;
			header.mAlive = true;

			// create the object it points to -----------@/
			new(&mTableObject[last_idx]) Type(std::forward<Params>(params)...);
			return &header;
		}

		// list operations ------------------------------------@/
		auto get_tableAlive() -> std::vector<TypeRef> {
			size_t limit = size();
			std::vector<TypeRef> table;

			for(size_t i=0; i<max_size() && table.size() < limit; i++) {
				auto& header = &mTableHeader[i];
				if(header.alive()) {
					table.push_back(TypeRef(&header));
				}
			}

			return table;
		}
	private:
		auto setup() -> void {
			SCL_ASSERT_MSG(mAliveMax>0, "pool %p: max alive must be > 0",this);
			SCL_ASSERT_MSG(mAliveMax<=65535, "pool %p: max alive must be < 65536", this);
			SCL_ASSERT_MSG(mAliveNow==0, "pool %p: num. alive must be 0",this);

			if(mAutoEnable) {
				SCL_ASSERT_MSG(mTableObject,"pool %p: objects must exist",this);
				SCL_ASSERT_MSG(mTableHeader,"pool %p: header must exist",this);
			}
			
			for(size_t i=0; i<mAliveMax; i++) {
				auto &header = mTableHeader[i];
				header.mAlive = false;
				header.mID = i & 0xFFFF;
				header.mIDNext = (i+1) & 0xFFFF;
				header.mManager = this;
				header.mReflistTail = nullptr;
			}
			mTableHeader[mAliveMax-1].mID = 0xFFFF;
		}
};

// ------------------------------------------------------@/
} // namespace pool
} // namespace scl
// ------------------------------------------------------@/

