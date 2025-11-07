cmake_minimum_required(VERSION 3.10)
project(rrtDemo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Repository root is the parent of this subproject's source dir
get_filename_component(REPO_ROOT "${CMAKE_SOURCE_DIR}" DIRECTORY)
# Also consider the parent of the repository (workspace root) in case rrt_graph_builder lives alongside this repo
get_filename_component(WORKSPACE_ROOT "${REPO_ROOT}" DIRECTORY)

# If graphLib isn't vendored locally, use CPM to fetch it from GitHub
# Prefer a local rrt_graph_builder (if present), otherwise fall back to CPM fetching
if(EXISTS ${REPO_ROOT}/rrt_graph_builder OR EXISTS ${WORKSPACE_ROOT}/rrt_graph_builder)
    # Prefer a local copy inside this repo, otherwise use the sibling repo next to the workspace
    if(EXISTS ${REPO_ROOT}/rrt_graph_builder)
        set(_rrt_graph_builder_root "${REPO_ROOT}/rrt_graph_builder")
    else()
        set(_rrt_graph_builder_root "${WORKSPACE_ROOT}/rrt_graph_builder")
    endif()
    message(STATUS "Using local rrt_graph_builder at ${_rrt_graph_builder_root}")
    # Look for platform-specific prebuilt libs under prebuilt/<platform> (macOS -> macOS, Linux -> linux)
    if(APPLE)
        set(_platform_dir "macOS")
    else()
        set(_platform_dir "linux")
    endif()

    set(_prebuilt_dir "${_rrt_graph_builder_root}/prebuilt/${_platform_dir}")
    set(_build_prebuilt_dir "${_rrt_graph_builder_root}/build/graphLib")

    if(EXISTS ${_prebuilt_dir} AND (EXISTS ${_prebuilt_dir}/libgraphLib.a OR EXISTS ${_prebuilt_dir}/libgraphLib.dylib OR EXISTS ${_prebuilt_dir}/libgraphLib.so))
        message(STATUS "Found platform prebuilt graphLib in ${_prebuilt_dir}; using prebuilt libraries and headers from the upstream repo")
        set(graphLib_PREBUILT_DIR ${_prebuilt_dir})
        set(graphLib_SOURCE_DIR "${_rrt_graph_builder_root}/graphLib")
    elseif(EXISTS ${_build_prebuilt_dir} AND (EXISTS ${_build_prebuilt_dir}/libgraphLib.a OR EXISTS ${_build_prebuilt_dir}/libgraphLib.dylib OR EXISTS ${_build_prebuilt_dir}/libgraphLib.so))
        message(STATUS "Found prebuilt graphLib in ${_build_prebuilt_dir}; using prebuilt libraries and headers from the upstream repo")
        set(graphLib_PREBUILT_DIR ${_build_prebuilt_dir})
        set(graphLib_SOURCE_DIR "${_rrt_graph_builder_root}/graphLib")
    else()
        # Only add the graphLib subdirectory from the upstream repo to avoid building its rrtDemo example
        add_subdirectory(${_rrt_graph_builder_root}/graphLib ${CMAKE_BINARY_DIR}/_ext/rrt_graph_builder_graphLib EXCLUDE_FROM_ALL)
        set(graphLib_SOURCE_DIR "${_rrt_graph_builder_root}/graphLib")
    endif()
elseif(EXISTS ${REPO_ROOT}/graphLib)
    message(STATUS "Using local graphLib at ${REPO_ROOT}/graphLib")
    # Try to disable demos/examples in this dependency as well
    set(BUILD_EXAMPLES OFF CACHE BOOL "Disable examples in dependent project" FORCE)
    set(BUILD_EXAMPLE OFF CACHE BOOL "Disable example in dependent project" FORCE)
    set(BUILD_DEMOS OFF CACHE BOOL "Disable demos in dependent project" FORCE)
    set(BUILD_DEMO OFF CACHE BOOL "Disable demo in dependent project" FORCE)
    set(BUILD_RRTDEMO OFF CACHE BOOL "Disable rrtDemo in dependent project" FORCE)
    set(BUILD_RRT OFF CACHE BOOL "Disable rrt in dependent project" FORCE)
    add_subdirectory(${REPO_ROOT}/graphLib ${CMAKE_BINARY_DIR}/_ext/graphLib EXCLUDE_FROM_ALL)
    set(graphLib_SOURCE_DIR ${REPO_ROOT}/graphLib)
else()
    message(STATUS "graphLib not found locally — will fetch via CPM from https://github.com/shedlock987/rrt_graph_builder")
    # Download CPM.cmake into the build dir and include it (more robust than include(URL)).
    set(CPM_DIR "${CMAKE_BINARY_DIR}/cmake")
    set(CPM_FILE "${CPM_DIR}/CPM.cmake")
    if(NOT EXISTS "${CPM_FILE}")
        file(MAKE_DIRECTORY "${CPM_DIR}")
        # use STATUS/LOG to detect download failures and fail early with a helpful message
        file(DOWNLOAD "https://github.com/cpm-cmake/CPM.cmake/releases/latest/download/CPM.cmake" "${CPM_FILE}" STATUS _cpm_status LOG _cpm_log SHOW_PROGRESS)
        list(GET _cpm_status 0 _cpm_code)
        if(NOT _cpm_code EQUAL 0)
            message(FATAL_ERROR "Failed to download CPM.cmake (${_cpm_code}): ${_cpm_log})")
        endif()
    endif()
    include("${CPM_FILE}")
    if(NOT COMMAND CPMAddPackage)
        message(FATAL_ERROR "CPM did not provide CPMAddPackage after including ${CPM_FILE}")
    endif()
    CPMAddPackage(
        NAME graphLib
        GIT_REPOSITORY https://github.com/shedlock987/rrt_graph_builder.git
        GIT_TAG main
        SOURCE_SUBDIR graphLib
    )
    # CPM sets CPM_PACKAGE_graphLib_SOURCE_DIR to the temporary download location (e.g. .../build/_deps/graphlib-src)
    if(DEFINED CPM_PACKAGE_graphLib_SOURCE_DIR)
        # The actual graphLib subdir is under that location -> /graphLib
        set(graphLib_SOURCE_DIR "${CPM_PACKAGE_graphLib_SOURCE_DIR}/graphLib")
        message(STATUS "CPM provided graphLib source dir: ${graphLib_SOURCE_DIR}")
        include_directories(${graphLib_SOURCE_DIR}/include)
    endif()
endif()

find_package(PythonLibs REQUIRED)
find_package(Boost REQUIRED COMPONENTS python)

include_directories(${PYTHON_INCLUDE_DIRS})
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
if(DEFINED graphLib_SOURCE_DIR)
    include_directories(${graphLib_SOURCE_DIR}/include)
elseif(EXISTS ${REPO_ROOT}/graphLib/include)
    include_directories(${REPO_ROOT}/graphLib/include)
endif()

# Shared Python module
# If a target named rrtDemo was already created by an added subproject, reuse it
if(POLICY CMP0079)
    cmake_policy(SET CMP0079 NEW)
endif()

if(TARGET rrtDemo)
    message(STATUS "A target named 'rrtDemo' already exists; reusing it")
    set(RRTDEMO_TARGET rrtDemo)
else()
    add_library(rrtDemo SHARED src/rrtDemo.cpp)
    set(RRTDEMO_TARGET rrtDemo)
    set_target_properties(${RRTDEMO_TARGET} PROPERTIES PREFIX "" SUFFIX ".so")
endif()

# Link libraries: prefer CMake targets if available, otherwise fall back to named libs/variables
set(_LINK_LIBS)
if(DEFINED graphLib_PREBUILT_DIR)
    # Prefer linking against prebuilt static/dynamic libs from the upstream repo
    if(APPLE)
        if(EXISTS ${graphLib_PREBUILT_DIR}/libgraphLib.dylib)
            list(APPEND _LINK_LIBS "${graphLib_PREBUILT_DIR}/libgraphLib.dylib")
        elseif(EXISTS ${graphLib_PREBUILT_DIR}/libgraphLib.a)
            list(APPEND _LINK_LIBS "${graphLib_PREBUILT_DIR}/libgraphLib.a")
        endif()
        if(EXISTS ${graphLib_PREBUILT_DIR}/librrtLib.dylib)
            list(APPEND _LINK_LIBS "${graphLib_PREBUILT_DIR}/librrtLib.dylib")
        elseif(EXISTS ${graphLib_PREBUILT_DIR}/librrtLib.a)
            list(APPEND _LINK_LIBS "${graphLib_PREBUILT_DIR}/librrtLib.a")
        endif()
    else()
        if(EXISTS ${graphLib_PREBUILT_DIR}/libgraphLib.so)
            list(APPEND _LINK_LIBS "${graphLib_PREBUILT_DIR}/libgraphLib.so")
        elseif(EXISTS ${graphLib_PREBUILT_DIR}/libgraphLib.a)
            list(APPEND _LINK_LIBS "${graphLib_PREBUILT_DIR}/libgraphLib.a")
        endif()
        if(EXISTS ${graphLib_PREBUILT_DIR}/librrtLib.so)
            list(APPEND _LINK_LIBS "${graphLib_PREBUILT_DIR}/librrtLib.so")
        elseif(EXISTS ${graphLib_PREBUILT_DIR}/librrtLib.a)
            list(APPEND _LINK_LIBS "${graphLib_PREBUILT_DIR}/librrtLib.a")
        endif()
    endif()
elseif(TARGET graphLib)
    list(APPEND _LINK_LIBS graphLib)
elseif(DEFINED graphLib_SOURCE_DIR)
    # If graphLib wasn't exported as a target, rely on include paths and static libs in that project
    list(APPEND _LINK_LIBS graphLib)
else()
    list(APPEND _LINK_LIBS graphLib)
endif()

# If we did not link prebuilt libs above, link rrtLib (either a target or by name)
if(NOT DEFINED graphLib_PREBUILT_DIR)
    if(TARGET rrtLib)
        list(APPEND _LINK_LIBS rrtLib)
    else()
        list(APPEND _LINK_LIBS rrtLib)
    endif()
endif()

list(APPEND _LINK_LIBS ${Boost_PYTHON_LIBRARY} ${PYTHON_LIBRARIES})

target_link_libraries(${RRTDEMO_TARGET} ${_LINK_LIBS})

# Ensure the rrtDemo target has the proper include paths (graphLib headers, boost, python, and local includes)
target_include_directories(${RRTDEMO_TARGET} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
if(DEFINED graphLib_SOURCE_DIR)
    target_include_directories(${RRTDEMO_TARGET} PRIVATE ${graphLib_SOURCE_DIR}/include)
elseif(EXISTS ${REPO_ROOT}/graphLib/include)
    target_include_directories(${RRTDEMO_TARGET} PRIVATE ${REPO_ROOT}/graphLib/include)
endif()
if(DEFINED PYTHON_INCLUDE_DIRS)
    target_include_directories(${RRTDEMO_TARGET} PRIVATE ${PYTHON_INCLUDE_DIRS})
endif()
if(DEFINED Boost_INCLUDE_DIRS)
    target_include_directories(${RRTDEMO_TARGET} PRIVATE ${Boost_INCLUDE_DIRS})
endif()

# Executable target: build from a separate main.cpp that provides int main()
#add_executable(rrtDemoMain src/main.cpp)
## Ensure executable links the shared module (rrtDemo) which contains VisRRT implementation
#target_link_libraries(rrtDemoMain
#    rrtDemo
#    rrtLib
#    graphLib
#    ${Boost_PYTHON_LIBRARY}
#    ${PYTHON_LIBRARIES}
#)
#add_dependencies(rrtDemoMain rrtDemo)
