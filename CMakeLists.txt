cmake_minimum_required(VERSION 3.25)

project(FooPodBridge VERSION 0.1.0 LANGUAGES CXX RC)

set(FOOPODBRIDGE_DISPLAY_VERSION "0.1.0-beta.1")

if(NOT WIN32)
  message(FATAL_ERROR "FooPodBridge currently supports Windows only.")
endif()

if(NOT MSVC)
  message(FATAL_ERROR "FooPodBridge requires the Microsoft Visual C++ toolchain.")
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(FATAL_ERROR "FooPodBridge must be configured for x64.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

include(CTest)
include(cmake/Foobar2000Sdk.cmake)

add_library(foopodbridge_core STATIC
  src/core/version.cpp
)
target_include_directories(foopodbridge_core PUBLIC include)
target_compile_options(foopodbridge_core PRIVATE /W4 /WX /permissive- /Zc:__cplusplus /utf-8)

# These interface targets are permanent dependency boundaries. Later tasks add
# real implementations without changing the allowed Core dependency direction.
foreach(core_module IN ITEMS device media transaction)
  add_library(foopodbridge_core_${core_module} INTERFACE)
  target_link_libraries(foopodbridge_core_${core_module} INTERFACE foopodbridge_core)
  target_include_directories(foopodbridge_core_${core_module} INTERFACE include)
endforeach()

add_library(foopodbridge_core_database STATIC
  src/core/database/reader.cpp
  src/core/database/edit.cpp
  src/core/database/writer.cpp
  src/core/database/validator.cpp
  src/core/database/comparator.cpp
)
target_link_libraries(foopodbridge_core_database PUBLIC foopodbridge_core)
target_include_directories(foopodbridge_core_database PUBLIC include)
target_compile_options(foopodbridge_core_database PRIVATE /W4 /WX /permissive- /Zc:__cplusplus /utf-8)

add_library(foopodbridge_contract STATIC
  src/contract/service_guids.cpp
)
target_include_directories(foopodbridge_contract PUBLIC include)
target_link_libraries(foopodbridge_contract PUBLIC
  foopodbridge_foobar2000_sdk
  foopodbridge_pfc
)
target_compile_options(foopodbridge_contract PRIVATE /W4 /WX /permissive- /Zc:__cplusplus /utf-8)

add_library(foo_pod_bridge MODULE
  src/foobar/component.cpp
  src/foobar/foo_pod_bridge.rc
)
set_target_properties(foo_pod_bridge PROPERTIES
  PREFIX ""
  OUTPUT_NAME "foo_pod_bridge"
)
target_include_directories(foo_pod_bridge PRIVATE include src)
target_compile_definitions(foo_pod_bridge PRIVATE
  _UNICODE
  UNICODE
  _WINDLL
  WIN32_LEAN_AND_MEAN
  NOMINMAX
)
target_compile_options(foo_pod_bridge PRIVATE /W4 /WX /permissive- /Zc:__cplusplus /utf-8 /external:W0)
target_link_options(foo_pod_bridge PRIVATE /INCREMENTAL:NO)
target_link_libraries(foo_pod_bridge PRIVATE
  foopodbridge_core
  foopodbridge_contract
  foopodbridge_foobar2000_sdk
  foopodbridge_component_client
  foopodbridge_pfc
  foopodbridge_shared
  comctl32
  shlwapi
  shell32
  version
  winmm
)

if(BUILD_TESTING)
  add_executable(foopodbridge_core_version_tests tests/core_version_tests.cpp)
  target_link_libraries(foopodbridge_core_version_tests PRIVATE foopodbridge_core)
  target_compile_options(foopodbridge_core_version_tests PRIVATE /W4 /WX /permissive- /Zc:__cplusplus /utf-8)
  add_test(NAME core_version COMMAND foopodbridge_core_version_tests)

  add_executable(foopodbridge_contract_tests tests/contract_tests.cpp)
  target_link_libraries(foopodbridge_contract_tests PRIVATE foopodbridge_contract)
  target_compile_definitions(foopodbridge_contract_tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
  target_compile_options(foopodbridge_contract_tests PRIVATE /W4 /WX /permissive- /Zc:__cplusplus /utf-8 /external:W0)
  add_test(NAME service_contract COMMAND foopodbridge_contract_tests)

  add_executable(foopodbridge_component_identity_tests tests/component_identity_tests.cpp)
  target_include_directories(foopodbridge_component_identity_tests PRIVATE src)
  target_compile_definitions(foopodbridge_component_identity_tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
  target_compile_options(foopodbridge_component_identity_tests PRIVATE /W4 /WX /permissive- /Zc:__cplusplus /utf-8)
  target_link_libraries(foopodbridge_component_identity_tests PRIVATE ole32)
  add_test(NAME component_identity COMMAND foopodbridge_component_identity_tests)

  foreach(database_test IN ITEMS reader writer corruption private_fixture)
    add_executable(foopodbridge_database_${database_test}_tests
      tests/database_${database_test}_tests.cpp
    )
    target_link_libraries(foopodbridge_database_${database_test}_tests PRIVATE foopodbridge_core_database)
    target_compile_options(foopodbridge_database_${database_test}_tests PRIVATE /W4 /WX /permissive- /Zc:__cplusplus /utf-8)
    add_test(NAME database_${database_test} COMMAND foopodbridge_database_${database_test}_tests)
  endforeach()
  target_compile_definitions(foopodbridge_database_private_fixture_tests PRIVATE
    FOOPODBRIDGE_PRIVATE_FIXTURE_ROOT="${CMAKE_SOURCE_DIR}/tests/private-fixtures"
  )

  find_program(FOOPODBRIDGE_POWERSHELL_EXECUTABLE NAMES powershell REQUIRED)
  add_test(NAME source_boundaries
    COMMAND "${FOOPODBRIDGE_POWERSHELL_EXECUTABLE}"
      -NoProfile
      -ExecutionPolicy Bypass
      -File "${CMAKE_SOURCE_DIR}/scripts/check-source-boundaries.ps1"
      -RepositoryRoot "${CMAKE_SOURCE_DIR}"
  )
endif()

find_program(FOOPODBRIDGE_POWERSHELL_EXECUTABLE NAMES powershell REQUIRED)

add_custom_target(package_component
  COMMAND "${FOOPODBRIDGE_POWERSHELL_EXECUTABLE}"
    -NoProfile
    -ExecutionPolicy Bypass
    -File "${CMAKE_SOURCE_DIR}/scripts/package-component.ps1"
    -RepositoryRoot "${CMAKE_SOURCE_DIR}"
    -ComponentDll "$<TARGET_FILE:foo_pod_bridge>"
    -OutputDirectory "${CMAKE_SOURCE_DIR}/dist"
    -Version "${FOOPODBRIDGE_DISPLAY_VERSION}"
  DEPENDS foo_pod_bridge
  VERBATIM
)
