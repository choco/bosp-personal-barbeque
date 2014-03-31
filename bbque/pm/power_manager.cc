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

#include "bbque/pm/power_manager.h"

#define POWER_MANAGER AMDPowerManager
#include "bbque/pm/power_manager_amd.h"

#define MODULE_NAMESPACE POWER_MANAGER_NAMESPACE

namespace bbque {


PowerManager & PowerManager::GetInstance(
		ResourcePathPtr_t rp,
		std::string const & vendor) {
	static POWER_MANAGER instance(rp, vendor);
	return instance;
}

PowerManager::PowerManager(
		ResourcePathPtr_t rp,
		std::string const & vendor = ""):
	rsrc_path_domain(rp),
	vendor(vendor) {
	//---------- Get a logger module
	logger = bu::Logger::GetLogger(POWER_MANAGER_NAMESPACE);
	assert(logger);
}


PowerManager::~PowerManager() {

}


PowerManager::PMResult PowerManager::GetLoad(
		ResourcePathPtr_t const & rp, uint32_t &perc) {
	(void)rp;
	(void)perc;
	return PMResult::ERR_API_NOT_SUPPORTED;
}


PowerManager::PMResult PowerManager::GetTemperature(
		ResourcePathPtr_t const & rp, uint32_t &celsius) {
	(void)rp;
	(void)celsius;
	return PMResult::ERR_API_NOT_SUPPORTED;
}


PowerManager::PMResult
PowerManager::GetClockFrequency(ResourcePathPtr_t const & rp, uint32_t &khz) {
	(void)rp;
	(void)khz;
	return PMResult::ERR_API_NOT_SUPPORTED;
}

PowerManager::PMResult
PowerManager::GetClockFrequencyInfo(
		ResourcePathPtr_t const & rp,
		uint32_t &khz_min,
		uint32_t &khz_max,
		uint32_t &khz_step) {
	(void)rp;
	(void)khz_min;
	(void)khz_max;
	(void)khz_step;
	return PMResult::ERR_API_NOT_SUPPORTED;
}

PowerManager::PMResult
PowerManager::SetClockFrequency(ResourcePathPtr_t const & rp, uint32_t khz) {
	(void)rp;
	(void)khz;
	return PMResult::ERR_API_NOT_SUPPORTED;
}


PowerManager::PMResult
PowerManager::GetVoltage(ResourcePathPtr_t const & rp, uint32_t &volt) {
	(void)rp;
	(void)volt;
	return PMResult::ERR_API_NOT_SUPPORTED;
}

PowerManager::PMResult
PowerManager::GetVoltageInfo(
		ResourcePathPtr_t const & rp,
		uint32_t &volt_min,
		uint32_t &volt_max,
		uint32_t &volt_step) {
	(void)rp;
	(void)volt_min;
	(void)volt_max;
	(void)volt_step;
	return PMResult::ERR_API_NOT_SUPPORTED;
}


PowerManager::PMResult
PowerManager::GetFanSpeed(
		ResourcePathPtr_t const & rp,
		FanSpeedType fs_type,
		uint32_t &value) {
	(void)rp;
	(void)fs_type;
	(void)value;
	return PMResult::ERR_API_NOT_SUPPORTED;
}

PowerManager::PMResult
PowerManager::GetFanSpeedInfo(
		ResourcePathPtr_t const & rp,
		uint32_t &rpm_min,
		uint32_t &rpm_max,
		uint32_t &rpm_step) {
	(void)rp;
	(void)rpm_min;
	(void)rpm_max;
	(void)rpm_step;
	return PMResult::ERR_API_NOT_SUPPORTED;
}

PowerManager::PMResult
PowerManager::SetFanSpeed(
		ResourcePathPtr_t const & rp,
		FanSpeedType fs_type,
		uint32_t value) {
	(void)rp;
	(void)fs_type;
	(void)value;
	return PMResult::ERR_API_NOT_SUPPORTED;
}

PowerManager::PMResult
PowerManager::ResetFanSpeed(ResourcePathPtr_t const & rp) {
	(void)rp;
	return PMResult::ERR_API_NOT_SUPPORTED;
}


PowerManager::PMResult
PowerManager::GetPowerUsage(ResourcePathPtr_t const & rp, uint32_t &mwatt) {
	(void)rp;
	(void)mwatt;
	return PMResult::ERR_API_NOT_SUPPORTED;
}

PowerManager::PMResult
PowerManager::GetPowerInfo(
		ResourcePathPtr_t const & rp,
		uint32_t &mwatt_min,
		uint32_t &mwatt_max) {
	(void)rp;
	(void)mwatt_min;
	(void)mwatt_max;
	return PMResult::ERR_API_NOT_SUPPORTED;
}

PowerManager::PMResult
PowerManager::GetPowerState(ResourcePathPtr_t const & rp, int &state) {
	(void)rp;
	(void)state;
	return PMResult::ERR_API_NOT_SUPPORTED;
}

PowerManager::PMResult
PowerManager::GetPowerStatesInfo(
	ResourcePathPtr_t const & rp,
	int & min, int & max, int & step) {
	(void)rp;
	(void)min;
	(void)max;
	(void)step;
	return PMResult::ERR_API_NOT_SUPPORTED;
}

PowerManager::PMResult
PowerManager::SetPowerState(ResourcePathPtr_t const & rp, int state) {
	(void)rp;
	(void)state;
	return PMResult::ERR_API_NOT_SUPPORTED;
}


PowerManager::PMResult
PowerManager::GetPerformanceState(ResourcePathPtr_t const & rp, int &state) {
	(void)rp;
	(void)state;
	return PMResult::ERR_API_NOT_SUPPORTED;
}

PowerManager::PMResult
PowerManager::GetPerformanceStatesCount(ResourcePathPtr_t const & rp, int &count) {
	(void)rp;
	(void)count;
	return PMResult::ERR_API_NOT_SUPPORTED;
}

PowerManager::PMResult
PowerManager::SetPerformanceState(ResourcePathPtr_t const & rp, int state) {
	(void)rp;
	(void)state;
	return PMResult::ERR_API_NOT_SUPPORTED;
}

} // namespace bbque
