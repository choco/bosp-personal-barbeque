/*
 * Copyright (C) 2014  Politecnico di Milano
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

#include <dlfcn.h>

#include "bbque/pm/power_manager_amd.h"


#define  GET_PLATFORM_ADAPTER_ID(rp, adapter_id) \
	int adapter_id = GetAdapterId(rp); \
	if (adapter_id < 0) { \
		logger->Error("ADL: Path '%s' does not resolve a resource"); \
		return PMResult::ERR_RSRC_INVALID_PATH; \
	}

#define CHECK_OD_VERSION(adapter_id)\
	if (od_status_map[adapter_id].version != ADL_OD_VERSION) {\
		logger->Warn("ADL: Overdrive version %d not supported."\
			"Version %d expected", ADL_OD_VERSION); \
		return PMResult::ERR_API_VERSION;\
	}


namespace bbque {


/**   AMD Display Library */

void* __stdcall ADL_Main_Memory_Alloc ( int iSize )
{
    void* lpBuffer = malloc ( iSize );
    return lpBuffer;
}

void __stdcall ADL_Main_Memory_Free ( void** lpBuffer )
{
    if ( NULL != *lpBuffer )
    {
        free ( *lpBuffer );
        *lpBuffer = NULL;
    }
}

AMDPowerManager::AMDPowerManager(
		ResourcePathPtr_t rp,
		std::string const & vendor = ""):
	PowerManager(rp, vendor) {

	// Init references to library functions
	if (InitLibrary() != 0)
		return;
	// Retrieve information about the GPU(s) of the system
	LoadAdaptersInfo();
}


int AMDPowerManager::InitLibrary() {
	// Load AMD Display Library
	adlib = dlopen(ADL_NAME, RTLD_LAZY|RTLD_GLOBAL);
	if (adlib == NULL) {
		logger->Fatal("ADL: Unable to load '%s'", ADL_NAME);
		return 1;
	}

	// Initialization functions
	*(void **) (&ODCaps          ) = dlsym(adlib, SYM_ADL_OD_CAPS);
	*(void **) (&ODParamsGet     ) = dlsym(adlib, SYM_ADL_OD_PARAMS_GET);
	*(void **) (&MainCtrlCreate  ) = dlsym(adlib, SYM_ADL_MAIN_CTRL_CREATE);
	*(void **) (&MainCtrlDestroy ) = dlsym(adlib, SYM_ADL_MAIN_CTRL_DESTROY);
	// General adapters API
	*(void **) (&AdapterNumberGet ) = dlsym(adlib, SYM_ADL_ADAPTER_NUMBER_GET);
	*(void **) (&AdapterActiveGet ) = dlsym(adlib, SYM_ADL_ADAPTER_ACTIVE_GET);
	*(void **) (&AdapterInfoGet   ) = dlsym(adlib, SYM_ADL_ADAPTER_INFO_GET);
	*(void **) (&AdapterAccessGet ) = dlsym(adlib, SYM_ADL_ADAPTER_ACCESS_GET);
	if (ODCaps == NULL
		|| ODParamsGet      == NULL
		|| MainCtrlCreate   == NULL
		|| MainCtrlDestroy  == NULL
		|| AdapterNumberGet == NULL
		|| AdapterActiveGet == NULL
		|| AdapterInfoGet   == NULL) {
		logger->Fatal("ADL: Cannot load library functions in '%s'", ADL_NAME);
		return 2;
	}
	// OverDrive related API
	*(void **) (&ODCurrentActivityGet ) = dlsym(adlib, SYM_ADL_OD_CURRACTIVITY_GET);
	*(void **) (&ODTemperatureGet     ) = dlsym(adlib, SYM_ADL_OD_TEMPERATURE_GET);
	*(void **) (&ODFanSpeedGet        ) = dlsym(adlib, SYM_ADL_OD_FANSPEED_GET);
	*(void **) (&ODFanSpeedSet        ) = dlsym(adlib, SYM_ADL_OD_FANSPEED_SET);
	*(void **) (&ODFanSpeed2DfSet     ) = dlsym(adlib, SYM_ADL_OD_FANSPEED2DF_SET);
	*(void **) (&ODFanSpeedInfoGet    ) = dlsym(adlib, SYM_ADL_OD_FANSPEEDINFO_GET);
	*(void **) (&ODPowerCtrlCaps      ) = dlsym(adlib, SYM_ADL_OD_POWERCTRL_CAPS);
	*(void **) (&ODPowerCtrlInfoGet   ) = dlsym(adlib, SYM_ADL_OD_POWERCTRLINFO_GET);
	*(void **) (&ODPowerCtrlGet       ) = dlsym(adlib, SYM_ADL_OD_POWERCTRL_GET);
	*(void **) (&ODPowerCtrlSet       ) = dlsym(adlib, SYM_ADL_OD_POWERCTRL_SET);

	return 0;
}

