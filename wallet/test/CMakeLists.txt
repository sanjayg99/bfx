include_directories(../..)

add_executable(bfx-tests
    accounting_tests.cpp
    allocator_tests.cpp
    base32_tests.cpp
    base58_tests.cpp
    base64_tests.cpp
    bignum_tests.cpp
    bloom_tests.cpp
    canonical_tests.cpp
    compress_tests.cpp
    crypter_tests.cpp
    db_tests.cpp
    getarg_tests.cpp
    hash_tests.cpp
    key_tests.cpp
    mruset_tests.cpp
    netbase_tests.cpp
    bfxt_tests.cpp
    pmt_tests.cpp
    rpc_tests.cpp
    script_tests.cpp
    serialize_tests.cpp
    sigopcount_tests.cpp
    transaction_tests.cpp
    uint160_tests.cpp
    uint256_tests.cpp
    util_tests.cpp
    wallet_tests.cpp
    ${GTEST_PATH}/src/gtest_main.cc
    # sources that depend on target as they have defs inside them, these are not tests
    ${CMAKE_SOURCE_DIR}/wallet/wallet.cpp
    ${CMAKE_SOURCE_DIR}/wallet/init.cpp
    )

target_link_libraries(bfx-tests
    gtest
    core_lib
    zerocoin_lib
    bfxt_lib
    curltools_lib
    json_spirit_lib
    txdb_lib
    -lpthread
    -lrt
    -ldl
    Boost::system
    Boost::filesystem
    Boost::thread
    Boost::regex
    Boost::program_options
    Boost::iostreams
    Boost::atomic
    ${BERKELEY_DB_LIBRARIES}
    ${CURL_LIBS}
    ${OPENSSL_LIBS}
    ${ZLIB_LIBRARIES}
    )

set_target_properties(bfx-tests PROPERTIES COMPILE_FLAGS -Wno-unused-function)

target_compile_definitions(bfx-tests PRIVATE
    TEST_ROOT_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}\"
    BFX_UNITTESTS
    )

option(BFX_DOWNLOAD_AND_TEST_ALL_TXS "[Unit-tests]: Enable downloading and testing all transactions" ON)
option(BFX_DOWNLOAD_TX_DATA "[Unit-tests]: Enable downloading transaction data" OFF)
option(BFX_RUN_NTP_PARSE_TESTS "[Unit-tests]: Enable BFXT parsing tests" OFF)

if(BFX_DOWNLOAD_AND_TEST_ALL_TXS)
    target_compile_definitions(bfx-tests PRIVATE UNITTEST_DOWNLOAD_PREMADE_TX_DATA_AND_RUN_PARSE_TESTS)
endif()

if(BFX_DOWNLOAD_TX_DATA)
    target_compile_definitions(bfx-tests PRIVATE UNITTEST_DOWNLOAD_TX_DATA)
endif()

if(BFX_RUN_NTP_PARSE_TESTS)
    target_compile_definitions(bfx-tests PRIVATE UNITTEST_RUN_NTP_PARSE_TESTS)
endif()

add_test(
    NAME bfx-core-tests
    COMMAND bfx-core-tests
    --datadir ${CMAKE_CURRENT_SOURCE_DIR}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
