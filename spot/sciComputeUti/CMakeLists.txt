set(sciCompute_SRCS
	curve_fit.h
	curve_fit.cpp
	PolyfitBoost.hpp
	PolyfitEigen.hpp
	PythonEnv.h
	PythonEnv.cc
	# gslCurve.h
	# gslCurve.cpp
  )

add_library(spot_sciCompute ${sciCompute_SRCS})
if(UNIX)
target_link_libraries(spot_sciCompute pthread rt)
target_link_libraries(spot_sciCompute dl util)
target_link_libraries(spot_sciCompute libpython3.8.a)
target_link_libraries(spot_sciCompute libeigen_blas.so)
target_link_libraries(spot_sciCompute libeigen_lapack.so)
# target_link_libraries(spot_sciCompute gsl)
# target_link_libraries(spot_sciCompute gslcblas)
set_target_properties(spot_sciCompute PROPERTIES COMPILE_FLAGS "-std=c++17")
endif(UNIX)


file(GLOB HEADERS "*.h")
install(FILES ${HEADERS} DESTINATION include/spot/sciComputeUti)

#add_subdirectory(tests)
