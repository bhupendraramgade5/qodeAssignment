cmake_minimum_required(VERSION 3.20)

project(MarketDataSystem
    VERSION 1.0
    LANGUAGES CXX
)

# --------------------------------------------------
# Global C++ settings
# --------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# --------------------------------------------------
# Dependencies
# --------------------------------------------------
include(FetchContent)

# ---------------- Boost (ONLY for simulator) ----------------
set(BOOST_ENABLE_CMAKE ON)
set(BOOST_INCLUDE_LIBRARIES filesystem program_options)

FetchContent_Declare(
    boost
    URL https://github.com/boostorg/boost/releases/download/boost-1.84.0/boost-1.84.0.tar.gz
)

FetchContent_MakeAvailable(boost)

# --------------------------------------------------
# Source collection
# --------------------------------------------------

# ---- Exchange Simulator sources ----
file(GLOB_RECURSE SIMULATOR_SOURCES
    CONFIGURE_DEPENDS
    src/server/*.cpp
    src/server/*.cxx
    src/server/*.cc
    include/*.cpp
)

# ---- Feed Handler sources ----
file(GLOB_RECURSE FEED_HANDLER_SOURCES
    CONFIGURE_DEPENDS
    src/client/*.cpp
)

# --------------------------------------------------
# Exchange Simulator target
# --------------------------------------------------
add_executable(exchange_simulator
    ${SIMULATOR_SOURCES}
)

target_include_directories(exchange_simulator
    PRIVATE
        src/server
        src/common
        include
)

target_link_libraries(exchange_simulator
    PRIVATE
        Boost::filesystem
        Boost::program_options
        pthread
)

# --------------------------------------------------
# Feed Handler target (NO Boost)
# --------------------------------------------------
add_executable(feed_handler
    ${FEED_HANDLER_SOURCES}
)

target_include_directories(feed_handler
    PRIVATE
        src/client
        src/common
)

target_link_libraries(feed_handler
    PRIVATE
        pthread
)

# --------------------------------------------------
# Install (optional but professional)
# --------------------------------------------------
install(TARGETS exchange_simulator feed_handler
        RUNTIME DESTINATION bin)
