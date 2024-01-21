if(CRYPTO_Mode STREQUAL "ON")
set(cryptoUtility_SRCS
	algo_hmac.cc
	algo_hmac.h
  )
else()
set(cryptoUtility_SRCS
  )
endif()


set(utility_SRCS
	CountDownLatch.cc
	CountDownLatch.h
	Date.cc
	Date.h
	FileUtil.cc
	FileUtil.h
	LogFile.cc
	LogFile.h
	Logging.cc
	Logging.h
	LogStream.cc
	LogStream.h
	Timestamp.cc
	Timestamp.h
	Thread.cc
	Thread.h
	Measure.cc
	Measure.h
	ReadIni.cc
	ReadIni.h
	SPSCQueue.h
	Utility.h
	Types.h
	Compatible.h
	concurrentqueue.h
	TradingTime.cc
	TradingTime.h
	TradingDay.cc
	TradingDay.h
	FileWriter.cc
	FileWriter.h
	BlockingQueue.h
	gzipwrapper.h
	okzlibwrapper.h
	LoggerEx.h
	LoggerEx.cc
	UrlHelper.h
	tscns.h
	MeasureFunc.cc
	MeasureFunc.h
	# curve_fit.h
	# curve_fit.cpp
  )

set (depend_SRCS
	boost_date_time.so
	boost_filesystem.so
	boost_iostreams.so
	boost_program_options.so
	boost_system.so
	crypto
	ssl
)

add_library(spot_utility SHARED ${utility_SRCS} ${cryptoUtility_SRCS})
if(UNIX)
target_link_libraries(spot_utility pthread rt)
target_link_libraries(spot_utility ${depend_SRCS})
# target_link_libraries(spot_utility gsl)
# target_link_libraries(spot_utility gslcblas)
set_target_properties(spot_utility PROPERTIES COMPILE_FLAGS "-std=c++17")
endif(UNIX)


file(GLOB HEADERS "*.h")
install(FILES ${HEADERS} DESTINATION include/spot/utility)

#add_subdirectory(tests)
