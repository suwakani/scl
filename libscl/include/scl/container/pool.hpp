#pragma once

/*
	*	unlike previous implementations, PoolStatus will hold a list of
		pointers to weak references. you may use this to safely deal with
		references to objects in the pool.
	*	getting a reference is done via:
		auto obj = pool.add_ref();
		auto ref = scl::PoolRef(
*/

#include <cstdint>
#include <scl/basis/errhandle.hpp>

namespace scl {

template<typename Type> class Pool;
template<typename Type> class PoolRef;
template<typename Type> struct PoolStatus;

template<typename Type> class PoolRef {
	public:
		PoolStatus<Type>* mStatus;
		PoolRef<Type>* mLLPrev;
		PoolRef<Type>* mLLNext;

		constexpr PoolRef() : mStatus(NULL),mLLPrev(NULL),mLLNext(NULL) {}
		constexpr PoolRef(PoolStatus<Type>* status) {
			mStatus = NULL;
			mLLPrev = NULL;
			mLLNext = NULL;
			bind(status);
		}
		constexpr ~PoolRef() {
			unbind();
		}

		auto bind(PoolStatus<Type>* status) -> void;
		auto unbind() -> void;
		constexpr auto get() -> Type*;

		constexpr auto expired() -> bool { 
			return mStatus == nullptr;
		}
		constexpr auto lock() -> Type* {
			SCL_ASSERT_MSG(expired(), "ref %p: attempted to lock null ref!", this);
			return get();
		}
};

template<typename Type> struct PoolStatus {
	uint32_t mAlive;
	uint16_t mID,mIDNext;
	Pool<Type>* mPool;
	PoolRef<Type>* mReflistTail;
	constexpr auto alive() const -> bool { return mAlive; }
	constexpr auto id() const -> std::size_t { return mID; }
	constexpr auto get() -> Type*;

	PoolStatus() : mAlive(false),mID(0),mIDNext(0),
		mPool(NULL),mReflistTail(NULL) {}
};

template<typename Type> auto PoolRef<Type>::bind(PoolStatus<Type>* status) -> void {
	SCL_ASSERT_MSG(status, "ref %p: bind to null!", this);
	SCL_ASSERT_MSG(status->mAlive, "ref %p: bind to dead pool entry!", this);
	if(expired()) unbind();
	mStatus = status;

	// create link (add to tail, if needed) -------------@/
	auto tailRef = status->mReflistTail;
	if(tailRef) {
		// this case is used if adding to existing list
		mLLPrev = tailRef;
		mLLNext = nullptr;
		tailRef->mLLNext = this;
	} else {
		// this case is used if the list must be created
		mLLPrev = nullptr;
		mLLNext = nullptr;
	}
	status->mReflistTail = this;
}
template<typename Type> auto PoolRef<Type>::unbind() -> void {
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
template<typename Type> constexpr auto PoolRef<Type>::get() -> Type* {
	return mStatus->get();
}

template <typename Type> class Pool {
	public:
		using TypeRef = PoolRef<Type>;
		using TypeStatus = PoolStatus<Type>;
	public:
		Type* mObjects;
		TypeStatus* mStatus;
		bool mAutoEnable;
		size_t mAliveNow;
		size_t mAliveMax;
		size_t mIdxLast;

		// construct/destructor -------------------------@/
		Pool() {
			mObjects = nullptr;
			mStatus = nullptr;
			mAutoEnable = false;
			mIdxLast = 0;
			mAliveNow = 0;
			mAliveMax = 0;
		}
		Pool(size_t max) {
			mObjects = nullptr;
			mStatus = nullptr;
			mAutoEnable = true;
			mIdxLast = 0;
			mAliveNow = 0;
			mAliveMax = max;

			mObjects = static_cast<Type*>(std::malloc(sizeof(Type) * mAliveMax));
			mStatus = new TypeStatus[max];

			setup();
		}
		Pool(Type* objects, TypeStatus* status, size_t max) {
			mObjects = nullptr;
			mStatus = nullptr;
			mAutoEnable = false;
			mIdxLast = 0;
			mAliveNow = 0;
			mAliveMax = max;

			mObjects = objects;
			mStatus = status;

			setup();
		}

		~Pool() {
			if(mObjects) {
				std::free(mObjects);
				mObjects = nullptr;
			}
			if(mStatus) {
				delete[] mStatus;
				mStatus = nullptr;
			}

			mIdxLast = 0;
			mAliveNow = 0;
			mAliveMax = 0;
		}

		auto del_status(TypeStatus* status) {
			SCL_ASSERT_MSG(status, "pool %p: attempt to delete NULL status!", this);
			SCL_ASSERT_MSG(status->mAlive, "pool %p: attempt to delete dead status %p!", this, status);
			
			// deconstruct object -----------------------@/
			mObjects[status->mID].~Type();

			// clear references -------------------------@/
			while(status->mReflistTail) {
				status->mReflistTail->unbind();
			}

			// remove from free list --------------------@/
			status->mAlive = false;
			status->mIDNext = mIdxLast;
			mAliveNow--;
			mIdxLast = status->mID;
		}
		auto add_status() -> TypeStatus* {
			// get free object --------------------------@/
			SCL_ASSERT_MSG(mObjects,"pool %p: didn't setup!",this);
			SCL_ASSERT_MSG(mAliveNow != mAliveMax,"pool %p: object over!",this);
			auto last_idx = mIdxLast;

			// add to free list -------------------------@/
			auto& status = mStatus[last_idx];
			mIdxLast = status.mIDNext;
			mAliveNow++;
			status.mAlive = true;

			// create the object it points to -----------@/
			new(&mObjects[last_idx]) Type();
			return &status;
		}
		/*auto add() -> T* {
			auto status = add_status();
			return mObjects[T];
		}
		*/

	private:
		auto setup() -> void {
			SCL_ASSERT_MSG(mAliveMax>0,"pool %p: max alive must be > 0",this);
			SCL_ASSERT_MSG(mAliveNow==0,"pool %p: num. alive must be 0",this);

			if(mAutoEnable) {
				SCL_ASSERT_MSG(mObjects,"pool %p: objects must exist",this);
				SCL_ASSERT_MSG(mStatus,"pool %p: status must exist",this);
			}
			
			for(size_t i=0; i<mAliveMax; i++) {
				auto &status = mStatus[i];
				status.mAlive = false;
				status.mID = i & 0xFFFF;
				status.mIDNext = (i+1) & 0xFFFF;
				status.mPool = this;
				status.mReflistTail = nullptr;
			}
			mStatus[mAliveMax-1].mID = 0xFFFF;
		}
};

template<typename Type> constexpr auto PoolStatus<Type>::get() -> Type* {
	return &mPool->mObjects[mID];
}
	
} // namespace scl

