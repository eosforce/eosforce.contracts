add_contract(eosio.freeze eosio.freeze 
   ${CMAKE_CURRENT_SOURCE_DIR}/src/eosio.freeze.cpp)

target_include_directories(eosio.freeze PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_target_properties(eosio.freeze PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/eosio.freeze.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/eosio.freeze.contracts.md @ONLY )
target_compile_options( eosio.token PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
