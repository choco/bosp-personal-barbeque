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

struct RTLIB_Resources_Amount_Wrapper {
   int32_t amount;
};

class RTLIB_Resources_Systems_Wrapper {
   public:
      RTLIB_Resources_Systems_Wrapper(uint16_t number_of_systems) :
         _number_of_systems(number_of_systems) {
         _systems.reserve(number_of_systems);
      }
      int32_t * systems() {
         return &_systems[0];
      }
      uint16_t number_of_systems() {
         return _number_of_systems;
      }
   private:
      uint16_t _number_of_systems;
      std::vector<int32_t> _systems;
};

class RTLIB_AffinityMasks_Wrapper {
   public:
      RTLIB_AffinityMasks_Wrapper(int number_of_masks) :
         _number_of_masks(number_of_masks) {
         _masks.reserve(number_of_masks);
      }
      int32_t * masks() {
         return &_masks[0];
      }
      int number_of_masks() {
         return _number_of_masks;
      }
   private:
      int _number_of_masks;
      std::vector<int32_t> _masks;
};

class RTLIB_Logger_Wrapper {
   public:
      RTLIB_Logger_Wrapper(std::unique_ptr<bu::Logger> &logger) :
         w_logger(logger){}
      void Debug(std::string msg) { w_logger->Debug(msg.c_str()); };
      void Info(std::string msg) { w_logger->Info(msg.c_str()); };
      void Notice(std::string msg) { w_logger->Notice(msg.c_str()); };
      void Warn(std::string msg) { w_logger->Warn(msg.c_str()); };
      void Error(std::string msg) { w_logger->Error(msg.c_str()); };
      void Crit(std::string msg) { w_logger->Crit(msg.c_str()); };
      void Alert(std::string msg) { w_logger->Alert(msg.c_str()); };
      void Fatal(std::string msg) { w_logger->Fatal(msg.c_str()); };

   private:
      std::unique_ptr<bu::Logger> &w_logger;
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

      std::unique_ptr<bu::Logger>& get_logger() {
         return logger;
      };
      std::string const get_exc_name() {
         return exc_name;
      };
      std::string const get_rpc_name() {
         return rpc_name;
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

   py::enum_<RTLIB_ConstraintOperation>(m, "RTLIB_ConstraintOperation")
      .value("CONSTRAINT_REMOVE", RTLIB_ConstraintOperation::CONSTRAINT_REMOVE)
      .value("CONSTRAINT_ADD", RTLIB_ConstraintOperation::CONSTRAINT_ADD)
      .export_values();

   py::enum_<RTLIB_ConstraintType>(m, "RTLIB_ConstraintType")
      .value("LOWER_BOUND", RTLIB_ConstraintType::LOWER_BOUND)
      .value("UPPER_BOUND", RTLIB_ConstraintType::UPPER_BOUND)
      .value("EXACT_VALUE", RTLIB_ConstraintType::EXACT_VALUE)
      .export_values();

   py::class_<RTLIB_Constraint>(m, "RTLIB_Constraint")
      .def_readwrite("awm", &RTLIB_Constraint::awm)
      .def_readwrite("operation", &RTLIB_Constraint::operation)
      .def_readwrite("type", &RTLIB_Constraint::type);

   py::class_<RTLIB_WorkingModeParams>(m, "RTLIB_WorkingModeParams")
      .def_readwrite("awm_id", &RTLIB_WorkingModeParams::awm_id)
      .def_readwrite("services", &RTLIB_WorkingModeParams::services)
      .def_readwrite("nr_sys", &RTLIB_WorkingModeParams::nr_sys)
      .def_readwrite("systems", &RTLIB_WorkingModeParams::systems);

   py::class_<RTLIB_Resources_Amount_Wrapper>(m, "RTLIB_Resources_Amount_Wrapper")
      .def(py::init<>())
      .def_readwrite("amount", &RTLIB_Resources_Amount_Wrapper::amount);

   py::class_<RTLIB_Resources_Systems_Wrapper>(m, "RTLIB_Resources_Systems_Wrapper")
      .def(py::init<uint16_t>())
      .def("systems", &RTLIB_Resources_Systems_Wrapper::systems);

   py::class_<RTLIB_AffinityMasks_Wrapper>(m, "RTLIB_AffinityMasks_Wrapper")
      .def(py::init<int>())
      .def("masks", &RTLIB_AffinityMasks_Wrapper::masks);

   py::class_<RTLIB_Logger_Wrapper>(m, "RTLIB_Logger_Wrapper")
      .def("Debug", &RTLIB_Logger_Wrapper::Debug)
      .def("Info", &RTLIB_Logger_Wrapper::Info)
      .def("Notice", &RTLIB_Logger_Wrapper::Notice)
      .def("Warn", &RTLIB_Logger_Wrapper::Warn)
      .def("Erro", &RTLIB_Logger_Wrapper::Error)
      .def("Crit", &RTLIB_Logger_Wrapper::Crit)
      .def("Alert", &RTLIB_Logger_Wrapper::Alert)
      .def("Fatal", &RTLIB_Logger_Wrapper::Fatal);

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
      .def("SetAWMConstraints",
            [](BbqueEXC &bbqueEXC, std::vector<RTLIB_Constraint> constraints)
            {
               return bbqueEXC.SetAWMConstraints(&constraints[0], constraints.size());
            })
      .def("ClearAWMConstraints", &BbqueEXC::ClearAWMConstraints)
      .def("SetGoalGap", &BbqueEXC::SetGoalGap)
      .def("GetUniqueID_String", &BbqueEXC::GetUniqueID_String)
      .def("GetUniqueID", &BbqueEXC::GetUniqueID)
      .def("GetAssignedResources",
            [](BbqueEXC &bbqueEXC, RTLIB_ResourceType r_type,
               RTLIB_Resources_Amount_Wrapper &r_amount_w)
            {
               return bbqueEXC.GetAssignedResources(r_type, r_amount_w.amount);
            })
      .def("GetAssignedResources",
            [](BbqueEXC &bbqueEXC, RTLIB_ResourceType r_type,
               RTLIB_Resources_Systems_Wrapper &r_systems_w)
            {
               return bbqueEXC.GetAssignedResources(r_type, r_systems_w.systems(), r_systems_w.number_of_systems());
            })
      .def("GetAffinityMask",
            [](BbqueEXC &bbqueEXC, RTLIB_AffinityMasks_Wrapper &a_masks_w)
            {
               return bbqueEXC.GetAffinityMask(a_masks_w.masks(), a_masks_w.number_of_masks());
            })
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
      .def("Configuration", &BbqueEXC::Configuration)
      .def_property_readonly("exc_name", &PyBbqueEXC::get_exc_name)
      .def_property_readonly("rpc_name", &PyBbqueEXC::get_rpc_name)
      .def_property_readonly("logger", [](PyBbqueEXC &bbqueEXC)
            {
               auto logger_w = new RTLIB_Logger_Wrapper(bbqueEXC.get_logger());
               return logger_w;
            }, py::return_value_policy::take_ownership);

   m.attr("__version__") = py::str("dev");

   return m.ptr();
}