void AMDPowerManager::LoadAdaptersInfo() {
	int ADL_Err = ADL_ERR;
	int status;
	int power_caps = 0;
	ODStatus_t od_status;
	ADLODParameters od_params;

	ADL_Err = MainCtrlCreate(
		ADL_Main_Memory_Alloc, 1, &context, ADL_THREADING_LOCKED);
	if (ADL_Err != ADL_OK) {
		logger->Error("ADL: Control initialization failed");
		return;
	}

	// Adapters enumeration
	ADL_Err = AdapterNumberGet(context, &adapters_count);
	if (adapters_count < 1) {
		logger->Error("ADL: No adapters available on the system");
		return;
	}
	logger->Debug("ADL: Adapters (GPUs) count = %d", adapters_count);

	for (int i = 0; i < adapters_count; ++i) {
		ADL_Err = AdapterActiveGet(context, i, &status);
		if (ADL_Err != ADL_OK || status == ADL_FALSE) {
			logger->Debug("Skipping '%d' [Err:%d] ", i, ADL_Err);
			continue;
		}
		// Adapters ID mapping and resouce path
		adapters_map.insert(
			std::pair<ResID_t, int>(adapters_map.size(), i));
		activity_map.insert(
			std::pair<int, ActivityPtr_t>(
				i, ActivityPtr_t(new ADLPMActivity)));

		// Power control capabilities
		if (ODPowerCtrlCaps != NULL) {
			ODPowerCtrlCaps(context, i, &power_caps);
			power_caps_map.insert(
				std::pair<int, int>(i, power_caps));
		}
		logger->Info("ADL: [A%d] Power control capabilities: %d",
			i, power_caps_map[i]);

		// Overdrive information
		ODCaps(context, i,
			&od_status.supported, &od_status.enabled,
			&od_status.version);
		if (ADL_Err != ADL_OK) {
			logger->Error("ADL: Overdrive information read failed [%d]",
				ADL_Err);
			return;
		}
		od_status_map.insert(
			std::pair<int, ODStatus_t>(i, od_status));
		logger->Info("ADL: [A%d] Overdrive: "
			"[supported:%d, enabled:%d, version:%d]", i,
			od_status_map[i].supported,
			od_status_map[i].enabled,
			od_status_map[i].version);

		// Overdrive parameters
		ADL_Err = ODParamsGet(context, i, &od_params);
		if (ADL_Err != ADL_OK) {
			logger->Error("ADL: Overdrive parameters read failed [%d]",
				ADL_Err);
			return;
		}
		od_params_map.insert(
			std::pair<int, ADLODParameters>(i, od_params));
		od_ready = true;
	}

	initialized = true;
	logger->Info("ADL: Adapters information initialized");
	MainCtrlDestroy(context);
}

AMDPowerManager::~AMDPowerManager() {
	for (auto adapter: adapters_map) {
		_ResetFanSpeed(adapter.second);
	}

	if (MainCtrlDestroy == NULL)
		return;
	MainCtrlDestroy(context);
	logger->Info("ADL: Control destroyed");

	adapters_map.clear();
	activity_map.clear();
	power_caps_map.clear();
	od_status_map.clear();
}

int AMDPowerManager::GetAdapterId(ResourcePathPtr_t const & rp) const {
	std::map<ResID_t, int>::const_iterator it;
	it = adapters_map.find(rp->GetID(ResourceIdentifier::GPU));
	if (it == adapters_map.end())
		return -1;
	return it->second;
}

AMDPowerManager::ActivityPtr_t
AMDPowerManager::GetActivityInfo(int adapter_id) {
	std::map<int, ActivityPtr_t>::iterator a_it;
	a_it = activity_map.find(adapter_id);
	if (a_it == activity_map.end()) {
		logger->Error("ADL: GetActivity invalid adapter id");
		return nullptr;
	}
	return a_it->second;
}

