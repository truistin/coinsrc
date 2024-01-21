set(HEADERS
	SpotMd.cc
	MdInterface.cc
	MdInterface.h
  )

if(WIN32)
LINK_DIRECTORIES(${PROJECT_SOURCE_DIR}/lib/zlib)
endif(WIN32)

set(RABBITMQ_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/lib/librabbitmq)
include_directories(${RABBITMQ_INCLUDE_DIR})
LINK_DIRECTORIES(${RABBITMQ_INCLUDE_DIR})

add_executable(spotmd ${HEADERS})
if(UNIX)
	target_link_libraries(spotmd pthread rt z)
	target_link_libraries(spotmd boost_program_options)
	target_link_libraries(spotmd curl)
	set_target_properties(spotmd PROPERTIES COMPILE_FLAGS "-std=c++17")
	target_link_libraries(spotmd uuid)
else(UNIX)
	target_link_libraries(spotmd curl)	
	target_link_libraries(spotmd zlib)
endif(UNIX)
target_link_libraries(spotmd spot_base)
target_link_libraries(spotmd spot_utility)
target_link_libraries(spotmd spot_gateway)
target_link_libraries(spotmd spot_strategy)
target_link_libraries(spotmd spot_comm)
target_link_libraries(spotmd spot_cache)
target_link_libraries(spotmd spot_dist)
target_link_libraries(spotmd restful)

if(CRYPTO_Mode STREQUAL "ON")
	target_link_libraries(spotmd websocket)
    target_link_libraries(spotmd restful)
endif()
