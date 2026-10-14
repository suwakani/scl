// ==========================================================================@/
//                                                                           @/
// -- object pool system --------------------------------------------------- @/
//                                                                           @/
// ==========================================================================@/
/*
	*	unlike previous implementations, each Status will hold a list of
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
// ------------------------------------------------------@/
namespace scl {
namespace pool {
// ------------------------------------------------------@/
template<typename Type> class Ref;
template<typename Type> class Status;
template<typename Type> class Manager;

// ==========================================================================@/
//                                                                           @/
// -- Ref & Status --------------------------------------------------------- @/
//                                                                           @/
// ==========================================================================@/
template<typename Type> class Ref {
	public:
		Ref<Type>* mLLPrev;
		Ref<Type>* mLLNext;
		Status<Type>* mStatus;

		// modifiers ------------------------------------------@/
		auto bind(Status<Type>& status) -> void;
		auto unbind() -> void;
		constexpr auto del() -> void;

		// observers ------------------------------------------@/
		constexpr auto expired() const -> bool { 
			return mStatus == nullptr;
		}
		constexpr auto lock() -> const Type* {
			SCL_ASSERT_MSG(not expired(), "ref %p: attempted to lock null ref!", this);
			return get();
		}
		constexpr auto get() const -> Type*;

		// constructors ---------------------------------------@/
		constexpr Ref() : mStatus(NULL),mLLPrev(NULL),mLLNext(NULL) {
		}
		constexpr Ref(Status<Type>& status) {
			mStatus = NULL;
			mLLPrev = NULL;
			mLLNext = NULL;
			bind(status);
		}
		constexpr ~Ref() {
			unbind();
		}
		constexpr Ref(const Ref<Type>& other) {
			mStatus = NULL;
			mLLPrev = NULL;
			mLLNext = NULL;
			if(not other.expired()) {
				bind(*other.mStatus);
			}
		}

		auto operator=(const Ref<Type>& other) -> Ref<Type>& {
			if(not other.expired()) {
				bind(*other.mStatus);
			}
			return *this;
		}

		// misc -----------------------------------------------@/
		auto to_str() const -> std::string {
			return std::format("Ref(@{0:X})[ status={1:X}, prev={2:X}, next={3:X} ]",
				(size_t)this,(size_t)mStatus,(size_t)mLLPrev,(size_t)mLLNext
			);
		}
};
template<typename Type> class Status {
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
		Status() : mAlive(false),mID(0),mIDNext(0),
			mManager(NULL),mReflistTail(NULL) {}
};

// reference structure's methods ------------------------@/
template<typename Type> auto Ref<Type>::bind(Status<Type>& status) -> void {
	/*
		* we should NOT be able to bind a reference twice to the same object.
		* we should also only be able to bind if not expired. if this ref is
		  already binded, it should unbind before re-binding.
	*/
	SCL_ASSERT_MSG(&status, "ref %p: bind to null!", this);
	SCL_ASSERT_MSG(status.alive(), "ref %p: bind to dead pool entry!", this);
	SCL_ASSERT_MSG(&status != mStatus, "ref %p: double-binded to status %p!", this, status);
	if(!expired()) unbind();
	mStatus = &status;

	// create link (add to tail, if needed) -------------@/
	if(auto tailRef = mStatus->mReflistTail) {
		// this case is used if adding to existing list
		mLLPrev = tailRef;
		mLLNext = nullptr;
		tailRef->mLLNext = this;
	} else {
		// this case is used if the list must be created
		mLLPrev = nullptr;
		mLLNext = nullptr;
	}
	mStatus->mReflistTail = this;
}
template<typename Type> auto Ref<Type>::unbind() -> void {
	if(mStatus) {
		if(mStatus->mReflistTail == this) {
			mStatus->mReflistTail = mLLPrev;
		}

		// remove link ----------------------------------@/
		if(mLLPrev) { mLLPrev->mLLNext = mLLNext; }
		if(mLLNext) { mLLNext->mLLPrev = mLLPrev; }
	}
	mLLPrev = nullptr;
	mLLNext = nullptr;
	mStatus = nullptr;
}
template<typename Type> constexpr auto Ref<Type>::get() const -> Type* {
	return mStatus->get();
}
template<typename Type> constexpr auto Ref<Type>::del() -> void {
	mStatus->del();
}
// status structure's methods ---------------------------@/
template<typename Type> constexpr auto Status<Type>::get() const -> Type* {
	return &mManager->mTableObject[mID];
}
template<typename Type> constexpr auto Status<Type>::del() -> void {
	mManager->del_status(this);
}
template<typename Type> constexpr auto Status<Type>::reflist_clear() -> void {
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
		using TypeStatus = Status<Type>;
	public:
		Type* mTableObject;
		TypeStatus* mTableStatus;
		bool mAutoEnable;
		size_t mAliveNow;
		size_t mAliveMax;
		size_t mIdxLast;

		constexpr auto size() const -> size_t { return mAliveNow; }
		constexpr auto max_size() const -> size_t { return mAliveMax; }

		// construct/destructor -------------------------@/
		Manager() {
			mTableObject = nullptr;
			mTableStatus = nullptr;
			mIdxLast = 0;
			mAliveNow = 0;
			mAliveMax = 0;
			mAutoEnable = false;
		}
		Manager(size_t max) {
			mTableObject = nullptr;
			mTableStatus = nullptr;
			mIdxLast = 0;
			mAliveNow = 0;
			mAliveMax = max;
			mAutoEnable = true;

			mTableObject = static_cast<Type*>(std::malloc(sizeof(Type) * mAliveMax));
			mTableStatus = new TypeStatus[max];

			setup();
		}
		Manager(Type* objects, TypeStatus* status, size_t max) {
			mTableObject = nullptr;
			mTableStatus = nullptr;
			mIdxLast = 0;
			mAliveNow = 0;
			mAliveMax = max;
			mAutoEnable = false;

			mTableObject = objects;
			mTableStatus = status;

			setup();
		}

		~Manager() {
			// delete all objects --------------------------------@/
			for(size_t i=0; i<max_size(); i++) {
				auto& status = mTableStatus[i];
				if(status.alive()) {
					del_status(&status);
				}
			}

			// deallocate ----------------------------------------@/
			if(mAutoEnable) {
				if(mTableObject) { std::free(mTableObject); }
				if(mTableStatus) { delete[] mTableStatus; }
			}
			mTableObject = nullptr;
			mTableStatus = nullptr;

			mIdxLast = 0;
			mAliveNow = 0;
			mAliveMax = 0;
		}

		// adding fns -----------------------------------------@/
		auto del_status(TypeStatus* status) {
			SCL_ASSERT_MSG(status, "pool %p: attempt to delete NULL status!", this);
			SCL_ASSERT_MSG(status->alive(), "pool %p: attempt to delete dead status %p!", this, status);
			
			// deconstruct object -----------------------@/
			mTableObject[status->mID].~Type();

			// clear references -------------------------@/
			status->reflist_clear();

			// remove from free list --------------------@/
			status->mAlive = false;
			status->mIDNext = mIdxLast;
			mAliveNow--;
			mIdxLast = status->mID;
		}
		constexpr auto add_ref() -> TypeRef {
			return TypeRef(*add_status());
		}
		auto add_status() -> TypeStatus* {
			// get free object --------------------------@/
			SCL_ASSERT_MSG(mTableObject,"pool %p: didn't setup!",this);
			SCL_ASSERT_MSG(mAliveNow != mAliveMax,"pool %p: object over!",this);
			auto last_idx = mIdxLast;

			// add to free list -------------------------@/
			auto& status = mTableStatus[last_idx];
			mIdxLast = status.mIDNext;
			mAliveNow++;
			status.mAlive = true;

			// create the object it points to -----------@/
			new(&mTableObject[last_idx]) Type();
			return &status;
		}

		// list operations ------------------------------------@/
		auto get_tableAlive() -> std::vector<TypeRef> {
			size_t limit = size();
			std::vector<TypeRef> table;

			for(size_t i=0; i<max_size() && table.size() < limit; i++) {
				auto& status = &mTableStatus[i];
				if(status.alive()) {
					table.push_back(TypeRef(&status));
				}
			}

			return table;
		}
	private:
		auto setup() -> void {
			SCL_ASSERT_MSG(mAliveMax>0,"pool %p: max alive must be > 0",this);
			SCL_ASSERT_MSG(mAliveNow==0,"pool %p: num. alive must be 0",this);

			if(mAutoEnable) {
				SCL_ASSERT_MSG(mTableObject,"pool %p: objects must exist",this);
				SCL_ASSERT_MSG(mTableStatus,"pool %p: status must exist",this);
			}
			
			for(size_t i=0; i<mAliveMax; i++) {
				auto &status = mTableStatus[i];
				status.mAlive = false;
				status.mID = i & 0xFFFF;
				status.mIDNext = (i+1) & 0xFFFF;
				status.mManager = this;
				status.mReflistTail = nullptr;
			}
			mTableStatus[mAliveMax-1].mID = 0xFFFF;
		}
};

// ------------------------------------------------------@/
} // namespace pool
} // namespace scl
// ------------------------------------------------------@/

