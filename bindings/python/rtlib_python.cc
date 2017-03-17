#include <pybind11/pybind11.h>
#include <bbque/bbque_exc.h>

using bbque::rtlib::BbqueEXC;
namespace py = pybind11;

/*
 * We wrap RTLIB_Services inside another object since pybind11 doesn't support
 * functions with double pointer arguments
 */
struct RTLIB_Services_Wrapper {
   RTLIB_Services_t *services;
};

class PyBbqueEXC : public BbqueEXC {
   public:
      using BbqueEXC::BbqueEXC;

      RTLIB_ExitCode_t onSetup() override {
         py::gil_scoped_release release;

         PYBIND11_OVERLOAD(
               RTLIB_ExitCode,
               BbqueEXC,
               onSetup,
               );
      };
      RTLIB_ExitCode_t onConfigure(int8_t awm_id) override {
         py::gil_scoped_release release;
         PYBIND11_OVERLOAD(
               RTLIB_ExitCode,
               BbqueEXC,
               onConfigure,
               awm_id
               );
      };
      RTLIB_ExitCode_t onSuspend() override {
         py::gil_scoped_release release;
         PYBIND11_OVERLOAD(
               RTLIB_ExitCode,
               BbqueEXC,
               onSuspend,
               );
      };
      RTLIB_ExitCode_t onResume() override {
         py::gil_scoped_release release;
         PYBIND11_OVERLOAD(
               RTLIB_ExitCode,
               BbqueEXC,
               onResume,
               );
      };
      RTLIB_ExitCode_t onRun() override {
         py::gil_scoped_release release;
         PYBIND11_OVERLOAD(
               RTLIB_ExitCode,
               BbqueEXC,
               onRun,
               );
      };
      RTLIB_ExitCode_t onMonitor() override {
         py::gil_scoped_release release;
         PYBIND11_OVERLOAD(
               RTLIB_ExitCode,
               BbqueEXC,
               onMonitor,
               );
      };
      RTLIB_ExitCode_t onRelease() override {
         py::gil_scoped_release release;
         PYBIND11_OVERLOAD(
               RTLIB_ExitCode,
               BbqueEXC,
               onRelease,
               );
      };
};


