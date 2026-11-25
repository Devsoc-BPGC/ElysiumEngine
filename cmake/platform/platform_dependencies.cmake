# ==============================================================================
#  Shared dependency resolution (LIBDIR -> find_package -> FetchContent)
# ==============================================================================

if(ELYSIUM_LIBDIR AND EXISTS "${ELYSIUM_LIBDIR}")
    message(STATUS "Using ELYSIUM_LIBDIR: ${ELYSIUM_LIBDIR}")
    list(PREPEND CMAKE_PREFIX_PATH
        "${ELYSIUM_LIBDIR}/sfml"
        "${ELYSIUM_LIBDIR}/spdlog"
        "${ELYSIUM_LIBDIR}"
    )
elseif(ELYSIUM_LIBDIR)
    message(WARNING "ELYSIUM_LIBDIR is set to '${ELYSIUM_LIBDIR}', but the directory does not exist.")
endif()

# --- 1. Resolve SFML 3.x ---
find_package(SFML 3 COMPONENTS Graphics Window System Audio QUIET)

if(NOT SFML_FOUND)
    message(STATUS "SFML 3 not found via find_package; falling back to FetchContent (3.1.0)")
    include(FetchContent)

    set(SFML_BUILD_EXAMPLES      OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_TEST_PROGRAMS OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_DOC           OFF CACHE BOOL "" FORCE)
    set(SFML_INSTALL             OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_NETWORK       OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_AUDIO         ON  CACHE BOOL "" FORCE)

    FetchContent_Declare(
        SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG        3.1.0
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(SFML)
else()
    message(STATUS "Found SFML ${SFML_VERSION} via find_package")
endif()

# --- 2. Resolve spdlog ---
find_package(spdlog QUIET)

if(NOT spdlog_FOUND)
    message(STATUS "spdlog not found via find_package; falling back to FetchContent (v1.15.3)")
    include(FetchContent)

    set(SPDLOG_FMT_EXTERNAL  OFF CACHE BOOL "" FORCE)
    set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
    set(SPDLOG_BUILD_TESTS   OFF CACHE BOOL "" FORCE)
    set(SPDLOG_INSTALL       OFF CACHE BOOL "" FORCE)
    set(SPDLOG_BUILD_SHARED  OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG        v1.15.3
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(spdlog)
else()
    message(STATUS "Found spdlog ${spdlog_VERSION} via find_package")
endif()
