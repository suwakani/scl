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
	flagShouldDel_set(false);
	flagIsStepping_set(false);
}
Unit::Unit(size_t priority, Callback fn, const std::string& name) {
	mManager = nullptr;
	mTreeChildren.clear();

	mPriority = priority;
	mCallback = fn;
	mName = name;
	flagShouldDel_set(false);
	flagIsStepping_set(false);
}
Unit::~Unit() {
}
// ------------------------------------------------------@/
auto Unit::step() -> void {
	// execute current task -----------------------------@/
	if(!paused()) {
		flagIsStepping_set(true);
		mCallback(*this);
		flagIsStepping_set(false);
	}

	// handle deleting ----------------------------------@/
	// NOTE: supposedly safe ? https://stackoverflow.com/questions/20236610/can-i-call-destructor-from-its-class-method
	if(flagShouldDel_get()) {
		std::printf("let's get out! (children: %zu)\n", mTreeChildren.size());
		mManager->unit_del(selfref());
		return;
	}

	// execute child tasks ------------------------------@/
	std::vector<UnitRef> children;
	for(auto& childRef : mTreeChildren) {
		children.push_back(childRef);
	}

    std::sort(children.begin(), children.end(), [](UnitRef& a, UnitRef& b) {
		return a->priority() < b->priority();
	});

	for(auto& childRef : children) {
		childRef->step();
	}
}
auto Unit::tostr_tree() -> std::string {
	std::string fullstr;

	std::function<void(UnitRef&,int)> cb = [&](UnitRef& unitRef, int depth) {
		auto unit = unitRef.lock();
		for(int i=0; i<depth; i++) {
			fullstr += "    ";
		}
		fullstr += std::format("* [{0:02X}h] '{1:24}'", unit->priority(),unit->name());
		fullstr += "\n";

		for(auto& childRef : unit->mTreeChildren) {
			cb(childRef,depth+1);
		}
	};

	cb(selfref(),0);
	return fullstr.substr(0,fullstr.size()-1);
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
	auto task_table = mUnitPool.get_tableAlive();
	for(auto& unitRef : task_table) {
		if(unitRef.valid()) {
			unitRef.del();
		}
	}
}

// ------------------------------------------------------@/
auto Manager::unit_add(size_t priority, UnitRef parentRef, Callback fn, const std::string& name) -> UnitRef {
	SCL_ASSERT_MSG(mDidSetup, "taskmgr %p: not initialized yet", this);
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
	if(unit->flagIsStepping_get()) {
		unit->flagShouldDel_set(true);
		return;
	}
	SCL_ASSERT_MSG(mDidSetup, "taskmgr %p: not initialized yet", this);
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

	// remove ref from pool -----------------------------@/
	unit->selfref().del();
}

// ------------------------------------------------------@/
} // namespace task
} // namespace scl
// ------------------------------------------------------@/

