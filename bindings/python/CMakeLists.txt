find_package(PythonInterp)
find_package(PythonLibs REQUIRED)

if (PYTHONLIBS_FOUND)
  include_directories ("${PYTHON_INCLUDE_DIRS}")
else ()
  message (FATAL_ERROR "Unable to find PythonLibs.")
endif()

add_library(bbque_python_bindings MODULE
  rtlib_python.cc
  )

set_target_properties(bbque_python_bindings PROPERTIES
  OUTPUT_NAME "barbeque"
  PREFIX ""
  )

target_link_libraries(bbque_python_bindings
  bbque_rtlib
  ${Boost_LIBRARIES}
  ${PYTHON_LIBRARIES}
  )
