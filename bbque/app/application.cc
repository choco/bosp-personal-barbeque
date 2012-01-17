/**
*       @file  application.cc
*      @brief  Application descriptor implementation
*
* This implements the application descriptor.
* Such descriptor includes static and dynamic information upon application
* execution. It embeds usual information about name, priority, user, PID
* (could be different from the one given by OS) plus a reference to the
* recipe object, the list of enabled working modes and resource constraints.
*
*     @author  Giuseppe Massari (jumanix), joe.massanga@gmail.com
*
*   @internal
*     Created  06/04/2011
*    Revision  $Id: doxygen.templates,v 1.3 2010/07/06 09:20:12 mehner Exp $
*    Compiler  gcc/g++
*     Company  Politecnico di Milano
*   Copyright  Copyright (c) 2011, Giuseppe Massari
*
* This source code is released for free distribution under the terms of the
* GNU General Public License as published by the Free Software Foundation.
* ============================================================================
*/

#include "bbque/app/application.h"

#include <limits>

#include "bbque/application_manager.h"
#include "bbque/modules_factory.h"
#include "bbque/plugin_manager.h"
#include "bbque/app/overheads.h"
#include "bbque/app/working_mode.h"
#include "bbque/res/resource_accounter.h"

namespace ba = bbque::app;
namespace br = bbque::res;
namespace bp = bbque::plugins;

