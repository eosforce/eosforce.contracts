#include <eosio.token.hpp>

namespace eosio {
   ACTION token::create( account_name issuer,
            asset        maximum_supply) {
      auto sym = maximum_supply.symbol;
      eosio::check( sym.is_valid(), "invalid symbol name" );
      eosio::check( maximum_supply.is_valid(), "invalid supply");
      eosio::check( maximum_supply.amount > 0, "max-supply must be positive");
      eosio::check( sym.code() != CORE_SYMBOL.code(), "not create EOS");

      stats statstable( _self, sym.code().raw() );
      auto existing = statstable.find( sym.code().raw() );
      eosio::check( existing == statstable.end(), "token with symbol already exists" );

      statstable.emplace( _self, [&]( auto& s ) {
         s.supply.symbol = maximum_supply.symbol;
         s.max_supply    = maximum_supply;
         s.issuer        = issuer;
      });
   }

   ACTION token::issue( account_name to, asset quantity, std::string memo ) {
      auto sym = quantity.symbol;
      eosio::check( sym.is_valid(), "invalid symbol name" );
      eosio::check( memo.size() <= 256, "memo has more than 256 bytes" );

      auto sym_name = sym.code();
      stats statstable( _self, sym_name.raw() );
      auto existing = statstable.find( sym_name.raw() );
      eosio::check( existing != statstable.end(), "token with symbol does not exist, create token before issue" );
      const auto& st = *existing;

      require_auth( st.issuer );
      eosio::check( quantity.is_valid(), "invalid quantity" );
      eosio::check( quantity.amount > 0, "must issue positive quantity" );

      eosio::check( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
      eosio::check( quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

      statstable.modify( st, to, [&]( auto& s ) {
         s.supply += quantity;
      });

      add_balance( st.issuer, quantity, st.issuer );

      if( to != st.issuer ) {
         SEND_INLINE_ACTION( *this, transfer, {st.issuer,config::active_permission}, {st.issuer, to, quantity, memo} );
      }
   }

   ACTION token::transfer( account_name from,
                  account_name to,
                  asset        quantity,
                  string       memo ) {
      eosio::check( from != to, "cannot transfer to self" );
      require_auth( from );
      eosio::check( is_account( to ), "to account does not exist");
      auto sym = quantity.symbol.code();
      stats statstable( _self, sym.raw() );
      const auto& st = statstable.get( sym.raw() );

      require_recipient( from );
      require_recipient( to );

      eosio::check( quantity.is_valid(), "invalid quantity" );
      eosio::check( quantity.amount > 0, "must transfer positive quantity" );
      eosio::check( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
      eosio::check( memo.size() <= 256, "memo has more than 256 bytes" );


      sub_balance( from, quantity );
      add_balance( to, quantity, from );
   
   }

   void token::sub_balance( account_name owner, asset value ) {
      accounts from_acnts( _self, owner.value );

      const auto& from = from_acnts.get( value.symbol.code().raw(), "no balance object found" );
      eosio::check( from.balance.amount >= value.amount, "overdrawn balance" );

      if( from.balance.amount == value.amount ) {
         from_acnts.erase( from );
      } else {
         from_acnts.modify( from, owner, [&]( auto& a ) {
            a.balance -= value;
         });
      }
   }

   void token::add_balance( account_name owner, asset value, account_name ram_payer ) {
      accounts to_acnts( _self, owner.value );
      auto to = to_acnts.find( value.symbol.code().raw() );
      if( to == to_acnts.end() ) {
         to_acnts.emplace( ram_payer, [&]( auto& a ){
         a.balance = value;
         });
      } else {
         to_acnts.modify( to, owner, [&]( auto& a ) {
         a.balance += value;
         });
      }
   }
}//end namespace

EOSIO_DISPATCH( eosio::token,(create)(issue)(transfer) )