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
// ------------------------------------------------------@/
namespace scl {
namespace task {
// ------------------------------------------------------@/
class Unit;
using UnitRef = scl::pool::Ref<Unit>;
using UnitList = std::list<UnitRef>;
class Manager;
using Callback = std::function<void(Unit&)>;
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
// -- Single task unit ----------------------------------------------------- @/
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

		// con/destructor -------------------------------@/
		Unit();
		Unit(size_t priority, Callback fn, const std::string& name);
		~Unit();

		// modifiers ------------------------------------------@/
		constexpr auto selfref() -> UnitRef { return mRefSelf; }
		constexpr auto selfref_assign(UnitRef ref) -> void { mRefSelf = ref; }
		constexpr auto manager() -> Manager* { return mManager; }
		constexpr auto manager_assign(Manager* mgr) -> void { mManager = mgr; }

		constexpr auto flagPaused_get() const -> bool { return mFlags[UnitFlag::Paused]; }
		constexpr auto flagPaused_set(bool v) -> void { mFlags[UnitFlag::Paused]; }
		constexpr auto flagShouldDel_get() const -> bool { return mFlags[UnitFlag::ShouldDel]; }
		constexpr auto flagShouldDel_set(bool v) -> void { mFlags[UnitFlag::ShouldDel]; }
		constexpr auto flagIsStepping_get() const -> bool { return mFlags[UnitFlag::IsStepping]; }
		constexpr auto flagIsStepping_set(bool v) -> void { mFlags[UnitFlag::IsStepping]; }
		constexpr auto paused() const -> bool { return flagPaused_get(); }

		// stepping -------------------------------------@/
		auto step() -> void;

		// tree -----------------------------------------@/
		auto del() -> void;
};

// ==========================================================================@/
//                                                                           @/
// -- Task manager --------------------------------------------------------- @/
//                                                                           @/
// ==========================================================================@/
class Manager {
	public:
		scl::pool::Manager<Unit> mUnitPool;
		bool mDidSetup;

		constexpr auto size() const -> size_t { return mUnitPool.size(); }
		constexpr auto max_size() const -> size_t { return mUnitPool.max_size(); }
		constexpr auto totalsize_unit() const -> size_t { return mUnitPool.totalsize_object(); }

		// con/destructor -------------------------------@/
		Manager();
		Manager(size_t max_tasks);
		~Manager();

		// tree -----------------------------------------@/
		auto unit_add(size_t priority, UnitRef parent, Callback fn, const std::string& name) -> UnitRef;
		auto unit_del(UnitRef unit) -> void;
};

// ------------------------------------------------------@/
} // namespace task
} // namespace scl
// ------------------------------------------------------@/