namespace bbque { namespace app {

char const *ApplicationStatusIF::stateStr[] = {
	"DISABLED",
	"READY",
	"SYNC",
	"RUNNING",
	"FINISHED"
};

char const *ApplicationStatusIF::syncStateStr[] = {
	"STARTING",
	"RECONF",
	"MIGREC",
	"MIGRATE",
	"BLOCKED",
	"NONE"
};

// Compare two working mode values.
// This is used to sort the list of enabled working modes.
bool AwmValueLesser(const AwmPtr_t & wm1, const AwmPtr_t & wm2) {
		return wm1->Value() < wm2->Value();
}

bool AwmIdLesser(const AwmPtr_t & wm1, const AwmPtr_t & wm2) {
		return wm1->Id() < wm2->Id();
}


/**
 * The following map keeps track of the constraints on resources.
 * It is used by function UsageOutOfBounds() (see below) to check if a working
 * mode includes a resource usages that violates a bounds contained in this
 * map.
 **/
static ConstrMap_t rsrc_constraints;


Application::Application(std::string const & _name, AppPid_t _pid,
		uint8_t _exc_id) :
	name(_name),
	pid(_pid),
	exc_id(_exc_id) {

	// Init the working modes vector
	awms.recipe_vect.resize(MAX_NUM_AWM);

	// Get a logger
	bp::LoggerIF::Configuration conf(APPLICATION_NAMESPACE);
	logger = ModulesFactory::GetLoggerModule(std::cref(conf));
	assert(logger);

	// Format the EXC string identifier
	::snprintf(str_id, 16, "%05d:%6s:%02d",
			Pid(), Name().substr(0,6).c_str(), ExcId());

	// Initialized scheduling state
	schedule.state = DISABLED;
	schedule.preSyncState = DISABLED;
	schedule.syncState = SYNC_NONE;

	logger->Info("Built new EXC [%s]", StrId());
}

Application::~Application() {
	br::ResourceAccounter &ra(br::ResourceAccounter::GetInstance());
	ApplicationManager &am(ApplicationManager::GetInstance());
	logger->Debug("Destroying EXC [%s]", StrId());

	// Release resources
	if (CurrentAWM())
		ra.ReleaseResources(am.GetApplication(Uid()));

	// Release the recipe used
	recipe.reset();

	// Reset scheduling info
	schedule.awm.reset();

	// Releasing AWMs and ResourceConstraints...
	awms.recipe_vect.clear();
	awms.enabled_list.clear();
	rsrc_constraints.clear();
}

void Application::SetPriority(AppPrio_t _prio) {
	bbque::ApplicationManager &am(bbque::ApplicationManager::GetInstance());

	// If _prio value is greater then the lowest priority
	// (maximum integer value) it is trimmed to the last one.
	priority = std::min(_prio, am.LowestPriority());
}

void Application::InitWorkingModes(AppPtr_t & papp) {
	// Get the working modes from recipe and init the vector size
	AwmPtrVect_t const & rcp_awms(recipe->WorkingModesAll());

	// Init AWM range attributes (for AWM constraints)
	awms.max_id = rcp_awms.size() - 1;
	awms.low_id = 0;
	awms.upp_id = awms.max_id;
	awms.enabled_bset.set();
	logger->Debug("InitWorkingModes: enabled bitset = %s",
			awms.enabled_bset.to_string().c_str());
	logger->Debug("InitWorkingModes: max_id = %d", awms.max_id);

	for (int i = 0; i <= awms.max_id; ++i) {
		// Copy the working mode and set the owner (current Application)
		AwmPtr_t app_awm(new WorkingMode(*rcp_awms[i]));
		assert(!app_awm->Owner());
		app_awm->SetOwner(papp);

		// Insert the working mode into the structures
		awms.recipe_vect[app_awm->Id()] = app_awm;
		awms.enabled_list.push_back(app_awm);
	}

	// Sort the enabled list by "value"
	awms.enabled_list.sort(AwmValueLesser);
}

void Application::InitResourceConstraints() {
	ConstrMap_t::const_iterator cons_it(recipe->ConstraintsAll().begin());
	ConstrMap_t::const_iterator end_cons(recipe->ConstraintsAll().end());

	// For each static constraint on a resource make an assertion
	for (; cons_it != end_cons; ++cons_it) {
		std::string const & rsrc_path(cons_it->first);
		ResourceConstrPtr_t const & rsrc_constr(cons_it->second);

		// Lower bound
		if (rsrc_constr->lower > 0)
				SetResourceConstraint(rsrc_path,
						ResourceConstraint::LOWER_BOUND, rsrc_constr->lower);
		// Upper bound
		if (rsrc_constr->upper > 0)
				SetResourceConstraint(rsrc_path,
						ResourceConstraint::UPPER_BOUND, rsrc_constr->upper);
	}

	logger->Debug("%d resource constraints from the recipe",
			rsrc_constraints.size());
}

Application::ExitCode_t
Application::SetRecipe(RecipePtr_t & _recipe, AppPtr_t & papp) {
	// Check if the recipe object is null
	if (!_recipe) {
		logger->Error("SetRecipe: null recipe object");
		return APP_RECP_NULL;
	}

	// Set the recipe that the application will use
	recipe = _recipe;
	// Set the static priority
	priority = recipe->GetPriority();
	// Set the working modes
	InitWorkingModes(papp);
	// Are there any resource constraints?
	InitResourceConstraints();
	// Specific attributes (i.e. plugin specific)
	attributes = AttributesMap_t(recipe->attributes);

	// Debug messages
	logger->Info("%d working modes (enabled = %d).", awms.recipe_vect.size(),
			awms.enabled_list.size());
	logger->Info("%d constraints in the application.",
			rsrc_constraints.size());
	logger->Info("%d plugins specific attributes.", attributes.size());

	return APP_SUCCESS;
}

AwmPtrList_t::iterator Application::FindWorkingModeIter(
		AwmPtrList_t & awm_list,
		uint16_t wmId) {
	AwmPtrList_t::iterator awm_it(awm_list.begin());
	AwmPtrList_t::iterator end_awm(awm_list.end());

	for (; awm_it != end_awm; ++awm_it) {
		if ((*awm_it)->Id() == wmId)
			break;
	}

	return awm_it;
}


/*******************************************************************************
 *  EXC State and SyncState Management
 ******************************************************************************/

bool Application::_Disabled() const {
	return ((_State() == DISABLED) ||
			(_State() == FINISHED));
}

bool Application::Disabled() {
	std::unique_lock<std::recursive_mutex> state_ul(schedule.mtx);
	return _Disabled();
}

bool Application::_Active() const {
	return ((schedule.state == READY) ||
			(schedule.state == RUNNING));
}

bool Application::Active() {
	std::unique_lock<std::recursive_mutex> state_ul(schedule.mtx);
	return _Active();
}

bool Application::_Synching() const {
	return (schedule.state == SYNC);
}

bool Application::Synching() {
	std::unique_lock<std::recursive_mutex> state_ul(schedule.mtx);
	return _Synching();
}

bool Application::_Blocking() const {
	return (_Synching() && (_SyncState() == BLOCKED));
}

bool Application::Blocking() {
	std::unique_lock<std::recursive_mutex> state_ul(schedule.mtx);
	return _Blocking();
}

Application::State_t Application::_State() const {
	return schedule.state;
}

Application::State_t Application::State() {
	std::unique_lock<std::recursive_mutex> state_ul(schedule.mtx);
	return _State();
}

Application::State_t Application::_PreSyncState() const {
	return schedule.preSyncState;
}

Application::State_t Application::PreSyncState() {
	std::unique_lock<std::recursive_mutex> state_ul(schedule.mtx);
	return _PreSyncState();
}

Application::SyncState_t Application::_SyncState() const {
	return schedule.syncState;
}

Application::SyncState_t Application::SyncState() {
	std::unique_lock<std::recursive_mutex> state_ul(schedule.mtx);
	return _SyncState();
}

AwmPtr_t const & Application::_CurrentAWM() const {
	return schedule.awm;
}

AwmPtr_t const & Application::CurrentAWM() {
	std::unique_lock<std::recursive_mutex> state_ul(schedule.mtx);
	return _CurrentAWM();
}

AwmPtr_t const & Application::_NextAWM() const {
	return schedule.next_awm;
}

AwmPtr_t const & Application::NextAWM() {
	std::unique_lock<std::recursive_mutex> state_ul(schedule.mtx);
	return _NextAWM();
}

// NOTE: this requires a lock on schedule.mtx
void Application::SetSyncState(SyncState_t sync) {

	logger->Debug("Changing sync state [%s, %d:%s => %d:%s]",
			StrId(),
			_SyncState(), SyncStateStr(_SyncState()),
			sync, SyncStateStr(sync));

	schedule.syncState = sync;
}

// NOTE: this requires a lock on schedule.mtx
void Application::SetState(State_t state, SyncState_t sync) {
	bbque::ApplicationManager &am(bbque::ApplicationManager::GetInstance());
	AppPtr_t papp = am.GetApplication(Uid());

	logger->Debug("Changing state [%s, %d:%s => %d:%s]",
			StrId(),
			_State(), StateStr(_State()),
			state, StateStr(state));

	// Entering a Synchronization state
	if (state == SYNC) {
		assert(sync != SYNC_NONE);

		// Save a copy of the pre-synchronization state
		schedule.preSyncState = _State();

		// Update synchronization state
		SetSyncState(sync);

		// Update queue based on current application state
		am.NotifyNewState(papp, Application::SYNC);

		// Updating state
		schedule.state = Application::SYNC;

		return;
	}


	// Entering a statble state
	assert(sync == SYNC_NONE);

	// Update queue based on current application state
	am.NotifyNewState(papp, state);

	// Updating state
	schedule.preSyncState = state;
	schedule.state = state;

	// Update synchronization state
	SetSyncState(sync);

	// Release current selected AWM
	if ((state == DISABLED) ||
			(state == READY)) {
		schedule.awm.reset();
		schedule.next_awm.reset();
	}

}

/*******************************************************************************
 *  EXC Destruction
 ******************************************************************************/

Application::ExitCode_t Application::Terminate() {
	br::ResourceAccounter &ra(br::ResourceAccounter::GetInstance());
	ApplicationManager &am(ApplicationManager::GetInstance());
	std::unique_lock<std::recursive_mutex> state_ul(schedule.mtx);

	// Release resources
	if (_CurrentAWM())
		ra.ReleaseResources(am.GetApplication(Uid()));

	// Mark the application has finished
	SetState(FINISHED);
	state_ul.unlock();

	logger->Info("EXC [%s] FINISHED", StrId());

	return APP_SUCCESS;

}


/*******************************************************************************
 *  EXC Enabling
 ******************************************************************************/

Application::ExitCode_t Application::Enable() {
	std::unique_lock<std::recursive_mutex>
		state_ul(schedule.mtx, std::defer_lock);

	// Enabling the execution context
	logger->Debug("Enabling EXC [%s]...", StrId());

	state_ul.lock();

	// Not disabled applications could not be marked as READY
	if (!_Disabled()) {
		logger->Crit("Trying to enable already enabled application [%s] "
				"(Error: possible data structure curruption?)",
				StrId());
		assert(_Disabled());
		return APP_ABORT;
	}

	// Mark the application has ready to run
	SetState(READY);

	state_ul.unlock();

	logger->Info("EXC [%s] ENABLED", StrId());

	return APP_SUCCESS;
}


/*******************************************************************************
 *  EXC Disabled
 ******************************************************************************/

Application::ExitCode_t Application::Disable() {
	br::ResourceAccounter &ra(br::ResourceAccounter::GetInstance());
	ApplicationManager &am(ApplicationManager::GetInstance());
	std::unique_lock<std::recursive_mutex>
		state_ul(schedule.mtx, std::defer_lock);

	// Not disabled applications could not be marked as READY
	if (_Disabled()) {
		logger->Warn("Trying to disable already disabled application [%s]",
				StrId());
		return APP_SUCCESS;
	}

	state_ul.lock();

	// Release resources
	if (_CurrentAWM())
		ra.ReleaseResources(am.GetApplication(Uid()));

	// Mark the application has ready to run
	SetState(DISABLED);
	state_ul.unlock();

	logger->Info("EXC [%s] DISABLED", StrId());

	return APP_SUCCESS;
}


/*******************************************************************************
 *  EXC Optimization
 ******************************************************************************/

// NOTE: this requires a lock on schedule.mtx
Application::ExitCode_t Application::RequestSync(SyncState_t sync) {
	bbque::ApplicationManager &am(bbque::ApplicationManager::GetInstance());
	AppPtr_t papp = am.GetApplication(Uid());
	ApplicationManager::ExitCode_t result;

	if (!_Active()) {
		logger->Crit("Sync request FAILED (Error: wrong application status)");
		assert(_Active());
		return APP_ABORT;
	}

	logger->Debug("Request synchronization [%s, %d:%s]",
			StrId(), sync, SyncStateStr(sync));

	// Ensuring the AM has an hander for this application
	if (!papp) {
		logger->Crit("Request synchronization [%s, %d:%s] FAILED "
				"(Error: unable to get an application handler",
				StrId(), sync, SyncStateStr(sync));
		assert(papp);
		return APP_ABORT;
	}

	// Update our state
	SetState(SYNC, sync);

	// Request the application manager to synchronization this application
	// accorting to our new state
	result = am.SyncRequest(papp, sync);
	if (result != ApplicationManager::AM_SUCCESS) {
		logger->Error("Synchronization request FAILED (Error: %d)", result);
		// This is not an error on AWM scheduling but only on the notification
		// of the SynchronizationManager module. The AWM could still be
		// accepted.
	}

	logger->Info("Sync scheduled [%s, %d:%s]",
			StrId(), sync, SyncStateStr(sync));

	return APP_SUCCESS;

}

Application::SyncState_t
Application::SyncRequired(AwmPtr_t const & awm) {

	// This must be called only by running applications
	assert(_State() == RUNNING);
	assert(_CurrentAWM().get());

	// Check if the assigned operating point implies RECONF|MIGREC|MIGRATE
	if ((_CurrentAWM()->Id() != awm->Id()) &&
			(_CurrentAWM()->ClusterSet() != awm->ClusterSet())) {
		logger->Debug("SynchRequired: [%s] to MIGREC", StrId());
		return MIGREC;
	}

	if ((_CurrentAWM()->Id() == awm->Id()) &&
			(_CurrentAWM()->ClustersChanged())) {
		logger->Debug("SynchRequired: [%s] to MIGRATE", StrId());
		return MIGRATE;
	}

	if (_CurrentAWM()->Id() != awm->Id()) {
		logger->Debug("SynchRequired: [%s] to RECONF", StrId());
		return RECONF;
	}

	logger->Debug("SynchRequired: [%s] SYNC_NONE", StrId());
	// NOTE: By default no reconfiguration is assumed to be required, thus we
	// return the SYNC_STATE_COUNT which must be read as false values
	return SYNC_NONE;
}

Application::ExitCode_t
Application::Reschedule(AwmPtr_t const & awm) {
	SyncState_t sync;

	// Ready application could be synchronized to start
	if (_State() == READY)
		return RequestSync(STARTING);

	// Otherwise, the application should be running...
	if (_State() != RUNNING) {
		logger->Crit("Rescheduling FAILED (Error: wrong application status "
				"{%s/%s})", StateStr(_State()), SyncStateStr(_SyncState()));
		assert(_State() == RUNNING);
		return APP_ABORT;
	}

	// Checking if a synchronization is required
	sync = SyncRequired(awm);
	if (sync == SYNC_NONE)
		return APP_SUCCESS;

	// Request a synchronization for the identified reconfiguration
	return RequestSync(sync);
}

Application::ExitCode_t Application::Unschedule() {

	// Ready application remain into ready state
	if (_State() == READY)
		return APP_ABORT;

	// Check if the application has been already blocked by a previous failed
	// schedule request
	if (_Blocking())
		return APP_ABORT;

	// Otherwise, the application should be running...
	if (_State() != RUNNING) {
		logger->Crit("Rescheduling FAILED (Error: wrong application status)");
		assert(_State() == RUNNING);
		return APP_ABORT;
	}

	// The application should be blocked
	return RequestSync(BLOCKED);
}

Application::ExitCode_t Application::ScheduleRequest(AwmPtr_t const & awm,
		RViewToken_t vtok) {
	std::unique_lock<std::recursive_mutex> schedule_ul(schedule.mtx);
	br::ResourceAccounter &ra(br::ResourceAccounter::GetInstance());
	br::ResourceAccounter::ExitCode_t booking;
	AppPtr_t papp(awm->Owner());
	ExitCode_t result;

	// App is SYNC/BLOCKED for a previously failed scheduling.
	// Reset state and syncState for this new attempt.
	if (_Blocking())
		SetState(schedule.preSyncState, SYNC_NONE);

	logger->Debug("Schedule request for [%s] into AWM [%02d:%s]",
			papp->StrId(), awm->Id(), awm->Name().c_str());

	// Get the working mode pointer
	if (!awm) {
		logger->Crit("Schedule request for [%s] FAILED "
				"(Error: AWM not existing)", papp->StrId());
		assert(awm);
		return APP_WM_NOT_FOUND;
	}

	if (_Disabled()) {
		logger->Debug("Schedule request for [%s] FAILED "
				"(Error: EXC being disabled)", papp->StrId());
		return APP_DISABLED;
	}

	// Checking for resources availability
	booking = ra.BookResources(papp, awm->GetSchedResourceBinding(), vtok);

	// If resources are not available, unschedule
	if (booking != br::ResourceAccounter::RA_SUCCESS) {
		logger->Debug("Unscheduling [%s]...", papp->StrId());
		Unschedule();
		return APP_WM_REJECTED;
	}

	// Bind the resource set to the working mode
	awm->SetSchedResourceBinding();

	// Reschedule accordingly to "awm"
	logger->Debug("Rescheduling [%s] into AWM [%d:%s]...",
			papp->StrId(), awm->Id(), awm->Name().c_str());
	result = Reschedule(awm);

	// Reschedule failed. Release resources and clear resource binding
	if (result != APP_SUCCESS) {
		ra.ReleaseResources(papp, vtok);
		awm->ClearResourceBinding();
		return APP_WM_REJECTED;
	}

	// Set next awm
	schedule.next_awm = awm;

	return APP_WM_ACCEPTED;
}

/*******************************************************************************
 *  EXC Synchronization
 ******************************************************************************/

Application::ExitCode_t Application::SetRunning() {
	SetState(RUNNING);
	return APP_SUCCESS;
}

Application::ExitCode_t Application::SetBlocked() {
	// If the application as been marked FINISHED, than it is released
	if (_State() == FINISHED)
		return APP_SUCCESS;
	// Otherwise mark it as READY to be re-scheduled when possible
	SetState(READY);
	return APP_SUCCESS;
}

Application::ExitCode_t Application::ScheduleCommit() {
	std::unique_lock<std::recursive_mutex> state_ul(schedule.mtx);

	// Ignoring applications disabled during a SYNC
	if (_Disabled()) {
		logger->Info("Sync completed (on disabled EXC) [%s, %d:%s]",
				StrId(), _State(), StateStr(_State()));
		return APP_SUCCESS;
	}

	assert(_State() == SYNC);

	switch(_SyncState()) {
	case STARTING:
	case RECONF:
	case MIGREC:
	case MIGRATE:
		schedule.awm = schedule.next_awm;
		schedule.next_awm.reset();
		SetRunning();
		break;

	case BLOCKED:
		schedule.awm.reset();
		schedule.next_awm.reset();
		SetBlocked();
		break;

	default:
		logger->Crit("Sync for EXC [%s] FAILED"
				"(Error: invalid synchronization state)");
		assert(_SyncState() < Application::SYNC_NONE);
		return APP_ABORT;
	}

	logger->Info("Sync completed [%s, %d:%s]",
			StrId(), _State(), StateStr(_State()));

	return APP_SUCCESS;
}

void Application::ScheduleAbort() {
	std::unique_lock<std::recursive_mutex> state_ul(schedule.mtx);

	// The abort must be performed only for SYNC App/ExC
	if (!Synching()) {
		logger->Fatal("ScheduleAbort: [%s] in state [%s] (expected SYNC)",
				StrId(), StateStr(State()));
		assert(Synching());
	}

	// Set as READY;
	SetState(READY);

	// Reset working modes settings
	schedule.awm.reset();
	schedule.next_awm.reset();

	logger->Info("ScheduleAbort completed ");
}

Application::ExitCode_t Application::ScheduleContinue() {
	std::unique_lock<std::recursive_mutex> state_ul(schedule.mtx);

	// Current AWM must be set
	assert(schedule.awm);

	// This must be called only for RUNNING App/ExC
	if (_State() != RUNNING) {
		logger->Error("ScheduleRunning: [%s] is not running. State {%s/%s}",
				StrId(), StateStr(_State()), SyncStateStr(_SyncState()));
		assert(_State() == RUNNING);
		assert(_SyncState() == SYNC_NONE);
		return APP_ABORT;
	}

	// Return if Next AWN is already blank
	if (!schedule.next_awm)
		return APP_SUCCESS;

	// AWM current and next must match
	if (schedule.awm->Id() != schedule.next_awm->Id()) {
		logger->Error("ScheduleRunning: [%s] AWMs differs. "
				"{curr=%d / next=%d}", StrId(),
				schedule.awm->Id(), schedule.next_awm->Id());
		assert(schedule.awm->Id() != schedule.next_awm->Id());
		return APP_ABORT;
	}

	// Reset next AWM (only current must be set)
	schedule.next_awm.reset();

	return APP_SUCCESS;
}

/*******************************************************************************
 *  EXC Constraints Management
 ******************************************************************************/

Application::ExitCode_t Application::SetWorkingModeConstraint(
		RTLIB_Constraint & constraint) {

	// 'add' field must be true
	if (!constraint.add) {
		logger->Error("SetConstraint (AWMs): Expected 'add' == true");
		return APP_ABORT;
	}

	// Check the working mode ID validity
	if (constraint.awm > awms.max_id)
		return APP_WM_NOT_FOUND;

	logger->Debug("SetConstraint (AWMs): %d total working modes",
			awms.recipe_vect.size());
	logger->Debug("SetConstraint (AWMs): %d enabled working modes",
			awms.enabled_list.size());
	switch (constraint.type) {
	case RTLIB_ConstraintType::LOW_BOUND:

	case RTLIB_ConstraintType::UPPER_BOUND:
		// If the upper < lower: lower = begin
		if (constraint.awm < awms.low_id)
			awms.low_id = 0;


	case RTLIB_ConstraintType::EXACT_VALUE:

	}


	// Check if there are resource constraints
	UpdateEnabledWorkingModes();


	return APP_SUCCESS;
}

void Application::ClearWorkingModeConstraints() {
	// Reset range bounds
	awms.low_id = 0;
	awms.upp_id = awms.max_id;

	// Rebuild the list of enabled working modes

	// Check if there are resource constraints
	UpdateEnabledWorkingModes();

	logger->Debug("ClearConstraint (AWMs): %d total working modes",
			awms.recipe_vect.size());
	logger->Debug("ClearConstraint (AWMs): %d enabled working modes",
			awms.enabled_list.size());
}


/************************** Resource Constraints ****************************/

bool UsageOutOfBounds(const AwmPtr_t & awm) {
	ConstrMap_t::iterator rsrc_constr_it;
	ConstrMap_t::iterator end_rsrc_constr(rsrc_constraints.end());
	UsagesMap_t::const_iterator usage_it = awm->ResourceUsages().begin();
	UsagesMap_t::const_iterator end_usage = awm->ResourceUsages().end();

	// Check if there are constraints on the resource usages
	for (; usage_it != end_usage; ++usage_it) {
		rsrc_constr_it = rsrc_constraints.find(usage_it->first);
		if (rsrc_constr_it == end_rsrc_constr)
			continue;

		// Check if the usage value is out of the constraint bounds
		UsagePtr_t const & pusage(usage_it->second);
		if ((pusage->GetAmount() < rsrc_constr_it->second->lower) ||
				(pusage->GetAmount() > rsrc_constr_it->second->upper))
			return true;
	}

	return false;
}

void Application::UpdateEnabledWorkingModes() {
	// Remove working modes that violate resources constraints
	awms.enabled_list.remove_if(UsageOutOfBounds);

	// Sort by working mode "value
	awms.enabled_list.sort(AwmValueLesser);
}

Application::ExitCode_t Application::SetResourceConstraint(
				std::string const & _rsrc_path,
				ResourceConstraint::BoundType_t _type,
				uint64_t _value) {
	br::ResourceAccounter &ra(br::ResourceAccounter::GetInstance());

	// Check the existance of the resource
	if (!ra.ExistResource(_rsrc_path)) {
		logger->Warn("SetResourceConstraint: %s not found",
				_rsrc_path.c_str());
		return APP_RSRC_NOT_FOUND;
	}

	// Init a new constraint (if do not exist yet)
	ConstrMap_t::iterator it_con(rsrc_constraints.find(_rsrc_path));
	if (it_con == rsrc_constraints.end()) {
		rsrc_constraints.insert(ConstrPair_t(_rsrc_path,
					ConstrPtr_t(new ResourceConstraint)));
	}

	// Set the constraint bound value (if value exists overwrite it)
	switch(_type) {
	case ResourceConstraint::LOWER_BOUND:
		rsrc_constraints[_rsrc_path]->lower = _value;
		if (rsrc_constraints[_rsrc_path]->upper < _value)
			rsrc_constraints[_rsrc_path]->upper =
				std::numeric_limits<uint64_t>::max();

		logger->Debug("SetConstraint (Resources): Set on {%s} LB = %llu",
				_rsrc_path.c_str(), _value);
		break;

	case ResourceConstraint::UPPER_BOUND:
		rsrc_constraints[_rsrc_path]->upper = _value;
		if (rsrc_constraints[_rsrc_path]->lower > _value)
			rsrc_constraints[_rsrc_path]->lower = 0;

		logger->Debug("SetConstraint (Resources): Set on {%s} UB = %llu",
				_rsrc_path.c_str(), _value);
		break;
	}

	// Check if there are some AWMs to disable
	UpdateEnabledWorkingModes();

	return APP_SUCCESS;
}

Application::ExitCode_t Application::ClearResourceConstraint(
				std::string const & _rsrc_path,
				ResourceConstraint::BoundType_t _type) {
	// Lookup the constraint by resource pathname
	ConstrMap_t::iterator it_con(rsrc_constraints.find(_rsrc_path));
	if (it_con == rsrc_constraints.end()) {
		logger->Warn("ClearConstraint (Resources): failed due to unknown "
				"resource path");
		return APP_CONS_NOT_FOUND;
	}

	// Reset the constraint
	switch (_type) {
	case ResourceConstraint::LOWER_BOUND :
		it_con->second->lower = 0;
		if (it_con->second->upper == std::numeric_limits<uint64_t>::max())
			rsrc_constraints.erase(it_con);
		break;

	case ResourceConstraint::UPPER_BOUND :
		it_con->second->upper = std::numeric_limits<uint64_t>::max();
		if (it_con->second->lower == 0)
			rsrc_constraints.erase(it_con);
		break;
	}

	// Check if there are some awms to enable
	UpdateEnabledWorkingModes();

	return APP_SUCCESS;
}

} // namespace app

} // namespace bbque

