/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eosio.pledge/eosio.pledge.hpp>

namespace eosio {
   void pledge::addtype( const name& pledge_name,
                        const account_name& deduction_account,
                        const string& memo ) {
      require_auth( eosio_account );
      pledgetypes pt_tbl(get_self(),get_self().value);
      auto old_type = pt_tbl.find(pledge_name.value);
      check(old_type == pt_tbl.end(),"the pledge type has exist");
      pt_tbl.emplace( eosio_account, [&]( pledge_type& s ) {
         s.pledge_name = pledge_name;
         s.deduction_account = deduction_account;
      });
   }

   void pledge::addpledge( const name& pledge_name,
                                 const asset& quantity,
                                 const string& memo ){

   }

   void pledge::deduction( const name& pledge_name,
                                 const account_name& debitee,
                                 const asset& quantity,
                                 const string& memo ){

   }

   void pledge::withdraw( const name& pledge_name,
                                 const account_name& pledger,
                                 const asset& quantity,
                                 const string& memo ){

   }

   void pledge::getreward( const account_name& pledger,
                                 const asset& quantity,
                                 const string& memo ){

   }

} /// namespace eosio