PowerManager::PMResult
AMDPowerManager::GetActivity(int adapter_id) {
	int ADL_Err = ADL_ERR;

	CHECK_OD_VERSION(adapter_id);

	ActivityPtr_t activity(GetActivityInfo(adapter_id));
	if (activity == nullptr)
		return PMResult::ERR_RSRC_INVALID_PATH;

	ADL_Err = MainCtrlCreate(
		ADL_Main_Memory_Alloc, 1, &context, ADL_THREADING_LOCKED);
	if (ADL_Err != ADL_OK) {
		logger->Error("ADL: Control initialization failed");
		return PMResult::ERR_API_INVALID_VALUE;
	}

	if (ODCurrentActivityGet == NULL || !initialized) {
		logger->Warn("ADL: Cannot get GPU(s) activity");
		return PMResult::ERR_API_NOT_SUPPORTED;
	}

	ADL_Err = ODCurrentActivityGet(context, adapter_id, activity.get());
	MainCtrlDestroy(context);
	return PMResult::OK;
}

PowerManager::PMResult
AMDPowerManager::GetLoad(ResourcePathPtr_t const & rp, uint32_t & perc) {
	PMResult pm_result;
	perc = 0;

	GET_PLATFORM_ADAPTER_ID(rp, adapter_id);
	pm_result = GetActivity(adapter_id);
	if (pm_result != PMResult::OK) {
		return pm_result;
	}

	perc = activity_map[adapter_id]->iActivityPercent;
	return PMResult::OK;
}

PowerManager::PMResult
AMDPowerManager::GetTemperature(ResourcePathPtr_t const & rp, uint32_t &celsius) {
	int ADL_Err = ADL_ERR;
	celsius = 0;
	ADLTemperature temp;

	GET_PLATFORM_ADAPTER_ID(rp, adapter_id);
	CHECK_OD_VERSION(adapter_id);

	ADL_Err = MainCtrlCreate(
		ADL_Main_Memory_Alloc, 1, &context, ADL_THREADING_LOCKED);
	if (ADL_Err != ADL_OK) {
		logger->Error("ADL: Control initialization failed");
		return PMResult::ERR_API_INVALID_VALUE;
	}

	if (ODTemperatureGet == NULL || !initialized) {
		logger->Warn("ADL: Cannot get GPU(s) temperature");
		return PMResult::ERR_API_NOT_SUPPORTED;
	}

	ADL_Err = ODTemperatureGet(context, adapter_id, 0, &temp);
	if (ADL_Err != ADL_OK) {
		logger->Error(
			"ADL: [A%d] Temperature not available [%d]",
			adapter_id, ADL_Err);
		return PMResult::ERR_API_INVALID_VALUE;
	}
	celsius = temp.iTemperature / 1000;
	logger->Debug("ADL: [A%d] Temperature : %d °C",
		adapter_id, celsius);

	MainCtrlDestroy(context);
	return PMResult::OK;
}


/* Clock frequency */

PowerManager::PMResult
AMDPowerManager::GetClockFrequency(ResourcePathPtr_t const & rp, uint32_t &khz) {
	PMResult pm_result;
	ResourceIdentifier::Type_t r_type = rp->Type();
	khz = 0;

	GET_PLATFORM_ADAPTER_ID(rp, adapter_id);
	CHECK_OD_VERSION(adapter_id);

	pm_result = GetActivity(adapter_id);
	if (pm_result != PMResult::OK) {
		return pm_result;
	}

	if (r_type == ResourceIdentifier::PROC_ELEMENT)
		khz = activity_map[adapter_id]->iEngineClock * 10;
	else if (r_type == ResourceIdentifier::MEMORY)
		khz = activity_map[adapter_id]->iMemoryClock * 10;
	else {
		logger->Warn("ADL: Invalid resource path [%s]", rp->ToString().c_str());
		return PMResult::ERR_RSRC_INVALID_PATH;
	}

	return PMResult::OK;
}


PowerManager::PMResult
AMDPowerManager::GetClockFrequencyInfo(
		ResourcePathPtr_t const & rp,
		uint32_t &khz_min,
		uint32_t &khz_max,
		uint32_t &khz_step) {
	khz_min = khz_max = khz_step = 0;
	ResourceIdentifier::Type_t r_type = rp->Type();
	GET_PLATFORM_ADAPTER_ID(rp, adapter_id);

	if (!od_ready) {
		logger->Warn("ADL: Overdrive parameters missing");
		return PMResult::ERR_NOT_INITIALIZED;
	}

	if (r_type == ResourceIdentifier::PROC_ELEMENT) {
		khz_min  = od_params_map[adapter_id].sEngineClock.iMin * 10;
		khz_max  = od_params_map[adapter_id].sEngineClock.iMax * 10;
		khz_step = od_params_map[adapter_id].sEngineClock.iStep * 10;
	}
	else if (r_type == ResourceIdentifier::MEMORY) {
		khz_min  = od_params_map[adapter_id].sMemoryClock.iMin * 10;
		khz_max  = od_params_map[adapter_id].sMemoryClock.iMax * 10;
		khz_step = od_params_map[adapter_id].sMemoryClock.iStep * 10;
	}
	else
		return PMResult::ERR_RSRC_INVALID_PATH;

	return PMResult::OK;
}


