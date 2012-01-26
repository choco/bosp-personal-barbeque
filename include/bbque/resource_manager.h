/*
 * Copyright (C) 2012  Politecnico di Milano
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef BBQUE_RESOURCE_MANAGER_H_
#define BBQUE_RESOURCE_MANAGER_H_

#include "bbque/config.h"
#include "bbque/application_manager.h"
#include "bbque/application_proxy.h"
#include "bbque/platform_proxy.h"
#include "bbque/platform_services.h"
#include "bbque/plugin_manager.h"
#include "bbque/scheduler_manager.h"
#include "bbque/synchronization_manager.h"
#include "bbque/res/resource_accounter.h"

#include "bbque/plugins/logger.h"
#include "bbque/utils/timer.h"
#include "bbque/utils/metrics_collector.h"

#include <bitset>

using bbque::res::ResourceAccounter;
using bbque::plugins::PluginManager;
using bbque::plugins::LoggerIF;
using bbque::utils::MetricsCollector;

namespace bbque {

/**
 * @brief The class implementing the glue logic of Barbeque
 * @ingroup sec04_rm
 *
 * This class provides the glue logic of the Barbeque RTRM. Its 'Go'
 * method represents the entry point of the run-time manager and it is
 * called by main right after some playground preparation activities.
 * This class is in charge to load all needed modules and run the control
 * loop.
 */
class ResourceManager {

public:

	/**
	 * @brief Exit codes returned by methods of this class
	 */
	typedef enum ExitCode {
		OK = 0,
		SETUP_FAILED
	} ExitCode_t;

	typedef enum controlEvent {
		EXC_START = 0,
		EXC_STOP,

		BBQ_OPTS,
		BBQ_USR1,
		BBQ_USR2,

		BBQ_EXIT,
		BBQ_ABORT,

		EVENTS_COUNT
	} controlEvent_t;

	/**
	 * @brief Get a reference to the resource manager
	 * The ResourceManager is a singleton class providing the glue logic for
	 * the Barbeque run-time resource manager. An instance to this class is to
	 * be obtained by main in order to start grilling
	 * @return  a reference to the ResourceManager singleton instance
	 */
	static ResourceManager & GetInstance();

	/**
	 * @brief  Clean-up the grill by releasing current resource manager
	 * resources and modules.
	 */
	~ResourceManager();

	/**
	 * @brief Start managing resources
	 * This is the actual run-time manager entry-point, after the playground
	 * setup the main should call this method to start grilling applications.
	 * This will be in charge to load all the required modules and then start
	 * the control cycle.
	 */
	ExitCode_t Go();

	/**
	 * @brief Notify an event to the resource manager
	 *
	 * This method is used to notify the resource manager about events related
	 * to system activity (e.g. stopping/killing Barbeque), applications (e.g.
	 * starting a new execution context) and resources (e.g. changed resources
	 * availability). Notified events could trigger actions within the control
	 * loop, e.g. running the optimization policy to find a new resources
	 * schedule.
	 */
	void NotifyEvent(controlEvent_t evt);

private:

	/**
	 * Set true when Barbeuque should terminate
	 */
	bool done;

	/**
	 * @brief The logger used by the resource manager.
	 */
	LoggerIF *logger;

	/**
	 * Reference to supported platform services class.
	 * The platform services are a set of services exported by Barbeque to
	 * other modules (and plugins). The resource manager ensure an
	 * initialization of this core module before starting to grill.
	 */
	PlatformServices & ps;

	/**
	 * @brief The Resource Scheduler module
	 */
	SchedulerManager & sm;

	/**
	 * @brief The Synchronization Manager module
	 */
	SynchronizationManager & ym;

	/**
	 * @brief The Application Proxy module
	 */
	ApplicationManager & am;

	/**
	 * @brief The Application Proxy module
	 */
	ApplicationProxy & ap;

	/**
	 * Reference to the plugin manager module.
	 * The plugin manager is the required interface to load other modules. The
	 * resource manager ensure an initialization of this core module before
	 * starting to grill.
	 */
	PluginManager & pm;

