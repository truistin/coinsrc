set(HEADERS
	RabbitMqTest.h
	RabbitMqTest.cc
#	AsyncLogging_test.cc
  )

add_executable(rabbitmq_test ${HEADERS})
if(UNIX)
set_target_properties(rabbitmq_test PROPERTIES COMPILE_FLAGS "-std=c++17")
endif(UNIX)
target_link_libraries(rabbitmq_test spot_comm)

#add_executable(asynclogging_test AsyncLogging_test.cc)
#target_link_libraries(asynclogging_test spot_utility)
#if(UNIX)
#target_link_libraries(asynclogging_test pthread rt)
#set_target_properties(asynclogging_test PROPERTIES COMPILE_FLAGS "-std=c++17")
#endif(UNIX)







