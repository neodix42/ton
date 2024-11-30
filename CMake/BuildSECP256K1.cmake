if (NOT SECP256K1_LIBRARY)

    set(SECP256K1_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third-party/secp256k1)
    set(SECP256K1_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/third-party/secp256k1)
    set(SECP256K1_INCLUDE_DIR ${SECP256K1_BINARY_DIR}/include)

    file(MAKE_DIRECTORY ${SECP256K1_BINARY_DIR})

    if (EMSCRIPTEN)
      set(SECP256K1_BINARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third-party/secp256k1)
      set(SECP256K1_LIBRARY ${SECP256K1_BINARY_DIR}/.libs/libsecp256k1.a)
      set(SECP256K1_INCLUDE_DIR ${SECP256K1_SOURCE_DIR}/include)
      add_custom_command(
          WORKING_DIRECTORY ${SECP256K1_SOURCE_DIR}
          COMMAND ./autogen.sh
          COMMAND emconfigure ./configure --enable-module-recovery --enable-module-extrakeys --disable-tests --disable-benchmark
          COMMAND emmake make clean
          COMMAND emmake make
          COMMENT "Build Secp256k1 with emscripten"
          DEPENDS ${SECP256K1_SOURCE_DIR}
          OUTPUT ${SECP256K1_LIBRARY}
      )
    elseif (MINGW)
        set(SECP256K1_LIBRARY ${SECP256K1_BINARY_DIR}/lib/libsecp256k1.a)
        add_custom_command(
            WORKING_DIRECTORY ${SECP256K1_SOURCE_DIR}
            COMMAND bash autogen.sh
            COMMAND bash configure -q --disable-option-checking --enable-module-recovery --enable-module-extrakeys --prefix ${SECP256K1_BINARY_DIR} --with-pic --disable-shared --enable-static --disable-tests --disable-benchmark
            COMMAND make -j16
            COMMAND make install
            COMMENT "Build secp256k1"
            DEPENDS ${SECP256K1_SOURCE_DIR}
            OUTPUT ${SECP256K1_LIBRARY}
        )
    else()
      if (NOT NIX)
        set(SECP256K1_LIBRARY ${SECP256K1_BINARY_DIR}/lib/libsecp256k1.a)
        add_custom_command(
            WORKING_DIRECTORY ${SECP256K1_SOURCE_DIR}
            COMMAND ./autogen.sh
            COMMAND ./configure -q --disable-option-checking --enable-module-recovery --enable-module-extrakeys --prefix ${SECP256K1_BINARY_DIR} --with-pic --disable-shared --enable-static --disable-tests --disable-benchmark
            COMMAND make -j16
            COMMAND make install
            COMMENT "Build secp256k1"
            DEPENDS ${SECP256K1_SOURCE_DIR}
            OUTPUT ${SECP256K1_LIBRARY}
        )
      endif()
    endif()
else()
   message(STATUS "Use Secp256k1: ${SECP256K1_LIBRARY}")
endif()

add_custom_target(secp256k1 DEPENDS ${SECP256K1_LIBRARY})
