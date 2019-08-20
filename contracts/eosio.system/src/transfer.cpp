#include <utility>

#include <eosio.system.hpp>

namespace eosio {

   void system_contract::transfer( const account_name& from, 
                                   const account_name& to, 
                                   const asset& quantity,
                                   const string& memo ) {
      require_auth( name{from} );

      const auto& from_act = _accounts.get( from, "from account is not found in accounts table" );
      const auto& to_act   = _accounts.get( to,   "to account is not found in accounts table" );

      check( quantity.symbol == CORE_SYMBOL, "only support EOS which has 4 precision" );
      // from_act.available is already handling fee
      check( 0 <= quantity.amount && quantity.amount <= from_act.available.amount,
             "need 0.0000 EOS < quantity < available balance" );
      check( memo.size() <= 256, "memo has more than 256 bytes" );

      require_recipient( name{from} );
      require_recipient( name{to} );

      _accounts.modify( from_act, name{}, [&]( account_info& a ) { 
         a.available -= quantity; 
      } );

      _accounts.modify( to_act, name{}, [&]( account_info& a ) { 
         a.available += quantity; 
      } );
   }

} /// namespace eosio
