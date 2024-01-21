set(trading_SRCS
 	trading.cc
  )

if(UNIX)
add_definitions(-D__STDC_FORMAT_MACROS)
endif(UNIX)

file(GLOB HEADERS "*.h")
install(FILES ${HEADERS} DESTINATION include/spot/trading)

add_executable(spot ${trading_SRCS})
if(UNIX)
	target_link_libraries(spot pthread rt z)
	target_link_libraries(spot boost_program_options)
	target_link_libraries(spot curl)
	target_link_libraries(spot uuid)
	set_target_properties(spot PROPERTIES COMPILE_FLAGS "-std=c++17")
else(UNIX)
	target_link_libraries(spot curl)
endif(UNIX)
target_link_libraries(spot spot_base)

target_link_libraries(spot spot_gateway)
target_link_libraries(spot spot_strategy)
target_link_libraries(spot spot_comm)
target_link_libraries(spot spot_utility)
if(CRYPTO_Mode STREQUAL "ON")
	target_link_libraries(spot websocket)
    target_link_libraries(spot restful)
endif()
