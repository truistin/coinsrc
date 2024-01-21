set(Bianapi_SRCS
	BianStruct.h
	BianApi.cc
	BianApi.h
	BnApi.cc
	BnApi.h
  )

add_library(Bianapi ${Bianapi_SRCS})
target_link_libraries(Bianapi spot_base)
target_link_libraries(Bianapi spot_utility)
target_link_libraries(Bianapi restful)
target_link_libraries(Bianapi curl)

if(UNIX)
target_link_libraries(Bianapi crypto)
set_target_properties(Bianapi PROPERTIES COMPILE_FLAGS "-std=c++17")
endif(UNIX)
