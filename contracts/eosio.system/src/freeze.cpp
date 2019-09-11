#include <utility>

#include <eosio.system.hpp>
#include <eosio.freeze/eosio.freeze.hpp>

namespace eosio {

   static constexpr name freeze_account = "eosio.freeze"_n;

   bool system_contract::is_account_freezed( const account_name& account, const uint32_t curr_block_num ) {
      global_freezed_state gfstate( freeze_account, freeze_account.value );

      const auto act_table = gfstate.exists() ? gfstate.get().committer : 0;

      if( act_table == 0 ){
         return false;
      }

      freezeds freeze_tbl( freeze_account, act_table );
      const auto itr = freeze_tbl.find( account );
      return ( itr != freeze_tbl.end() ) 
          && ( itr->unfreeze_block_num <= 1024 || curr_block_num <= itr->unfreeze_block_num );
   }

} /// namespace eosio
