add_contract(eosio.pledge eosio.pledge 
   ${CMAKE_CURRENT_SOURCE_DIR}/src/eosio.pledge.cpp)

target_include_directories(eosio.pledge PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_target_properties(eosio.pledge PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/eosio.pledge.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/eosio.pledge.contracts.md @ONLY )
target_compile_options( eosio.token PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
