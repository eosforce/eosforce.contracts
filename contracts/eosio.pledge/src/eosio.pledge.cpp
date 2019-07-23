/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eosio.pledge/eosio.pledge.hpp>

namespace eosio {
   void pledge::addtype( const name& pledge_name,
                        const account_name& deduction_account,
                        const asset& quantity,
                        const string& memo ) {
      require_auth( eosio_account );
      pledgetypes pt_tbl(get_self(),get_self().value);
      auto old_type = pt_tbl.find(pledge_name.value);
      check(old_type == pt_tbl.end(),"the pledge type has exist");
      pt_tbl.emplace( eosio_account, [&]( pledge_type& s ) {
         s.pledge_name = pledge_name;
         s.deduction_account = deduction_account;
         s.pledge = quantity;
      });
   }

   void pledge::addpledge( const name& pledge_name,
                                 const account_name& pledger,
                                 const asset& quantity,
                                 const string& memo ){
      require_auth( name{pledger} );
      pledgetypes pt_tbl(get_self(),get_self().value);
      auto type = pt_tbl.find(pledge_name.value);
      check(type != pt_tbl.end(),"the pledge type do not exist");
      check(quantity.symbol.code() == type->pledge.symbol.code(),"the symbol do not match");

      pledges ple_tbl(get_self(),pledger);
      auto pledge = ple_tbl.find(pledge_name.value);
      if (pledge == ple_tbl.end()) {
         ple_tbl.emplace( name{pledger}, [&]( pledge_info& b ) { 
            b.pledge_name = pledge_name;
            b.pledge = quantity;
            b.deduction = asset(0,quantity.symbol);
         });
      }
      else {
         ple_tbl.modify( pledge, name{}, [&]( pledge_info& b ) { 
            b.pledge += quantity;
         });
      }
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
