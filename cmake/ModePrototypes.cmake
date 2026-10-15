# Isolated development targets only. Never install these into a normal host profile.
enable_language(C)
enable_language(Fortran)
set(WSPR_SOURCE "${CMAKE_SOURCE_DIR}/third_party/wsjtx/lib/wsprd")
if(NOT EXISTS "${WSPR_SOURCE}/wsprd.c")
 message(FATAL_ERROR "Stage the pinned WSPR sources with scripts/stage_wspr.py first")
endif()
add_library(dx_wspr_support STATIC
 "${WSPR_SOURCE}/wsprsim_utils.c" "${WSPR_SOURCE}/wsprd_utils.c" "${WSPR_SOURCE}/tab.c"
 "${WSPR_SOURCE}/fano.c" "${WSPR_SOURCE}/jelinek.c" "${WSPR_SOURCE}/nhash.c")
target_include_directories(dx_wspr_support PUBLIC "${WSPR_SOURCE}" /opt/homebrew/include)
target_compile_options(dx_wspr_support PRIVATE -O3)
add_executable(dx_wspr_reference "${WSPR_SOURCE}/wsprd.c" "${WSPR_SOURCE}/osdwspr.f90"
 "${CMAKE_SOURCE_DIR}/third_party/wsjtx/lib/indexx.f90")
target_link_libraries(dx_wspr_reference PRIVATE dx_wspr_support ${FFTW})
set_target_properties(dx_wspr_reference PROPERTIES BUILD_RPATH "${HOST_APP}/Contents/Frameworks")
add_custom_command(TARGET dx_wspr_reference POST_BUILD COMMAND install_name_tool -change
 /opt/homebrew/opt/fftw/lib/libfftw3f.3.dylib @rpath/libfftw3f.3.dylib $<TARGET_FILE:dx_wspr_reference>)
add_executable(dx_wspr_simulator "${WSPR_SOURCE}/wsprsim.c")
target_link_libraries(dx_wspr_simulator PRIVATE dx_wspr_support)
add_test(NAME dx_wspr_reference COMMAND ${Python3_EXECUTABLE} "${CMAKE_SOURCE_DIR}/tests/test_wspr_reference.py"
 $<TARGET_FILE:dx_wspr_reference> $<TARGET_FILE:dx_wspr_simulator>)
