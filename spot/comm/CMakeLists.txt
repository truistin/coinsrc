set(COMM_SRCS
	MqDispatch.h
	MqDispatch.cc
	MqUtils.h
	MqUtils.cc
	AsyncLogging.cc
	AsyncLogging.h
	RabbitMqProcess.cc
	RabbitMqProcess.h
  )


add_library(spot_comm ${COMM_SRCS})

if(UNIX)
set_target_properties(spot_comm PROPERTIES COMPILE_FLAGS "-std=c++17")
endif(UNIX)

set(RABBITMQ_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/lib/librabbitmq)
include_directories(${RABBITMQ_INCLUDE_DIR})
LINK_DIRECTORIES(${RABBITMQ_INCLUDE_DIR})

target_link_libraries(spot_comm spot_base spot_utility )
# target_link_libraries(spot_comm rabbitmq.4 )
target_link_libraries(spot_comm rabbitmq)

#add_subdirectory(tests)





