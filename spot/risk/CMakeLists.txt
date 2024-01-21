set(risk_SRCS		
	spotRiskBase.h
	spotRisk.h
	spotRisk.cc
  )

add_library(spot_risk ${risk_SRCS})
target_link_libraries(spot_risk spot_utility)
target_link_libraries(spot_risk spot_base)
target_link_libraries(spot_risk spot_cache)


if(UNIX)
set_target_properties(spot_risk PROPERTIES COMPILE_FLAGS "-std=c++11")
endif(UNIX)

file(GLOB HEADERS "*.h")
install(FILES ${HEADERS} DESTINATION include/spot/risk)