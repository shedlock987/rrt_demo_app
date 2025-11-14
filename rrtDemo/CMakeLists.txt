cmake_minimum_required(VERSION 3.18)  # Supports Python3 3.14+
project(rrtDemo LANGUAGES CXX)

# --------------------------------------------------------------
# 1. REPOSITORY & WORKSPACE ROOTS
# --------------------------------------------------------------
get_filename_component(REPO_ROOT "${CMAKE_SOURCE_DIR}" DIRECTORY)     # .../rrt_demo_app
get_filename_component(WORKSPACE_ROOT "${REPO_ROOT}" DIRECTORY)      # .../workspace

# --------------------------------------------------------------
# 2. FIND ACTIVE PYTHON
# --------------------------------------------------------------
set(Python3_FIND_STRATEGY LOCATION)      # Prefer active interpreter (venv)
set(Python3_FIND_REGISTRY NEVER)         # Avoid macOS registry junk
set(Python3_FIND_FRAMEWORK NEVER)        # We'll handle framework manually

# Require Python 3 with Interpreter + Development
find_package(Python3 REQUIRED COMPONENTS Interpreter Development.Module)

# Extract version
execute_process(
    COMMAND "${Python3_EXECUTABLE}" -c "import sys; print(f'{sys.version_info.major}.{sys.version_info.minor}')"
    OUTPUT_VARIABLE _PYTHON_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

message(STATUS "Using Python ${_PYTHON_VERSION}: ${Python3_EXECUTABLE}")
message(STATUS "Python include dirs: ${Python3_INCLUDE_DIRS}")
message(STATUS "Python libraries: ${Python3_LIBRARIES}")

# macOS: Homebrew framework fallback
if(APPLE AND NOT EXISTS "${Python3_INCLUDE_DIRS}/Python.h")
    set(_framework_dir "/opt/homebrew/Frameworks/Python.framework")
    if(EXISTS "${_framework_dir}/Versions/${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}/Headers/Python.h")
        set(Python3_INCLUDE_DIRS "${_framework_dir}/Versions/${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}/Headers")
        message(STATUS "Using Homebrew Python.framework headers: ${Python3_INCLUDE_DIRS}")
    endif()
endif()

# --------------------------------------------------------------
# 2b. FIND PYBIND11 
# --------------------------------------------------------------
find_package(pybind11 REQUIRED)
message(STATUS "Pybind11 version: ${pybind11_VERSION}")
message(STATUS "Pybind11 include dirs: ${pybind11_INCLUDE_DIRS}")

# --------------------------------------------------------------
# 3. PLATFORM-SPECIFIC PREBUILT SUBDIR
# --------------------------------------------------------------
if(APPLE)
    set(PLATFORM_SUBDIR "macOS")
else()
    set(PLATFORM_SUBDIR "linux")
endif()

# --------------------------------------------------------------
# 4. LOCATE SIBLING rrt_graph_builder
# --------------------------------------------------------------
set(_rrt_graph_builder_root "")
if(EXISTS "${REPO_ROOT}/rrt_graph_builder")
    set(_rrt_graph_builder_root "${REPO_ROOT}/rrt_graph_builder")
elseif(EXISTS "${WORKSPACE_ROOT}/rrt_graph_builder")
    set(_rrt_graph_builder_root "${WORKSPACE_ROOT}/rrt_graph_builder")
endif()

if(NOT _rrt_graph_builder_root)
    message(FATAL_ERROR "rrt_graph_builder not found in ${REPO_ROOT} or ${WORKSPACE_ROOT}")
endif()
message(STATUS "Found rrt_graph_builder at: ${_rrt_graph_builder_root}")

# --------------------------------------------------------------
# 5. PREBUILT GRAPH LIB
# --------------------------------------------------------------
set(_graphlib_prebuilt_dir "${_rrt_graph_builder_root}/prebuilt/${PLATFORM_SUBDIR}")
if(EXISTS "${_graphlib_prebuilt_dir}")
    message(STATUS "Using prebuilt graphLib from: ${_graphlib_prebuilt_dir}")
    set(graphLib_PREBUILT_DIR "${_graphlib_prebuilt_dir}")
else()
    set(_graphlib_build_dir "${_rrt_graph_builder_root}/build/graphLib")
    if(EXISTS "${_graphlib_build_dir}")
        set(graphLib_PREBUILT_DIR "${_graphlib_build_dir}")
    else()
        add_subdirectory("${_rrt_graph_builder_root}/graphLib"
                         "${CMAKE_BINARY_DIR}/_ext/rrt_graph_builder_graphLib"
                         EXCLUDE_FROM_ALL)
    endif()
endif()

# --------------------------------------------------------------
# 6. CREATE LOCAL PREBUILT DIRECTORY
# --------------------------------------------------------------
set(RRTDEMO_PREBUILT_DIR "${REPO_ROOT}/prebuilt/${PLATFORM_SUBDIR}")
file(MAKE_DIRECTORY "${RRTDEMO_PREBUILT_DIR}")

# --------------------------------------------------------------
# 7. BUILD PYTHON MODULE
# --------------------------------------------------------------
pybind11_add_module(rrtDemo src/rrtDemo.cpp)  # Use pybind11_add_module instead of add_library
set_target_properties(rrtDemo PROPERTIES
    OUTPUT_NAME "rrtDemo"
)

target_compile_features(rrtDemo PRIVATE cxx_std_17)
target_include_directories(rrtDemo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${Python3_INCLUDE_DIRS}
)

# Include graphLib headers
if(EXISTS "${_rrt_graph_builder_root}/graphLib/include")
    target_include_directories(rrtDemo PRIVATE "${_rrt_graph_builder_root}/graphLib/include")
endif()
if(graphLib_PREBUILT_DIR AND EXISTS "${graphLib_PREBUILT_DIR}/include")
    target_include_directories(rrtDemo PRIVATE "${graphLib_PREBUILT_DIR}/include")
endif()

# --------------------------------------------------------------
# 8. LINK LIBRARIES
# --------------------------------------------------------------
set(_LINK_LIBS ${Python3_LIBRARIES})

if(graphLib_PREBUILT_DIR)
    if(APPLE)
        foreach(lib IN ITEMS graphLib rrtLib)
            set(_path "${graphLib_PREBUILT_DIR}/lib${lib}.dylib")
            if(EXISTS "${_path}")
                list(APPEND _LINK_LIBS "${_path}")
            endif()
        endforeach()
    else()
        foreach(lib IN ITEMS graphLib rrtLib)
            set(_path "${graphLib_PREBUILT_DIR}/lib${lib}.so")
            if(EXISTS "${_path}")
                list(APPEND _LINK_LIBS "${_path}")
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

target_link_libraries(rrtDemo PRIVATE ${_LINK_LIBS})

# --------------------------------------------------------------
# 9. COPY rrtDemo.so TO PREBUILT
# --------------------------------------------------------------
add_custom_command(TARGET rrtDemo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:rrtDemo>
            "${RRTDEMO_PREBUILT_DIR}/rrtDemo.so"
    COMMENT "Copying rrtDemo.so → ${RRTDEMO_PREBUILT_DIR}"
)

# --------------------------------------------------------------
# 10. COPY GRAPHLIB (optional)
# --------------------------------------------------------------
option(COPY_GRAPHLIB "Copy libgraphLib into rrtDemo prebuilt dir" ON)
if(graphLib_PREBUILT_DIR AND COPY_GRAPHLIB)
    if(APPLE)
        set(_graphlib_file "${graphLib_PREBUILT_DIR}/libgraphLib.dylib")
    else()
        set(_graphlib_file "${graphLib_PREBUILT_DIR}/libgraphLib.so")
    endif()
    if(EXISTS "${_graphlib_file}")
        add_custom_command(TARGET rrtDemo POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${_graphlib_file}"
                    "${RRTDEMO_PREBUILT_DIR}/"
            COMMENT "Copying libgraphLib → ${RRTDEMO_PREBUILT_DIR}"
        )
    endif()
endif()

# --------------------------------------------------------------
# 11. macOS: Allow dynamic Python lookup
# --------------------------------------------------------------
if(APPLE)
    target_link_options(rrtDemo PRIVATE "-undefined" "dynamic_lookup")
endif()

# --------------------------------------------------------------
# 12. Strip in Release
# --------------------------------------------------------------
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_custom_command(TARGET rrtDemo POST_BUILD
        COMMAND ${CMAKE_STRIP} $<TARGET_FILE:rrtDemo>
        COMMENT "Stripping rrtDemo.so"
    )
endif()

# --------------------------------------------------------------
# C++ Standard
# --------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)