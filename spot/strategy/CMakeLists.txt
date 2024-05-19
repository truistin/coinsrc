set(strategy_SRCS
	StrategyCooker.cc
	StrategyCooker.h
	Strategy.cc
	Strategy.h
	StrategyCircuit.cc
	StrategyCircuit.h
	StrategyFactory.cc
	StrategyFactory.h
	Initializer.cc
	Initializer.h
	InstrumentOrder.cc
	InstrumentOrder.h
	OrderController.cc
	OrderController.h
	MarketDataManager.cc
	MarketDataManager.h
	StrategyInstrument.cc
	StrategyInstrument.h
	RmqWriter.cc
	RmqWriter.h
	ConfigParameter.cc
	ConfigParameter.h
	DealOrders.cc
	DealOrders.h
	DealOrdersApi.cc
	DealOrdersApi.h
	StrategyTimer.cc
	StrategyTimer.h
	StrategyInstrumentManager.cc
	StrategyInstrumentManager.h
	Position.cc
	Position.h
	# StrategyUCArb.cc
	# StrategyUCArb.h
	# StrategyUCArb1.cc
	# StrategyUCArb1.h
	# StrategyFR.cc
	# StrategyFR.h
	StrategyUCEasy.cc
	StrategyUCEasy.h
  )

add_library(spot_strategy ${strategy_SRCS})

if(UNIX)
set_target_properties(spot_strategy PROPERTIES COMPILE_FLAGS "-std=c++17")
endif(UNIX)

file(GLOB HEADERS "*.h")
install(FILES ${HEADERS} DESTINATION include/spot/strategy)

# target_link_libraries(spot_strategy ssl)
# target_link_libraries(spot_strategy crypto)
target_link_libraries(spot_strategy spot_base)
target_link_libraries(spot_strategy spot_utility)
target_link_libraries(spot_strategy spot_risk)
target_link_libraries(spot_strategy spot_gateway)
target_link_libraries(spot_strategy spot_cache)
#add_subdirectory(tests)
