cmake_minimum_required( VERSION 3.5 )

project(contracts)

set(EOSIO_WASM_OLD_BEHAVIOR "Off")
find_package(eosforce.cdt)

<<<<<<< HEAD

add_subdirectory(eosio.token)
add_subdirectory(eosio.system)
add_subdirectory(eosio.lock)
=======
add_subdirectory(eosio.token)
add_subdirectory(eosio.lock)
add_subdirectory(eosio.system)
add_subdirectory(eosio.msig)
>>>>>>> upstream/release-v1.1.x