/* Voltage */

PowerManager::PMResult
AMDPowerManager::GetVoltage(ResourcePathPtr_t const & rp, uint32_t &mvolt) {
	mvolt = 0;
	GET_PLATFORM_ADAPTER_ID(rp, adapter_id);

	ActivityPtr_t activity(GetActivityInfo(adapter_id));
	if (activity == nullptr) {
		return PMResult::ERR_RSRC_INVALID_PATH;
	}

	mvolt = activity_map[adapter_id]->iVddc;
	logger->Debug("ADL: [A%d] Voltage: %d mV", adapter_id, mvolt);

	return PMResult::OK;
}

PowerManager::PMResult
AMDPowerManager::GetVoltageInfo(
		ResourcePathPtr_t const & rp,
		uint32_t &mvolt_min,
		uint32_t &mvolt_max,
		uint32_t &mvolt_step) {
	ResourceIdentifier::Type_t r_type = rp->Type();
	GET_PLATFORM_ADAPTER_ID(rp, adapter_id);

	if (!od_ready) {
		logger->Warn("ADL: Overdrive parameters missing");
		return PMResult::ERR_NOT_INITIALIZED;
	}

	if (r_type != ResourceIdentifier::PROC_ELEMENT) {
		logger->Error("ADL: Not a processing resource!");
		return PMResult::ERR_RSRC_INVALID_PATH;
	}

	mvolt_min  = od_params_map[adapter_id].sVddc.iMin;
	mvolt_max  = od_params_map[adapter_id].sVddc.iMax;
	mvolt_step = od_params_map[adapter_id].sVddc.iStep;

	return PMResult::OK;
}

/* Fan */

PowerManager::PMResult
AMDPowerManager::GetFanSpeed(
		ResourcePathPtr_t const & rp,
		FanSpeedType fs_type,
		uint32_t &value) {
	int ADL_Err = ADL_ERR;
	ADLFanSpeedValue fan;
	value = 0;
	GET_PLATFORM_ADAPTER_ID(rp, adapter_id);
	CHECK_OD_VERSION(adapter_id);

	ADL_Err = MainCtrlCreate(
		ADL_Main_Memory_Alloc, 1, &context, ADL_THREADING_LOCKED);
	if (ADL_Err != ADL_OK) {
		logger->Error("ADL: Control initialization failed");
		return PMResult::ERR_API_INVALID_VALUE;
	}

	if (ODFanSpeedGet == NULL || !initialized) {
		logger->Warn("ADL: Cannot get GPU(s) fan speed");
		return PMResult::ERR_API_NOT_SUPPORTED;
	}

	// Fan speed type
	if (fs_type == FanSpeedType::PERCENT)
		fan.iSpeedType = ADL_DL_FANCTRL_SPEED_TYPE_PERCENT;
	else if (fs_type == FanSpeedType::RPM)
		fan.iSpeedType = ADL_DL_FANCTRL_SPEED_TYPE_RPM;

	ADL_Err = ODFanSpeedGet(context, adapter_id, 0, &fan);
	if (ADL_Err != ADL_OK) {
		logger->Error(
			"ADL: [A%d] Fan speed adapter not available [%d]",
			adapter_id, ADL_Err);
		return PMResult::ERR_API_INVALID_VALUE;
	}
	value = fan.iFanSpeed;
	logger->Debug("ADL: [A%d] Fan speed: %d % ", adapter_id, value);

	MainCtrlDestroy(context);
	return PMResult::OK;
}

