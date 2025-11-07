cmake_minimum_required(VERSION 3.10)
project(rrtDemo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# === REPOSITORY & WORKSPACE ROOTS ===
get_filename_component(REPO_ROOT "${CMAKE_SOURCE_DIR}" DIRECTORY)
get_filename_component(WORKSPACE_ROOT "${REPO_ROOT}" DIRECTORY)

# === FIND ACTIVE PYTHON (venv/conda/system) ===
set(Python3_FIND_STRATEGY LOCATION)
set(Python3_FIND_REGISTRY NEVER)
find_package(Python3 REQUIRED COMPONENTS Interpreter Development.Module)

# Extract exact paths via sysconfig
execute_process(
    COMMAND "${Python3_EXECUTABLE}" -c "import sysconfig, sys; print(sysconfig.get_config_var('LIBDIR') or sys.prefix + '/lib')"
    OUTPUT_VARIABLE Python3_LIBDIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
    COMMAND "${Python3_EXECUTABLE}" -c "import sysconfig; print(sysconfig.get_config_var('INCLUDEDIR'))"
    OUTPUT_VARIABLE Python3_INCLUDEDIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# === DYNAMIC BOOST.PYTHON COMPONENT NAME (CORRECT MAPPING) ===
execute_process(
    COMMAND "${Python3_EXECUTABLE}" -c "
import sys
major, minor = sys.version_info[:2]
candidates = [f'boost_python{major}{minor}', f'boost_python{major}', 'boost_python']
found = None
for lib in candidates:
    try:
        __import__(lib)
        found = lib
        break
    except ImportError:
        pass

# Map Python import name → Boost CMake component name
if found:
    if found == 'boost_python':
        component = 'python'
    else:
        suffix = found[len('boost_python'):]
        if suffix.isdigit() and len(suffix) == 2:
            component = f'python{suffix[0]}{suffix[1:]}'  # e.g. 311 → python311
        elif suffix.isdigit() and len(suffix) == 1:
            component = f'python{suffix}'
        else:
            component = f'python{suffix}'
else:
    component = 'python'

print(component)
"
    OUTPUT_VARIABLE BOOST_PYTHON_COMPONENT
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

message(STATUS "Using Python: ${Python3_EXECUTABLE}")
message(STATUS "Python libdir: ${Python3_LIBDIR}")
message(STATUS "Python includedir: ${Python3_INCLUDEDIR}")
message(STATUS "Detected Boost.Python component: ${BOOST_PYTHON_COMPONENT}")

# === FIND BOOST WITH CORRECT COMPONENT ===
find_package(Boost REQUIRED COMPONENTS ${BOOST_PYTHON_COMPONENT})

# === PLATFORM-SPECIFIC MODULE SUFFIX (macOS uses .so for Python modules) ===
if(APPLE)
    set(PY_MODULE_SUFFIX ".so")
else()
    set(PY_MODULE_SUFFIX ".so")
endif()

# === DEPENDENCY: graphLib / rrt_graph_builder ===
if(EXISTS ${REPO_ROOT}/rrt_graph_builder OR EXISTS ${WORKSPACE_ROOT}/rrt_graph_builder)
    if(EXISTS ${REPO_ROOT}/rrt_graph_builder)
        set(_rrt_graph_builder_root "${REPO_ROOT}/rrt_graph_builder")
    else()
        set(_rrt_graph_builder_root "${WORKSPACE_ROOT}/rrt_graph_builder")
    endif()
    message(STATUS "Using local rrt_graph_builder at ${_rrt_graph_builder_root}")

    if(APPLE)
        set(_platform_dir "macOS")
    else()
        set(_platform_dir "linux")
    endif()

    set(_prebuilt_dir "${_rrt_graph_builder_root}/prebuilt/${_platform_dir}")
    set(_build_prebuilt_dir "${_rrt_graph_builder_root}/build/graphLib")

    if(EXISTS ${_prebuilt_dir} AND (EXISTS ${_prebuilt_dir}/libgraphLib.a OR EXISTS ${_prebuilt_dir}/libgraphLib.dylib OR EXISTS ${_prebuilt_dir}/libgraphLib.so))
        message(STATUS "Found platform prebuilt graphLib in ${_prebuilt_dir}")
        set(graphLib_PREBUILT_DIR ${_prebuilt_dir})
        set(graphLib_SOURCE_DIR "${_rrt_graph_builder_root}/graphLib")
    elseif(EXISTS ${_build_prebuilt_dir} AND (EXISTS ${_build_prebuilt_dir}/libgraphLib.a OR EXISTS ${_build_prebuilt_dir}/libgraphLib.dylib OR EXISTS ${_build_prebuilt_dir}/libgraphLib.so))
        message(STATUS "Found prebuilt graphLib in ${_build_prebuilt_dir}")
        set(graphLib_PREBUILT_DIR ${_build_prebuilt_dir})
        set(graphLib_SOURCE_DIR "${_rrt_graph_builder_root}/graphLib")
    else()
        add_subdirectory(${_rrt_graph_builder_root}/graphLib ${CMAKE_BINARY_DIR}/_ext/rrt_graph_builder_graphLib EXCLUDE_FROM_ALL)
        set(graphLib_SOURCE_DIR "${_rrt_graph_builder_root}/graphLib")
    endif()
elseif(EXISTS ${REPO_ROOT}/graphLib)
    message(STATUS "Using local graphLib at ${REPO_ROOT}/graphLib")
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
    set(BUILD_DEMOS OFF CACHE BOOL "" FORCE)
    set(BUILD_DEMO OFF CACHE BOOL "" FORCE)
    set(BUILD_RRTDEMO OFF CACHE BOOL "" FORCE)
    set(BUILD_RRT OFF CACHE BOOL "" FORCE)
    add_subdirectory(${REPO_ROOT}/graphLib ${CMAKE_BINARY_DIR}/_ext/graphLib EXCLUDE_FROM_ALL)
    set(graphLib_SOURCE_DIR ${REPO_ROOT}/graphLib)
else()
    message(STATUS "graphLib not found locally — fetching via CPM")
    set(CPM_DIR "${CMAKE_BINARY_DIR}/cmake")
    set(CPM_FILE "${CPM_DIR}/CPM.cmake")
    if(NOT EXISTS "${CPM_FILE}")
        file(MAKE_DIRECTORY "${CPM_DIR}")
        file(DOWNLOAD "https://github.com/cpm-cmake/CPM.cmake/releases/latest/download/CPM.cmake"
             "${CPM_FILE}" STATUS _cpm_status LOG _cpm_log SHOW_PROGRESS)
        list(GET _cpm_status 0 _cpm_code)
        if(NOT _cpm_code EQUAL 0)
            message(FATAL_ERROR "Failed to download CPM.cmake: ${_cpm_log}")
        endif()
    endif()
    include("${CPM_FILE}")
    CPMAddPackage(
        NAME graphLib
        GIT_REPOSITORY https://github.com/shedlock987/rrt_graph_builder.git
        GIT_TAG main
        SOURCE_SUBDIR graphLib
    )
    if(DEFINED CPM_PACKAGE_graphLib_SOURCE_DIR)
        set(graphLib_SOURCE_DIR "${CPM_PACKAGE_graphLib_SOURCE_DIR}/graphLib")
        message(STATUS "CPM provided graphLib at: ${graphLib_SOURCE_DIR}")
    endif()
endif()

# === INCLUDE DIRECTORIES (target defined later) ===
# We'll set includes on the target after creation

# === BUILD SHARED PYTHON MODULE ===
if(POLICY CMP0079)
    cmake_policy(SET CMP0079 NEW)
endif()

if(TARGET rrtDemo)
    message(STATUS "Reusing existing 'rrtDemo' target")
    set(RRTDEMO_TARGET rrtDemo)
else()
    add_library(rrtDemo SHARED src/rrtDemo.cpp)
    set(RRTDEMO_TARGET rrtDemo)
    set_target_properties(${RRTDEMO_TARGET} PROPERTIES
        PREFIX ""
        SUFFIX "${PY_MODULE_SUFFIX}"
        OUTPUT_NAME "rrtDemo"
    )
endif()

# === APPLY INCLUDE DIRECTORIES TO TARGET ===
target_include_directories(${RRTDEMO_TARGET} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${Python3_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS}
)
if(DEFINED graphLib_SOURCE_DIR)
    target_include_directories(${RRTDEMO_TARGET} PRIVATE ${graphLib_SOURCE_DIR}/include)
elseif(EXISTS ${REPO_ROOT}/graphLib/include)
    target_include_directories(${RRTDEMO_TARGET} PRIVATE ${REPO_ROOT}/graphLib/include)
endif()

# === LINK LIBRARIES ===
set(_LINK_LIBS)

# Prebuilt graphLib/rrtLib
if(DEFINED graphLib_PREBUILT_DIR)
    if(APPLE)
        foreach(lib IN ITEMS graphLib rrtLib)
            if(EXISTS ${graphLib_PREBUILT_DIR}/lib${lib}.dylib)
                list(APPEND _LINK_LIBS "${graphLib_PREBUILT_DIR}/lib${lib}.dylib")
            elseif(EXISTS ${graphLib_PREBUILT_DIR}/lib${lib}.a)
                list(APPEND _LINK_LIBS "${graphLib_PREBUILT_DIR}/lib${lib}.a")
            endif()
        endforeach()
    else()
        foreach(lib IN ITEMS graphLib rrtLib)
            if(EXISTS ${graphLib_PREBUILT_DIR}/lib${lib}.so)
                list(APPEND _LINK_LIBS "${graphLib_PREBUILT_DIR}/lib${lib}.so")
            elseif(EXISTS ${graphLib_PREBUILT_DIR}/lib${lib}.a)
                list(APPEND _LINK_LIBS "${graphLib_PREBUILT_DIR}/lib${lib}.a")
            endif()
        endforeach()
    endif()
else()
    if(TARGET graphLib)
        list(APPEND _LINK_LIBS graphLib)
    endif()
    if(TARGET rrtLib)
        list(APPEND _LINK_LIBS rrtLib)
    endif()
endif()

# Boost.Python + Python
list(APPEND _LINK_LIBS Boost::python ${Python3_LIBRARIES})
if(Python3_LIBDIR)
    target_link_directories(${RRTDEMO_TARGET} PRIVATE ${Python3_LIBDIR})
endif()

target_link_libraries(${RRTDEMO_TARGET} ${_LINK_LIBS})

# === OPTIONAL: Strip in Release ===
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_custom_command(TARGET ${RRTDEMO_TARGET} POST_BUILD
        COMMAND ${CMAKE_STRIP} $<TARGET_FILE:${RRTDEMO_TARGET}>
        COMMENT "Stripping rrtDemo.so"
    )
endif()

# === (Commented) Executable Demo ===
#add_executable(rrtDemoMain src/main.cpp)
#target_link_libraries(rrtDemoMain rrtDemo ${Python3_LIBRARIES} Boost::python)
#add_dependencies(rrtDemoMain rrtDemo)