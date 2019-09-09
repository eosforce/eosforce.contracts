#include <utility>

#include <eosio.system.hpp>

namespace eosio {

   bool system_contract::is_account_freezed( const account_name& account, const uint32_t curr_block_num ) {
      return false;
   }

} /// namespace eosio
