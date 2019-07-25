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

   void pledge::addpledge( const account_name& from,
                           const account_name& to,
                           const asset& quantity,
                           const string& memo ){
      if (to != get_self().value || memo.length() == 0) return ;
      require_auth( name{from} );
      pledgetypes pt_tbl(get_self(),get_self().value);
      auto pledge_name = name{memo};
      auto type = pt_tbl.find(pledge_name.value);
      check(type != pt_tbl.end(),"the pledge type do not exist");
      check(quantity.symbol.code() == type->pledge.symbol.code(),"the symbol do not match");

      pledges ple_tbl(get_self(),from);
      auto pledge = ple_tbl.find(pledge_name.value);
      if (pledge == ple_tbl.end()) {
         ple_tbl.emplace( name{from}, [&]( pledge_info& b ) { 
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
      pledgetypes pt_tbl(get_self(),get_self().value);
      auto type = pt_tbl.find(pledge_name.value);
      check(type != pt_tbl.end(),"the pledge type do not exist");
      check(quantity.symbol.code() == type->pledge.symbol.code(),"the symbol do not match");
      check(0 < quantity.amount,"the quantity should be a positive number");
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
      check(asset(0,pledge->pledge.symbol) < quantity,"the quantity must be a positive number");

      ple_tbl.modify( pledge, name{}, [&]( pledge_info& b ) { 
         b.pledge -= quantity;
      });

      transfer_action temp{ eosio_account, {  {get_self(), active_permission} } };
      temp.send(  get_self().value, pledger, quantity, std::string("withdraw pledge") );
   }

   void pledge::getreward( const account_name& rewarder,
                           const asset& quantity,
                           const string& memo ){
      require_auth( name{rewarder} );
      rewards rew_tbl(get_self(),rewarder);
      auto reward_inf = rew_tbl.find(quantity.symbol.code().raw());
      check(reward_inf != rew_tbl.end(),"the reward do not exist");
      check(reward_inf->reward.amount > 0,"the reward do not enough to get");

      transfer_action temp{ eosio_account, {  {pledge_account, active_permission} } };
      temp.send(  pledge_account.value, rewarder, reward_inf->reward, std::string("get reward") );

      rew_tbl.modify( reward_inf, name{}, [&]( reward_info& b ) {
         b.reward -= b.reward;
      });
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
      check(0 < quantity.amount,"the quantity must be a positive number");
      check(quantity <= pre_allot,"the quantity is biger then the deduction");
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

   void pledge::open(const name& pledge_name,
                     const account_name& payer,
                     const string& memo) {
      require_auth( name{payer} );
      pledgetypes pt_tbl(get_self(),get_self().value);
      auto type = pt_tbl.find(pledge_name.value);
      check(type != pt_tbl.end(),"the pledge type do not exist");

      pledges ple_tbl(get_self(),payer);
      auto pledge = ple_tbl.find(pledge_name.value);
      if (pledge != ple_tbl.end()) {
         return ;
      }
//      check(pledge == ple_tbl.end(),"the pledge record has exist");
      ple_tbl.emplace( name{payer}, [&]( pledge_info& b ) { 
         b.pledge_name = pledge_name;
         b.pledge = asset(0,type->pledge.symbol);
         b.deduction = asset(0,type->pledge.symbol);
      });

   }
   void pledge::close(const name& pledge_name,
                     const account_name& payer,
                     const string& memo) {
      require_auth( name{payer} );
      pledgetypes pt_tbl(get_self(),get_self().value);
      auto type = pt_tbl.find(pledge_name.value);
      check(type != pt_tbl.end(),"the pledge type do not exist");

      pledges ple_tbl(get_self(),payer);
      auto pledge = ple_tbl.find(pledge_name.value);
      check(pledge == ple_tbl.end(),"the pledge record has exist");
      check(pledge->pledge == asset(0,type->pledge.symbol),"the pledge is not 0,can not be closed");
      check(pledge->deduction == asset(0,type->pledge.symbol),"the deduction is not 0,can not be closed");

      ple_tbl.erase(pledge);
      
   }


} /// namespace eosio