PowerManager::PMResult
AMDPowerManager::GetFanSpeedInfo(
		ResourcePathPtr_t const & rp,
		uint32_t &rpm_min,
		uint32_t &rpm_max,
		uint32_t &rpm_step) {
	int ADL_Err = ADL_ERR;
	GET_PLATFORM_ADAPTER_ID(rp, adapter_id);

	if (!od_ready) {
		logger->Warn("ADL: Overdrive parameters missing");
		return PMResult::ERR_NOT_INITIALIZED;
	}

	ADL_Err = MainCtrlCreate(
		ADL_Main_Memory_Alloc, 1, &context, ADL_THREADING_LOCKED);
	if (ADL_Err != ADL_OK) {
		logger->Error("ADL: Control initialization failed");
		return PMResult::ERR_API_INVALID_VALUE;
	}

	if (ODFanSpeedInfoGet == NULL || !initialized) {
		logger->Warn("ADL: Cannot get GPU(s) fan speed information");
		return PMResult::ERR_API_NOT_SUPPORTED;
	}

	ADLFanSpeedInfo fan;
	ADL_Err  = ODFanSpeedInfoGet(context, adapter_id, 0, &fan);
	rpm_min  = fan.iMinRPM;
	rpm_max  = fan.iMaxRPM;
	rpm_step = 0;

	MainCtrlDestroy(context);
	return PMResult::OK;
}

PowerManager::PMResult
AMDPowerManager::SetFanSpeed(
		ResourcePathPtr_t const & rp,
		FanSpeedType fs_type,
		uint32_t value)  {
	int ADL_Err = ADL_ERR;
	ADLFanSpeedValue fan;

	GET_PLATFORM_ADAPTER_ID(rp, adapter_id);
	CHECK_OD_VERSION(adapter_id);

	ADL_Err = MainCtrlCreate(
		ADL_Main_Memory_Alloc, 1, &context, ADL_THREADING_LOCKED);
	if (ADL_Err != ADL_OK) {
		logger->Error("ADL: Control initialization failed");
		return PMResult::ERR_API_INVALID_VALUE;
	}

	if (ODFanSpeedSet == NULL || !initialized) {
		logger->Warn("ADL: Cannot get GPU(s) fan speed");
		return PMResult::ERR_API_NOT_SUPPORTED;
	}

	if (fs_type == FanSpeedType::PERCENT)
		fan.iSpeedType = ADL_DL_FANCTRL_SPEED_TYPE_PERCENT;
	else if (fs_type == FanSpeedType::RPM)
		fan.iSpeedType = ADL_DL_FANCTRL_SPEED_TYPE_RPM;

	fan.iFanSpeed = value;
	ADL_Err = ODFanSpeedSet(context, adapter_id, 0, &fan);
	if (ADL_Err != ADL_OK) {
		logger->Error(
			"ADL: [A%d] Fan speed set failed [%d]",
			adapter_id, ADL_Err);
		return PMResult::ERR_API_INVALID_VALUE;
	}

	MainCtrlDestroy(context);
	return PMResult::OK;
}

PowerManager::PMResult
AMDPowerManager::ResetFanSpeed(ResourcePathPtr_t const & rp) {
	GET_PLATFORM_ADAPTER_ID(rp, adapter_id);

	return _ResetFanSpeed(adapter_id);
}

PowerManager::PMResult
AMDPowerManager::_ResetFanSpeed(int adapter_id) {
	int ADL_Err = ADL_ERR;

	CHECK_OD_VERSION(adapter_id);

	ADL_Err = MainCtrlCreate(
		ADL_Main_Memory_Alloc, 1, &context, ADL_THREADING_LOCKED);
	if (ADL_Err != ADL_OK) {
		logger->Error("ADL: Control initialization failed");
		return PMResult::ERR_API_INVALID_VALUE;
	}

	if (ODFanSpeed2DfSet == NULL || !initialized) {
		logger->Warn("ADL: Cannot get GPU(s) fan speed");
		return PMResult::ERR_API_NOT_SUPPORTED;
	}

	ADL_Err = ODFanSpeed2DfSet(context, adapter_id, 0);
	if (ADL_Err != ADL_OK) {
		logger->Error(
			"ADL: [A%d] Fan speed reset failed [%d]",
			adapter_id, ADL_Err);
		return PMResult::ERR_API_INVALID_VALUE;
	}

	MainCtrlDestroy(context);
	return PMResult::OK;
}

/* Power */

