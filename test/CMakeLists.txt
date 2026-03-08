set(LIB_SRC  ${CMAKE_SOURCE_DIR}/rfc-lib.c)
set(INC_DIRS ${CMAKE_SOURCE_DIR})

# Link math library on platforms that keep it separate (Linux)
find_library(M_LIB m)
if(NOT M_LIB)
    set(M_LIB "")
endif()

function(add_unity_test name)
    add_executable(${name} ${name}.c ${LIB_SRC})
    target_include_directories(${name} PRIVATE ${INC_DIRS})
    target_link_libraries(${name} PRIVATE unity ${M_LIB} NCrystal::NCrystal)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

add_unity_test(test_annulus)
add_unity_test(test_filter_collimator)
