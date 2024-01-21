set(rapidjson_SRCS
	tinyxml2.cpp
  )

add_library(rapidjson ${rapidjson_SRCS})

set(HEADERS
  tinyxml2.h
  )
  
file(GLOB HEADERS "*.h")
install(FILES ${HEADERS} DESTINATION include/spot/rapidjson)
add_subdirectory(tests)