PowerManager::PMResult
AMDPowerManager::GetPowerStatesInfo(
		ResourcePathPtr_t const & rp,
		int &min,
		int &max,
		int &step) {
	int ADL_Err = ADL_ERR;
	GET_PLATFORM_ADAPTER_ID(rp, adapter_id);

	if (!od_ready) {
		logger->Warn("ADL: Overdrive parameters missing");
		return PMResult::ERR_NOT_INITIALIZED;
	}

	ADL_Err = MainCtrlCreate(
		ADL_Main_Memory_Alloc, 1, &context, ADL_THREADING_LOCKED);
	if (ADL_Err != ADL_OK) {
		logger->Error("ADL: Control initialization failed");
		return PMResult::ERR_API_INVALID_VALUE;
	}

	if (ODPowerCtrlInfoGet == NULL || !initialized) {
		logger->Warn("ADL: Cannot get GPU(s) fan speed information");
		return PMResult::ERR_API_NOT_SUPPORTED;
	}

	ADLPowerControlInfo pwr_info;
	ADL_Err = ODPowerCtrlInfoGet(context, adapter_id, &pwr_info);
	min  = pwr_info.iMinValue;
	max  = pwr_info.iMaxValue;
	step = pwr_info.iStepValue;

	MainCtrlDestroy(context);
	return PMResult::OK;
}

/* States */

PowerManager::PMResult
AMDPowerManager::GetPowerState(ResourcePathPtr_t const & rp, int & state) {
	int ADL_Err = ADL_ERR;
	int dflt;

	GET_PLATFORM_ADAPTER_ID(rp, adapter_id);
	CHECK_OD_VERSION(adapter_id);

	ADL_Err = MainCtrlCreate(
		ADL_Main_Memory_Alloc, 1, &context, ADL_THREADING_LOCKED);
	if (ADL_Err != ADL_OK) {
		logger->Error("ADL: Control initialization failed");
		return PMResult::ERR_API_INVALID_VALUE;
	}

	if (ODPowerCtrlGet == NULL || !initialized) {
		logger->Warn("ADL: Cannot get GPU(s) power state");
		return PMResult::ERR_API_NOT_SUPPORTED;
	}

	ADL_Err = ODPowerCtrlGet(context, adapter_id, &state, &dflt);
	if (ADL_Err != ADL_OK) {
		logger->Error(
			"ADL: [A%d] Power control not available [%d]",
			adapter_id, ADL_Err);
		return PMResult::ERR_API_INVALID_VALUE;
	}

	MainCtrlDestroy(context);
	return PMResult::OK;
}

PowerManager::PMResult
AMDPowerManager::SetPowerState(ResourcePathPtr_t const & rp, int state) {
	int ADL_Err = ADL_ERR;

	GET_PLATFORM_ADAPTER_ID(rp, adapter_id);
	CHECK_OD_VERSION(adapter_id);

	ADL_Err = MainCtrlCreate(
		ADL_Main_Memory_Alloc, 1, &context, ADL_THREADING_LOCKED);
	if (ADL_Err != ADL_OK) {
		logger->Error("ADL: Control initialization failed");
		return PMResult::ERR_API_INVALID_VALUE;
	}

	if (ODPowerCtrlSet == NULL || !initialized) {
		logger->Warn("ADL: Cannot set GPU(s) power state");
		return PMResult::ERR_API_NOT_SUPPORTED;
	}

	ADL_Err = ODPowerCtrlSet(context, adapter_id, state);
	if (ADL_Err != ADL_OK) {
		logger->Error(
			"ADL: [A%d] Power state set failed [%d]",
			adapter_id, ADL_Err);
		return PMResult::ERR_API_INVALID_VALUE;
	}

	MainCtrlDestroy(context);
	return PMResult::OK;
}

PowerManager::PMResult
AMDPowerManager::GetPerformanceState(ResourcePathPtr_t const & rp, int &state) {
	state = 0;
	GET_PLATFORM_ADAPTER_ID(rp, adapter_id);

	ActivityPtr_t activity(GetActivityInfo(adapter_id));
	if (activity == nullptr)
		return PMResult::ERR_RSRC_INVALID_PATH;

	state = activity_map[adapter_id]->iCurrentPerformanceLevel,
	logger->Debug("ADL: [A%d] PerformanceState: %d ", adapter_id, state);

	return PMResult::OK;
}

PowerManager::PMResult
AMDPowerManager::GetPerformanceStatesCount(ResourcePathPtr_t const & rp, int &count) {
	count = 0;
	GET_PLATFORM_ADAPTER_ID(rp, adapter_id);

	if (!od_ready) {
		logger->Warn("ADL: Overdrive parameters missing");
		return PMResult::ERR_NOT_INITIALIZED;
	}

	CHECK_OD_VERSION(adapter_id);
	count = od_params_map[adapter_id].iNumberOfPerformanceLevels;

	return PMResult::OK;
}

} // namespace bbque

