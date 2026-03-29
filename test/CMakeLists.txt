set(RFC_LIB_SRC ${CMAKE_SOURCE_DIR}/rfc-lib.c)
set(FGM_LIB_SRC ${CMAKE_SOURCE_DIR}/filter-mem-lib.c)
set(INC_DIRS ${CMAKE_SOURCE_DIR})

# Link math library on platforms that keep it separate (Linux)
find_library(M_LIB m)
if(NOT M_LIB)
    set(M_LIB "")
endif()

function(add_unity_test name)
    add_executable(${name} ${name}.c ${RFC_LIB_SRC})
    target_include_directories(${name} PRIVATE ${INC_DIRS})
    target_link_libraries(${name} PRIVATE unity ${M_LIB} NCrystal::NCrystal)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

# Tests for filter-mem-lib force-include fgm-stubs.h so that McCode runtime helpers
# (UNSET, all_unset, all_set, …) are defined for both the test source and
# filter-mem-lib.c without modifying either.
function(add_fgm_unity_test name)
    add_executable(${name} ${name}.c ${FGM_LIB_SRC})
    target_include_directories(${name} PRIVATE ${INC_DIRS} ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_options(${name} PRIVATE
        -include ${CMAKE_CURRENT_SOURCE_DIR}/fgm-stubs.h)
    target_link_libraries(${name} PRIVATE unity ${M_LIB} NCrystal::NCrystal)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

add_unity_test(test_annulus)
add_unity_test(test_filter_collimator)
add_fgm_unity_test(test_fgm)
