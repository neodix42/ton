if (NOT OPENSSL_FOUND)
  find_package(OpenSSL REQUIRED)
endif()

add_executable(generate-random-id generate-random-id.cpp )
target_link_libraries(generate-random-id tl_api ton_crypto keys adnl git)

target_include_directories(generate-random-id PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/..)

add_executable(json2tlo json2tlo.cpp )
target_link_libraries(json2tlo tl_api ton_crypto keys git)
target_include_directories(json2tlo PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/..)

add_executable(pack-viewer pack-viewer.cpp )
target_link_libraries(pack-viewer tl_api ton_crypto keys validator tddb)
target_include_directories(pack-viewer PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/..)

add_executable(opcode-timing opcode-timing.cpp )
target_link_libraries(opcode-timing ton_crypto)
target_include_directories(pack-viewer PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/..)

add_executable(proxy-liteserver proxy-liteserver.cpp)
target_link_libraries(proxy-liteserver tdutils tdactor adnl dht tl_api ton_crypto git lite-client-common)

add_executable(prepare-ls-slice-config prepare-ls-slice-config.cpp)
target_link_libraries(prepare-ls-slice-config tdutils tdactor adnl dht tl_api ton_crypto git lite-client-common)

install(TARGETS generate-random-id proxy-liteserver RUNTIME DESTINATION bin)
