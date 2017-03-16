#include <pybind11/pybind11.h>
#include <bbque/bbque_exc.h>

struct RTLIB_Services_Wrapper {
   RTLIB_Services_t **services;
};

RTLIB_ExitCode_t RTLIB_Wrapped_Init(const char * name, RTLIB_Services_Wrapper services) {
   RTLIB_Services_t **test = services.services;
   return RTLIB_Init(name, test);
}

namespace py = pybind11;

PYBIND11_PLUGIN(barbeque) {
   py::module m("barbeque", R"pbdoc(
      Barbeque bindings
      -----------------

      .. currentmodule:: barbeque

      .. autosummary::
         :toctree: _generate

)pbdoc");

   m.def("RTLIB_Init", &RTLIB_Wrapped_Init, R"pbdoc(
      Initialize Execution Context
   )pbdoc");

   m.attr("__version__") = py::str("dev");

   return m.ptr();
}
