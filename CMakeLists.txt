cmake_minimum_required(VERSION 3.14)
project(rfc-lib C)

execute_process( COMMAND "ncrystal-config" "--show" "cmakedir" OUTPUT_VARIABLE NCrystal_DIR OUTPUT_STRIP_TRAILING_WHITESPACE )
find_package(NCrystal 4.0.0 REQUIRED)

include(FetchContent)
FetchContent_Declare(
    unity
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
    GIT_TAG        v2.6.0
)
FetchContent_MakeAvailable(unity)
target_compile_definitions(unity PUBLIC UNITY_INCLUDE_DOUBLE)

enable_testing()
add_subdirectory(test)
