#include <utility>

#include <eosio.system.hpp>

namespace eosio {

   void system_contract::transfer( const account_name& from, 
                                   const account_name& to, 
                                   const asset& quantity,
                                   const string& memo ) {
      require_auth( name{from} );
      accounts_table acnts_tbl( _self, _self.value );
      const auto& from_act = acnts_tbl.get( from, "from account is not found in accounts table" );
      const auto& to_act = acnts_tbl.get( to, "to account is not found in accounts table" );

      check( quantity.symbol == CORE_SYMBOL, "only support EOS which has 4 precision" );
      // from_act.available is already handling fee
      check( 0 <= quantity.amount && quantity.amount <= from_act.available.amount,
             "need 0.0000 EOS < quantity < available balance" );
      check( memo.size() <= 256, "memo has more than 256 bytes" );

      require_recipient( name{from} );
      require_recipient( name{to} );

      acnts_tbl.modify( from_act, name{}, [&]( account_info& a ) { 
         a.available -= quantity; 
      } );

      acnts_tbl.modify( to_act, name{}, [&]( account_info& a ) { 
         a.available += quantity; 
      } );
   }

} /// namespace eosio
