set(Okexapi_SRCS
	OkSwapStruct.h
	OkSwapApi.cpp
	OkSwapApi.h
  )

add_library(Okexapi ${Okexapi_SRCS})
target_link_libraries(Okexapi spot_base)
target_link_libraries(Okexapi spot_gateway)
target_link_libraries(Okexapi spot_utility)
target_link_libraries(Okexapi restful)
target_link_libraries(Okexapi curl)

if(UNIX)
target_link_libraries(Okexapi crypto)
set_target_properties(Okexapi PROPERTIES COMPILE_FLAGS "-std=c++17")
endif(UNIX)
