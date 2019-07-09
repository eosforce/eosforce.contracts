add_contract(eosio.lock eosio.lock 
   ${CMAKE_CURRENT_SOURCE_DIR}/src/eosio.lock.cpp)

target_include_directories(eosio.lock PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_target_properties(eosio.lock PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/eosio.lock.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/eosio.lock.contracts.md @ONLY )
target_compile_options( eosio.token PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
