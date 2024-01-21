set(huobapi_SRCS
	HuobStruct.h
	HuobApi.cc
	HuobApi.h
  )

add_library(huobapi ${huobapi_SRCS})
target_link_libraries(huobapi spot_base)
target_link_libraries(huobapi spot_utility)
target_link_libraries(huobapi restful)
target_link_libraries(huobapi curl)
if(UNIX)
target_link_libraries(huobapi crypto)
set_target_properties(huobapi PROPERTIES COMPILE_FLAGS "-std=c++17")
endif(UNIX)