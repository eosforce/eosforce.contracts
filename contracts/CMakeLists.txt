cmake_minimum_required( VERSION 3.5 )

project(contracts)

set(EOSIO_WASM_OLD_BEHAVIOR "Off")
find_package(eosforce.cdt)

add_subdirectory(eosio.token)
add_subdirectory(eosio.lock)
add_subdirectory(eosio.system)
add_subdirectory(eosio.msig)
add_subdirectory(eosio.pledge)
add_subdirectory(eosio.budget)
add_subdirectory(eosio.freeze)
