/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eosio.pledge/eosio.pledge.hpp>

#include <../../eosio.system/include/eosio.system.hpp>

namespace eosio {
   void pledge::addtype( const name& pledge_name,
                        const account_name& deduction_account,
                        const account_name& ram_payer,
                        const asset& quantity,
                        const string& memo ) {
      require_auth( name{ram_payer} );
      pledgetypes pt_tbl(get_self(),get_self().value);
      auto old_type = pt_tbl.find(pledge_name.value);
      check(old_type == pt_tbl.end(),"the pledge type has exist");
      pt_tbl.emplace( name{ram_payer}, [&]( pledge_type& s ) {
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
      // todo 
      transfer_action temp{ eosio_account, { {name{pledger}, active_permission} } };
      temp.send(  pledger, pledge_account.value, quantity, std::string("add pledge") );

   }

   void pledge::deduction( const name& pledge_name,
                                 const account_name& debitee,
                                 const asset& quantity,
                                 const string& memo ){
      pledgetypes pt_tbl(get_self(),get_self().value);
      auto type = pt_tbl.find(pledge_name.value);
      check(type != pt_tbl.end(),"the pledge type do not exist");
      check(quantity.symbol.code() == type->pledge.symbol.code(),"the symbol do not match");
      require_auth( name{type->deduction_account} );

      pledges ple_tbl(get_self(),debitee);
      auto pledge = ple_tbl.find(pledge_name.value);
      if (pledge == ple_tbl.end()) {
         ple_tbl.emplace( name{debitee}, [&]( pledge_info& b ) { 
            b.pledge_name = pledge_name;
            b.pledge = asset(0,quantity.symbol) - quantity;
            b.deduction = quantity;
         });
      }
      else {
         ple_tbl.modify( pledge, name{}, [&]( pledge_info& b ) { 
            b.pledge -= quantity;
            b.deduction += quantity;
         });
      }
   }

   void pledge::withdraw( const name& pledge_name,
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
      check(pledge != ple_tbl.end(),"the pledge do not exist");
      check(quantity < pledge->pledge,"the quantity is biger then the pledge");

      ple_tbl.modify( pledge, name{}, [&]( pledge_info& b ) { 
            b.pledge -= quantity;
         });

      // todo
      transfer_action temp{ eosio_account, {  {pledge_account, active_permission} } };
      temp.send(  pledge_account.value, pledger, quantity, std::string("withdraw pledge") );
   }

   void pledge::getreward( const account_name& rewarder,
                                 const asset& quantity,
                                 const string& memo ){
      require_auth( name{rewarder} );
      rewards rew_tbl(get_self(),rewarder);
      auto reward_inf = rew_tbl.find(quantity.symbol.code().raw());
      check(reward_inf != rew_tbl.end(),"the reward do not exist");
      check(reward_inf->reward.amount > 0,"the reward do not enough to get");

      rew_tbl.modify( reward_inf, name{}, [&]( reward_info& b ) {
         b.reward -= b.reward;
      });
      // todo
      transfer_action temp{ eosio_account, {  {pledge_account, active_permission} } };
      temp.send(  pledge_account.value, rewarder, quantity, std::string("get reward") );

   }

   void pledge::allotreward(const name& pledge_name,
                                 const account_name& pledger, 
                                 const account_name& rewarder,
                                 const asset& quantity,
                                 const string& memo ) {
      pledgetypes pt_tbl(get_self(),get_self().value);
      auto type = pt_tbl.find(pledge_name.value);
      check(type != pt_tbl.end(),"the pledge type do not exist");
      check(quantity.symbol.code() == type->pledge.symbol.code(),"the symbol do not match");
      require_auth( name{type->deduction_account} );

      pledges ple_tbl(get_self(),pledger);
      auto pledge = ple_tbl.find(pledge_name.value);
      check(pledge != ple_tbl.end(),"the pledge do not exist");
      auto pre_allot = pledge->deduction;
      if (pledge->pledge < asset(0,pledge->pledge.symbol)) {
         pre_allot += pledge->pledge;
      }
      check(quantity < pre_allot,"the quantity is biger then the deduction");
      ple_tbl.modify( pledge, name{}, [&]( pledge_info& b ) { 
            b.deduction -= quantity;
         });
      rewards rew_tbl(get_self(),rewarder);
      auto reward_inf = rew_tbl.find(quantity.symbol.code().raw());
      if (reward_inf == rew_tbl.end()) {
         rew_tbl.emplace( name{type->deduction_account}, [&]( reward_info& b ) { 
            b.reward = quantity;
         });
      }
      else {
         rew_tbl.modify( reward_inf, name{}, [&]( reward_info& b ) {
            b.reward += quantity;
         });
      }
   }

} /// namespace eosio
