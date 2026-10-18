// ==========================================================================@/
//                                                                           @/
// -- task system ---------------------------------------------------------- @/
//                                                                           @/
// ==========================================================================@/
#include <scl/thread/task.hpp>
// ------------------------------------------------------@/
namespace scl {
namespace task {

// ==========================================================================@/
//                                                                           @/
// -- Single task unit ----------------------------------------------------- @/
//                                                                           @/
// ==========================================================================@/
Unit::Unit() {
	mManager = nullptr;
	mTreeChildren.clear();

	mPriority = 0;
	mName = "UNNAMED";
	mShouldDel = false;
	mIsStepping = false;
}
Unit::Unit(size_t priority, Callback fn, const std::string& name) {
	mManager = nullptr;
	mTreeChildren.clear();

	mPriority = priority;
	mCallback = fn;
	mName = name;
	mShouldDel = false;
	mIsStepping = false;
}
Unit::~Unit() {
}
// ------------------------------------------------------@/
auto Unit::step() -> void {
	// execute current task -----------------------------@/
	mIsStepping = true;
	mCallback(*this);
	mIsStepping = false;

	// handle deleting ----------------------------------@/
	// NOTE: supposedly safe ? https://stackoverflow.com/questions/20236610/can-i-call-destructor-from-its-class-method
	if(mShouldDel) {
		std::puts("let's get out!");
		mManager->unit_del(selfref());
		return;
	}

	// execute child tasks ------------------------------@/
	std::vector<UnitRef> children;
	for(auto& childRef : mTreeChildren) {
		children.push_back(childRef);
	}

	for(auto& childRef : children) {
		childRef->step();
	}
}
// ------------------------------------------------------@/
auto Unit::del() -> void {
	SCL_ASSERT_MSG(manager(), "task %p: no manager!", this);
	manager()->unit_del(selfref());
}

// ==========================================================================@/
//                                                                           @/
// -- Task manager --------------------------------------------------------- @/
//                                                                           @/
// ==========================================================================@/
Manager::Manager() {
	mDidSetup = false;
}
Manager::Manager(size_t max_tasks) {
	SCL_ASSERT_MSG(max_tasks > 0, "taskmgr %p: no. of tasks must be > 0!", this);
	mDidSetup = true;
	mUnitPool.setup_auto(max_tasks);
}
Manager::~Manager() {
	
}

// ------------------------------------------------------@/
auto Manager::unit_add(size_t priority, UnitRef parentRef, Callback fn, const std::string& name) -> UnitRef {
	SCL_ASSERT_MSG(priority < PriorityMax, "taskmgr %p: cant add task w/ priority >= %d!", this, PriorityMax);
	if(parentRef.valid()) {
		SCL_ASSERT_MSG(parentRef->manager() == this, "taskmgr %p: parent manager mismatch!", this);
	}

	// create new unit ----------------------------------@/
	auto unitRef = mUnitPool.add_ref(priority, fn, name);
	auto unit = unitRef.lock();

	unit->selfref_assign(unitRef);
	unit->manager_assign(this);

	// setup hierarchy for unit -------------------------@/
	if(parentRef.valid()) {
		auto parent = parentRef.lock();
		unit->mTreeParent = parentRef;
		parent->mTreeChildren.push_back(unitRef);
	}

	return unitRef;
}
auto Manager::unit_del(UnitRef unit) -> void {
	if(unit->mIsStepping) {
		unit->mShouldDel = true;
		return;
	}
	SCL_ASSERT_MSG(unit.valid(), "taskmgr %p: attempt to delete invalid ref!", this);
	
	// remove all children of this unit, too ------------@/
	{
		std::vector<UnitRef> children;
		for(auto& childRef : unit->mTreeChildren) {
			children.push_back(childRef);
		}

		for(auto& childRef : children) {
			unit_del(childRef);
		}
	}

	// remove from unit's parent ------------------------@/
	if(unit->mTreeParent.valid()) {
		auto parentRef = unit->mTreeParent;
		auto& treeChildren = parentRef->mTreeChildren;
		UnitList::iterator it = treeChildren.begin();
		while(it != treeChildren.end()) {
			if((*it) == unit) {
				treeChildren.erase(it);
				break;
			} else {
				it++;
			}
		}
	}
}

// ------------------------------------------------------@/
} // namespace task
} // namespace scl
// ------------------------------------------------------@/

