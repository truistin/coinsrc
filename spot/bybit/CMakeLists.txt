set(Bybitapi_SRCS
	BybitStruct.h
	BybitApi.cc
	BybitApi.h
  )

add_library(Bybitapi ${Bybitapi_SRCS})
target_link_libraries(Bybitapi spot_base)
target_link_libraries(Bybitapi spot_utility)
target_link_libraries(Bybitapi restful)
target_link_libraries(Bybitapi curl)

if(UNIX)
target_link_libraries(Bybitapi crypto)
set_target_properties(Bybitapi PROPERTIES COMPILE_FLAGS "-std=c++17")
endif(UNIX)