PYBIND11_PLUGIN(barbeque) {
   // python module declaration
   py::module m("barbeque", R"pbdoc(
      Barbeque bindings
      -----------------

      .. currentmodule:: barbeque

      .. autosummary::
         :toctree: _generate

   )pbdoc");

   // wrapper class for RTLIB_Services
   py::class_<RTLIB_Services_Wrapper>(m, "RTLIB_Services_Wrapper")
      .def(py::init<>())
      .def_readwrite("services", &RTLIB_Services_Wrapper::services);

   // RTLIB_Services struct
   py::class_<RTLIB_Services>(m, "RTLIB_Services")
      .def(py::init<>());

   // RTLIB_ExitCode enum
   py::enum_<RTLIB_ExitCode>(m, "RTLIB_ExitCode")
      .value("RTLIB_OK", RTLIB_ExitCode::RTLIB_OK)
      .value("RTLIB_ERROR", RTLIB_ExitCode::RTLIB_ERROR)
      .value("RTLIB_VERSION_MISMATCH", RTLIB_ExitCode::RTLIB_VERSION_MISMATCH)
      .value("RTLIB_NO_WORKING_MODE", RTLIB_ExitCode::RTLIB_NO_WORKING_MODE)
      .value("RTLIB_BBQUE_CHANNEL_SETUP_FAILED", RTLIB_ExitCode::RTLIB_BBQUE_CHANNEL_SETUP_FAILED)
      .value("RTLIB_BBQUE_CHANNEL_TEARDOWN_FAILED", RTLIB_ExitCode::RTLIB_BBQUE_CHANNEL_TEARDOWN_FAILED)
      .value("RTLIB_BBQUE_CHANNEL_WRITE_FAILED", RTLIB_ExitCode::RTLIB_BBQUE_CHANNEL_WRITE_FAILED)
      .value("RTLIB_BBQUE_CHANNEL_READ_FAILED", RTLIB_ExitCode::RTLIB_BBQUE_CHANNEL_READ_FAILED)
      .value("RTLIB_BBQUE_CHANNEL_READ_TIMEOUT", RTLIB_ExitCode::RTLIB_BBQUE_CHANNEL_READ_TIMEOUT)
      .value("RTLIB_BBQUE_CHANNEL_PROTOCOL_MISMATCH", RTLIB_ExitCode::RTLIB_BBQUE_CHANNEL_PROTOCOL_MISMATCH)
      .value("RTLIB_BBQUE_CHANNEL_UNAVAILABLE", RTLIB_ExitCode::RTLIB_BBQUE_CHANNEL_UNAVAILABLE)
      .value("RTLIB_BBQUE_CHANNEL_TIMEOUT", RTLIB_ExitCode::RTLIB_BBQUE_CHANNEL_TIMEOUT)
      .value("RTLIB_BBQUE_UNREACHABLE", RTLIB_ExitCode::RTLIB_BBQUE_UNREACHABLE)
      .value("RTLIB_EXC_DUPLICATE", RTLIB_ExitCode::RTLIB_EXC_DUPLICATE)
      .value("RTLIB_EXC_NOT_REGISTERED", RTLIB_ExitCode::RTLIB_EXC_NOT_REGISTERED)
      .value("RTLIB_EXC_REGISTRATION_FAILED", RTLIB_ExitCode::RTLIB_EXC_REGISTRATION_FAILED)
      .value("RTLIB_EXC_MISSING_RECIPE", RTLIB_ExitCode::RTLIB_EXC_MISSING_RECIPE)
      .value("RTLIB_EXC_UNREGISTRATION_FAILED", RTLIB_ExitCode::RTLIB_EXC_UNREGISTRATION_FAILED)
      .value("RTLIB_EXC_NOT_STARTED", RTLIB_ExitCode::RTLIB_EXC_NOT_STARTED)
      .value("RTLIB_EXC_ENABLE_FAILED", RTLIB_ExitCode::RTLIB_EXC_ENABLE_FAILED)
      .value("RTLIB_EXC_NOT_ENABLED", RTLIB_ExitCode::RTLIB_EXC_NOT_ENABLED)
      .value("RTLIB_EXC_DISABLE_FAILED", RTLIB_ExitCode::RTLIB_EXC_DISABLE_FAILED)
      .value("RTLIB_EXC_GWM_FAILED", RTLIB_ExitCode::RTLIB_EXC_GWM_FAILED)
      .value("RTLIB_EXC_GWM_START", RTLIB_ExitCode::RTLIB_EXC_GWM_START)
      .value("RTLIB_EXC_GWM_RECONF", RTLIB_ExitCode::RTLIB_EXC_GWM_RECONF)
      .value("RTLIB_EXC_GWM_MIGREC", RTLIB_ExitCode::RTLIB_EXC_GWM_MIGREC)
      .value("RTLIB_EXC_GWM_MIGRATE", RTLIB_ExitCode::RTLIB_EXC_GWM_MIGRATE)
      .value("RTLIB_EXC_GWM_BLOCKED", RTLIB_ExitCode::RTLIB_EXC_GWM_BLOCKED)
      .value("RTLIB_EXC_SYNC_MODE", RTLIB_ExitCode::RTLIB_EXC_SYNC_MODE)
      .value("RTLIB_EXC_SYNCP_FAILED", RTLIB_ExitCode::RTLIB_EXC_SYNCP_FAILED)
      .value("RTLIB_EXC_WORKLOAD_NONE", RTLIB_ExitCode::RTLIB_EXC_WORKLOAD_NONE)
      .value("RTLIB_EXC_CGROUP_NONE", RTLIB_ExitCode::RTLIB_EXC_CGROUP_NONE)
      .value("RTLIB_EXIT_CODE_COUNT", RTLIB_ExitCode::RTLIB_EXIT_CODE_COUNT)
      .export_values();

   py::enum_<RTLIB_ResourceType>(m, "RTLIB_ResourceType")
      .value("SYSTEM", RTLIB_ResourceType::SYSTEM)
      .value("CPU", RTLIB_ResourceType::CPU)
      .value("PROC_NR", RTLIB_ResourceType::PROC_NR)
      .value("PROC_ELEMENT", RTLIB_ResourceType::PROC_ELEMENT)
      .value("MEMORY", RTLIB_ResourceType::MEMORY)
      .value("GPU", RTLIB_ResourceType::GPU)
      .value("ACCELERATOR", RTLIB_ResourceType::ACCELERATOR)
      .export_values();

   // Our wrapped RTLIB_Init function
   m.def("RTLIB_Init",
         [](const char *name, RTLIB_Services_Wrapper &services) {
            return RTLIB_Init(name, &services.services);
         }, R"pbdoc(
      Initialize Execution Context
   )pbdoc");

   py::class_<BbqueEXC, PyBbqueEXC>(m, "BbqueEXC")
      .def(py::init<std::string const &,
            std::string const &,
            RTLIB_Services_t * const>())
      .def("Start", &BbqueEXC::Start)
      .def("WaitCompletion", &BbqueEXC::WaitCompletion)
      .def("Terminate", &BbqueEXC::Terminate)
      .def("isRegistered", &BbqueEXC::isRegistered)
      .def("Enable", &BbqueEXC::Enable)
      .def("SetAWMConstraints", &BbqueEXC::SetAWMConstraints)
      .def("ClearAWMConstraints", &BbqueEXC::ClearAWMConstraints)
      .def("SetGoalGap", &BbqueEXC::SetGoalGap)
      .def("GetUniqueID_String", &BbqueEXC::GetUniqueID_String)
      .def("GetUniqueID", &BbqueEXC::GetUniqueID)
      .def("GetAssignedResources", (RTLIB_ExitCode (BbqueEXC::*)(RTLIB_ResourceType, int32_t &))&BbqueEXC::GetAssignedResources)
      .def("GetAssignedResources", (RTLIB_ExitCode (BbqueEXC::*)(RTLIB_ResourceType, int32_t *, uint16_t))&BbqueEXC::GetAssignedResources)
      .def("GetAffinityMask", &BbqueEXC::GetAffinityMask)
      .def("SetCPS", &BbqueEXC::SetCPS)
      .def("SetCPSGoal", (RTLIB_ExitCode (BbqueEXC::*)(float, float))&BbqueEXC::SetCPSGoal)
      .def("SetJPSGoal", &BbqueEXC::SetJPSGoal)
      .def("UpdateJPC", &BbqueEXC::UpdateJPC)
      .def("SetMinimumCycleTimeUs", &BbqueEXC::SetMinimumCycleTimeUs)
      .def("GetCPS", &BbqueEXC::GetCPS)
      .def("GetJPS", &BbqueEXC::GetJPS)
      .def("GetCTimeMs", &BbqueEXC::GetCTimeMs)
      .def("GetCTimeUs", &BbqueEXC::GetCTimeUs)
      .def("Cycles", &BbqueEXC::Cycles)
      .def("WorkingModeParams", &BbqueEXC::WorkingModeParams)
      .def("Done", &BbqueEXC::Done)
      .def("CurrentAWM", &BbqueEXC::CurrentAWM)
      .def("Configuration", &BbqueEXC::Configuration);

   m.attr("__version__") = py::str("dev");

   return m.ptr();
}
