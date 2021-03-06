#include "bbque/pp/local_platform_proxy.h"
#include "bbque/pp/test_platform_proxy.h"
#include "bbque/config.h"
#include "bbque/res/resource_path.h"

#ifdef CONFIG_TARGET_LINUX
#include "bbque/pp/linux_platform_proxy.h"
#elif CONFIG_TARGET_ANDROID
#include "bbque/pp/android_platform_proxy.h"
#else
#error LocalPlatformProxy does not know which target to compile.
#endif

#ifdef CONFIG_BBQUE_OPENCL
#include "bbque/pp/opencl_platform_proxy.h"
#endif

namespace bbque {
namespace pp {

LocalPlatformProxy::LocalPlatformProxy() {

#ifdef CONFIG_BBQUE_TEST_PLATFORM_DATA
	this->host = std::unique_ptr<TestPlatformProxy>(
						 TestPlatformProxy::GetInstance());
#elif defined CONFIG_TARGET_LINUX
	this->host = std::unique_ptr<LinuxPlatformProxy>(
	                     LinuxPlatformProxy::GetInstance());
#elif defined CONFIG_TARGET_ANDROID
	this->host = std::unique_ptr<AndroidPlatformProxy>(
	                     AndroidPlatformProxy()::GetInstance());
#else
#error "No suitable PlatformProxy for host found."
#endif

#ifdef CONFIG_BBQUE_OPENCL
	this->aux.push_back(
	        std::unique_ptr<OpenCLPlatformProxy>(OpenCLPlatformProxy::GetInstance()));
#endif

	assert(this->host);
}

const char* LocalPlatformProxy::GetPlatformID(int16_t system_id) const {
	return this->host->GetPlatformID(system_id);
}

const char* LocalPlatformProxy::GetHardwareID(int16_t system_id) const {
	return this->host->GetHardwareID(system_id);
}

LocalPlatformProxy::ExitCode_t LocalPlatformProxy::Setup(AppPtr_t papp) {
	ExitCode_t ec;

	// Obviously, at least a PE in the cpu must be provided to application
	// so we have to call the host PP (Linux, Android, etc.)
	ec = this->host->Setup(papp);
	if (ec != PLATFORM_OK) {
		return ec;
	}

	for (auto it=this->aux.begin() ; it < this->aux.end(); it++) {
		ec = (*it)->Setup(papp);
		if (ec != PLATFORM_OK) {
			return ec;
		}
	}

	return PLATFORM_OK;
}


LocalPlatformProxy::ExitCode_t LocalPlatformProxy::LoadPlatformData() {
	ExitCode_t ec;

	ec = this->host->LoadPlatformData();
	if (ec != PLATFORM_OK) {
		return ec;
	}

	for (auto it=this->aux.begin() ; it < this->aux.end(); it++) {
		ec = (*it)->LoadPlatformData();
		if (ec != PLATFORM_OK) {
			return ec;
		}
	}

	return PLATFORM_OK;
}


LocalPlatformProxy::ExitCode_t LocalPlatformProxy::Refresh() {
	ExitCode_t ec;

	ec = this->host->Refresh();
	if (ec != PLATFORM_OK) {
		return ec;
	}

	for (auto it=this->aux.begin() ; it < this->aux.end(); it++) {
		ec = (*it)->Refresh();
		if (ec != PLATFORM_OK) {
			return ec;
		}
	}

	return PLATFORM_OK;
}


LocalPlatformProxy::ExitCode_t LocalPlatformProxy::Release(AppPtr_t papp) {
	ExitCode_t ec;

	ec = this->host->Release(papp);
	if (ec != PLATFORM_OK) {
		return ec;
	}

	for (auto it=this->aux.begin() ; it < this->aux.end(); it++) {
		ec = (*it)->Release(papp);
		if (ec != PLATFORM_OK) {
			return ec;
		}
	}

	return PLATFORM_OK;

}


LocalPlatformProxy::ExitCode_t LocalPlatformProxy::ReclaimResources(AppPtr_t papp) {

	ExitCode_t ec;

	ec = this->host->ReclaimResources(papp);
	if (ec != PLATFORM_OK) {
		return ec;
	}

	for (auto it=this->aux.begin() ; it < this->aux.end(); it++) {
		ec = (*it)->ReclaimResources(papp);
		if (ec != PLATFORM_OK) {
			return ec;
		}
	}

	return PLATFORM_OK;
}


LocalPlatformProxy::ExitCode_t LocalPlatformProxy::MapResources(
        AppPtr_t papp,
        ResourceAssignmentMapPtr_t pres,
        bool excl) {
	ExitCode_t ec;

	ec = this->host->MapResources(papp, pres, excl);
	if (ec != PLATFORM_OK) {
		return ec;
	}

	for (auto it=this->aux.begin() ; it < this->aux.end(); it++) {
		ec = (*it)->MapResources(papp, pres, excl);
		if (ec != PLATFORM_OK) {
			return ec;
		}
	}

	return PLATFORM_OK;
}


bool LocalPlatformProxy::IsHighPerformance(
		bbque::res::ResourcePathPtr_t const & path) const {
	if (path->GetID(bbque::res::ResourceType::CPU) >= 0)
		return this->host->IsHighPerformance(path);
	return false;
}


} // namespace pp
} // namespace bbque

