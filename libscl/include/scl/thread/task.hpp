// ==========================================================================@/
//                                                                           @/
// -- task system ---------------------------------------------------------- @/
//                                                                           @/
// ==========================================================================@/
#pragma once
#include <scl/basis/errhandle.hpp>
#include <scl/container/pool.hpp>
#include <bitset>
#include <functional>
#include <list>
#include <vector>
// ------------------------------------------------------@/
namespace scl {
namespace task {
// ------------------------------------------------------@/
class Unit;
using UnitRef = scl::pool::Ref<Unit>;
using UnitList = std::list<UnitRef>;
using Callback = std::function<void(Unit&)>;
class Manager;
// ------------------------------------------------------@/
constexpr size_t PriorityMax = 16;
// ------------------------------------------------------@/
namespace UnitFlag {
	enum {
		Paused,
		ShouldDel,
		IsStepping,
	};
};

// ==========================================================================@/
//                                                                           @/
// -- single task unit ----------------------------------------------------- @/
//                                                                           @/
// ==========================================================================@/
class Unit {
	public:
		std::string mName;
		UnitRef mRefSelf;
		UnitRef mTreeParent;
		UnitList mTreeChildren;
		Callback mCallback;
		Manager* mManager;
		uint16_t mPriority;
		std::bitset<16> mFlags;
		float mStepTimer;
		float mStepFrequency;

		// con/destructor -------------------------------@/
		Unit();
		Unit(size_t priority, Callback fn, const std::string& name);
		~Unit();

		// access ---------------------------------------@/
		constexpr auto selfref() -> UnitRef& { return mRefSelf; }
		constexpr auto selfref_assign(const UnitRef& ref) -> void { mRefSelf = ref; }
		constexpr auto manager() -> Manager* { return mManager; }
		constexpr auto manager_assign(Manager* mgr) -> void { mManager = mgr; }

		constexpr auto flagPaused_get() const -> bool { return mFlags[UnitFlag::Paused]; }
		constexpr auto flagPaused_set(bool v) -> void { mFlags[UnitFlag::Paused]; }
		constexpr auto flagShouldDel_get() const -> bool { return mFlags[UnitFlag::ShouldDel]; }
		constexpr auto flagShouldDel_set(bool v) -> void { mFlags[UnitFlag::ShouldDel]; }
		constexpr auto flagIsStepping_get() const -> bool { return mFlags[UnitFlag::IsStepping]; }
		constexpr auto flagIsStepping_set(bool v) -> void { mFlags[UnitFlag::IsStepping]; }

		constexpr auto pause() -> void { flagPaused_set(true); }
		constexpr auto resume() -> void { flagPaused_set(false); }
		constexpr auto paused() const -> bool { return flagPaused_get(); }

		constexpr auto name() const -> const std::string& { return mName; }
		constexpr auto priority() const -> int { return mPriority; }

		constexpr auto stepTimer_set(float tmr) -> void { mStepTimer = tmr; }
		constexpr auto stepTimer_reset() -> void { stepTimer_set(1 - mStepFrequency); }
		constexpr auto stepFrequency_set(float freq, bool doResetTimer = false) -> void {
			mStepFrequency = freq;
			if(doResetTimer) {
				// * this is so a task always updates at least once and then
				//   has blank steps after
				stepTimer_reset();
			}
		}

		// stepping -------------------------------------@/
		auto step() -> void;
		auto tostr_tree() -> std::string;

		// tree -----------------------------------------@/
		auto children_get(bool sorted=false) -> std::vector<UnitRef>;
		constexpr auto children_getSorted() -> std::vector<UnitRef> {
			return children_get(true);
		}
		constexpr auto del() -> void;
};

// ==========================================================================@/
//                                                                           @/
// -- task manager --------------------------------------------------------- @/
//                                                                           @/
// ==========================================================================@/
class Manager {
	public:
		scl::pool::Manager<Unit> mUnitPool;
		bool mDidSetup;

		// con/destructor -------------------------------@/
		Manager();
		Manager(size_t max_tasks);
		~Manager() {} // not needed, pool destructs itself

		// access ---------------------------------------@/
		constexpr auto size() const -> size_t { return mUnitPool.size(); }
		constexpr auto max_size() const -> size_t { return mUnitPool.max_size(); }
		constexpr auto totalsize_unit() const -> size_t { return mUnitPool.totalsize_object(); }

		// tree -----------------------------------------@/
		auto unit_add(size_t priority, UnitRef parent, Callback fn, const std::string& name) -> UnitRef;
		auto unit_del(UnitRef unit) -> void;
		constexpr auto unit_addOrphan(Callback fn, const std::string& name) -> UnitRef {
			return unit_add(0, nullptr, fn, name);
		}
};
// ------------------------------------------------------@/
constexpr auto Unit::del() -> void {
	SCL_ASSERT_MSG(manager(), "task %p: no manager!", this);
	manager()->unit_del(selfref());
}

// ------------------------------------------------------@/
} // namespace task
} // namespace scl
// ------------------------------------------------------@/

