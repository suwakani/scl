// ==========================================================================@/
//                                                                           @/
// -- task system ---------------------------------------------------------- @/
//                                                                           @/
// ==========================================================================@/
#pragma once
#include <scl/basis/errhandle.hpp>
#include <scl/container/pool.hpp>
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

// ==========================================================================@/
//                                                                           @/
// -- Single task unit ----------------------------------------------------- @/
//                                                                           @/
// ==========================================================================@/
class Unit {
	public:
		size_t mPriority;
		std::string mName;
		UnitRef mRefSelf;
		UnitRef mTreeParent;
		UnitList mTreeChildren;
		Callback mCallback;
		Manager* mManager;
		bool mShouldDel;
		bool mIsStepping;

		// con/destructor -------------------------------@/
		Unit();
		Unit(size_t priority, Callback fn, const std::string& name);
		~Unit();

		// modifiers ------------------------------------------@/
		constexpr auto selfref() -> UnitRef { return mRefSelf; }
		constexpr auto selfref_assign(UnitRef ref) -> void { mRefSelf = ref; }
		constexpr auto manager() -> Manager* { return mManager; }
		constexpr auto manager_assign(Manager* mgr) -> void { mManager = mgr; }

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

