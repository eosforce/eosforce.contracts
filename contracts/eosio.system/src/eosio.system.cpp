#include <utility>

#include <eosio.system.hpp>

namespace eosio {

   void system_contract::transfer( const name& from, 
                                   const name& to, 
                                   const asset& quantity,
                                   const string& memo ) {
      require_auth( from );
      accounts_table acnts_tbl( _self, _self.value );
      const auto& from_act = acnts_tbl.get( from.value, "from account is not found in accounts table" );
      const auto& to_act = acnts_tbl.get( to.value, "to account is not found in accounts table" );

      check( quantity.symbol == CORE_SYMBOL, "only support EOS which has 4 precision" );
      // from_act.available is already handling fee
      check( 0 <= quantity.amount && quantity.amount <= from_act.available.amount,
             "need 0.0000 EOS < quantity < available balance" );
      check( memo.size() <= 256, "memo has more than 256 bytes" );

      require_recipient( from );
      require_recipient( to );

      acnts_tbl.modify( from_act, name{}, [&]( account_info& a ) { a.available -= quantity; } );
      acnts_tbl.modify( to_act, name{}, [&]( account_info& a ) { a.available += quantity; } );
   }

} /// namespace eosio