	/**
	 * @brief The Resource Accounter module
	 */
	ResourceAccounter & ra;

	/**
	 * @brief The collection of Barbeque metrics
	 */
	MetricsCollector & mc;

	/**
	 * @brief The Platform Proxy module
	 */
	PlatformProxy & pp;

	std::bitset<EVENTS_COUNT> pendingEvts;

	std::mutex pendingEvts_mtx;

	std::condition_variable pendingEvts_cv;

	/**
	 * @brief The collection of metrics generated by this module
	 */
	typedef enum ResMgrMetrics {

		//----- Counting metrics
		RM_EVT_TOTAL = 0,
		RM_EVT_START,
		RM_EVT_STOP,
		RM_EVT_OPTS,
		RM_EVT_USR1,
		RM_EVT_USR2,

		RM_SCHED_TOTAL,
		RM_SCHED_FAILED,
		RM_SCHED_DELAYED,
		RM_SCHED_EMPTY,

		RM_SYNCH_TOTAL,
		RM_SYNCH_FAILED,

		//----- Sample statistics
		RM_EVT_TIME,
		RM_EVT_TIME_START,
		RM_EVT_TIME_STOP,
		RM_EVT_TIME_OPTS,
		RM_EVT_TIME_USR1,
		RM_EVT_TIME_USR2,

		RM_EVT_PERIOD,
		RM_EVT_PERIOD_START,
		RM_EVT_PERIOD_STOP,
		RM_EVT_PERIOD_OPTS,
		RM_EVT_PERIOD_USR1,
		RM_EVT_PERIOD_USR2,

		RM_SCHED_PERIOD,
		RM_SYNCH_PERIOD,

		RM_METRICS_COUNT
	} ResMgrMetrics_t;

	/** The High-Resolution timer used for profiling */
	Timer rm_tmr;

	/** The metrics collected by this module */
	static MetricsCollector::MetricsCollection_t metrics[RM_METRICS_COUNT];

	/**
	 * @brief   Build a new instance of the resource manager
	 */
	ResourceManager();

	/**
	 * @brief   Run on optimization cycle (i.e. Schedule and Synchronization)
	 * Once an event happens which impacts on resources usage or availability
	 * an optimization cycle could be triggered by calling this method.
	 * An optimization cycles involve a run of the resource scheduler policy
	 * (e.g. YaMCA)  and an eventual Synchroniation of the active applications
	 * according to the currently loaded synchronization policy (e.g. SASB).
	 */
	void Optimize();

	/**
	 * @brief   The run-time resource manager setup routine
	 * This provides all the required playground setup to run the Barbeque RTRM.
	 */
	ExitCode_t Setup();

	/**
	 * @brief   The run-time resource manager control loop
	 * This provides the Barbeuqe applications and resources control logic.
	 * This is actually the entry point of the Barbeque state machine.
	 */
	void ControlLoop();

	/**
	 * @brief Process a EXC_START event
	 */
	void EvtExcStart();

	/**
	 * @brief Process a BBQ_OPTS event
	 */
	void EvtBbqOpts();

	/**
	 * @brief Process a BBQ_USR1 event
	 */
	void EvtBbqUsr1();

	/**
	 * @brief Process a BBQ_USR1 event
	 */
	void EvtBbqUsr2();

	/**
	 * @brief Process a EXC_STOP event
	 */
	void EvtExcStop();

	/**
	 * @brief Process a BBQ_EXIT event
	 */
	void EvtBbqExit();

};

} // namespace bbque

/*******************************************************************************
 *    Doxygen Module Documentation
 ******************************************************************************/

/**
 * @defgroup sec04_rm Resource Manager
 *
 * The ResourceManager is a BarbequeRTRM core module which provides the
 * implementation of the Run-Time Resource Manager (RTRM), which is the main
 * barbeque module implementing its glue code.
 *
 *
 * ADD MORE DETAILS HERE
 */

#endif // BBQUE_RESOURCE_MANAGER_H_
