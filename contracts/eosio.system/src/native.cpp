#include <utility>

#include <eosio.system.hpp>

namespace eosio {
   void native::onerror( ignore<uint128_t>, ignore<std::vector<char>> ) {
      check( false, "the onerror action cannot be called directly" );
   }
} /// namespace eosio
