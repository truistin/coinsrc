set(public_SRCS
  MktDataStoreCallBack.h
  OrderQueStore.h
  OrderQueStore.cc
  MktDataQueStore.h
  MktDataQueStore.cc
  GateWay.h
  GateWay.cc
  OrderStoreCallBack.h
  MktDataLogStore.cc
  MktDataLogStore.h
  ShmMdSpi.cc
  ShmMdSpi.h
  ShmMtSpi.cc
  ShmMtSpi.h
  DistMdSpi.cc
  DistMdSpi.h
  )

# set(I_DistMd_MD_SRCS
# 	DistMdSpi.cc
# 	DistMdSpi.h
# )

# set(I_ShmMd_MD_SRCS
# 	ShmMdSpi.cc
# 	ShmMdSpi.h
# )

# set(I_ShmMt_MD_SRCS
# 	ShmMtSpi.cc
# 	ShmMtSpi.h
# )

if (DEFINED I_Bybit_TD)
	set(I_Bybit_TD_SRCS
	  BybitTdSpi.cc
	  BybitTdSpi.h
	)
endif()
if (DEFINED I_Bybit_MD)
	set(I_Bybit_MD_SRCS
	  BybitMdSpi.cc
	  BybitMdSpi.h
	)
endif()

if (DEFINED I_OKExV5_TD)
	set(I_OKExV5_TD_SRCS
	  OKxV5TdSpi.cc
	  OKxV5TdSpi.h
	)
endif()
if (DEFINED I_OKExV5_MD)
	set(I_OKExV5_MD_SRCS
	  OKxV5MdSpi.cc
	  OKxV5MdSpi.h
	)
endif()

if (DEFINED I_Huob_TD)
	set(I_Huob_TD_SRCS
	  HuobTraderSpi.cc
	  HuobTraderSpi.h
	)
endif()
if (DEFINED I_Huob_MD)
	set(I_Huob_MD_SRCS
	  HuobMdSpi.cc
	  HuobMdSpi.h
	)
endif()

if (DEFINED I_HuobFuture_TD)
	set(I_HuobFuture_TD_SRCS
	  HuobTraderSpi.cc
	  HuobTraderSpi.h
	)
endif()
if (DEFINED I_HuobFuture_MD)
	set(I_HuobFuture_MD_SRCS
	  HuobFutureMdSpi.cc
	  HuobFutureMdSpi.h
	)
endif()

if (DEFINED I_Bian_TD)
  set(I_Bian_TD_SRCS
  BianTdSpi.cc
  BianTdSpi.h
  )
endif()
if (DEFINED I_Bian_MD)
  set(I_Bian_MD_SRCS
  BianMdSpi.cc
  BianMdSpi.h
  )
endif()

set(gateway_SRCS
    ${public_SRCS}
	${I_Bybit_MD_SRCS}
	${I_Bybit_TD_SRCS}
    ${I_OKExV5_TD_SRCS}
    ${I_OKExV5_MD_SRCS}
    ${I_Huob_TD_SRCS}
    ${I_Huob_MD_SRCS}
    ${I_HuobFuture_TD_SRCS}
    ${I_HuobFuture_MD_SRCS}
    ${I_Bian_TD_SRCS}
    ${I_Bian_MD_SRCS}
  )

add_library(spot_gateway ${gateway_SRCS})

if(UNIX)
set_target_properties(spot_gateway PROPERTIES COMPILE_FLAGS "-std=c++17")
endif(UNIX)

add_definitions(-DISLIB)
target_link_libraries(spot_gateway spot_base)
target_link_libraries(spot_gateway spot_utility)
target_link_libraries(spot_gateway spot_net)
target_link_libraries(spot_gateway spot_cache)

target_link_libraries(spot_gateway spot_dist)
target_link_libraries(spot_gateway spot_shmmd)

if (DEFINED I_Bybit_MD)
	target_link_libraries(spot_gateway Bybitapi)
	target_link_libraries(spot_gateway curl)
	target_link_libraries(spot_gateway websocket)
endif()
if (DEFINED I_Bybit_TD)
	target_link_libraries(spot_gateway Bybitapi)
	target_link_libraries(spot_gateway curl)
	target_link_libraries(spot_gateway websocket)
endif()

if (DEFINED I_OKExV5_TD)
	target_link_libraries(spot_gateway Okexapi)
	target_link_libraries(spot_gateway curl)
	target_link_libraries(spot_gateway websocket)
endif()
if (DEFINED I_OKExV5_MD)
	target_link_libraries(spot_gateway Okexapi)
	target_link_libraries(spot_gateway curl)
	target_link_libraries(spot_gateway websocket)
endif()

if (DEFINED I_Huob_TD)
	target_link_libraries(spot_gateway huobapi)
	target_link_libraries(spot_gateway curl)
	target_link_libraries(spot_gateway websocket)
endif()
if (DEFINED I_Huob_MD)
	target_link_libraries(spot_gateway huobapi)
	target_link_libraries(spot_gateway curl)
	target_link_libraries(spot_gateway websocket)
endif()

if (DEFINED I_HuobFuture_TD)
	target_link_libraries(spot_gateway huobapi)
	target_link_libraries(spot_gateway curl)
	target_link_libraries(spot_gateway websocket)
endif()
if (DEFINED I_HuobFuture_MD)
	target_link_libraries(spot_gateway huobapi)
	target_link_libraries(spot_gateway curl)
	target_link_libraries(spot_gateway websocket)
endif()

if (DEFINED I_Bian_TD)
	target_link_libraries(spot_gateway Bianapi)
	target_link_libraries(spot_gateway curl)
	target_link_libraries(spot_gateway websocket)
endif()
if (DEFINED I_Bian_MD)
	target_link_libraries(spot_gateway Bianapi)
	target_link_libraries(spot_gateway curl)
	target_link_libraries(spot_gateway websocket)
endif()

file(GLOB HEADERS "*.h")
install(FILES ${HEADERS} DESTINATION include/spot/gateway)