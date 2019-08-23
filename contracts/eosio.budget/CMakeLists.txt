add_contract(eosio.budget eosio.budget 
   ${CMAKE_CURRENT_SOURCE_DIR}/src/eosio.budget.cpp)

target_include_directories(eosio.budget PUBLIC 
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../eosio.token/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../include)

set_target_properties(eosio.budget PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/eosio.budget.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/eosio.budget.contracts.md @ONLY )
target_compile_options( eosio.budget PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
