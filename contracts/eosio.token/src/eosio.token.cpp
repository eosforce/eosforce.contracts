<<<<<<< HEAD
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
=======
/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eosio.token/eosio.token.hpp>

namespace eosio {

void token::create( const name&   issuer,
                    const asset&  maximum_supply )
{
   require_auth( issuer );

    const auto sym = maximum_supply.symbol;
    check( sym.is_valid(), "invalid symbol name" );
    check( maximum_supply.is_valid(), "invalid supply");
    check( maximum_supply.amount > 0, "max-supply must be positive");
    const auto sym_code_raw = sym.code().raw();

    static const auto core_symbol_code = symbol_code{ "EOS" };
    check( sym_code_raw != core_symbol_code.raw(), "not create EOS");

    stats statstable( get_self(), sym_code_raw );
    auto existing = statstable.find( sym_code_raw );
    check( existing == statstable.end(), "token with symbol already exists" );

    statstable.emplace( get_self(), [&]( auto& s ) {
       s.supply.symbol = maximum_supply.symbol;
       s.max_supply    = maximum_supply;
       s.issuer        = issuer;
    });
}


void token::issue( const name& to, const asset& quantity, const string& memo )
{
    const auto sym = quantity.symbol;
    check( sym.is_valid(), "invalid symbol name" );
    check( memo.size() <= 256, "memo has more than 256 bytes" );

    const auto sym_code_raw = sym.code().raw();
    stats statstable( _self, sym_code_raw );
    auto existing = statstable.find( sym_code_raw );
    check( existing != statstable.end(), "token with symbol does not exist, create token before issue" );
    const auto& st = *existing;

    require_auth( st.issuer );
    check( quantity.is_valid(), "invalid quantity" );
    check( quantity.amount > 0, "must issue positive quantity" );

    check( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    check( quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

    statstable.modify( st, name{}, [&]( auto& s ) {
       s.supply += quantity;
    });

    add_balance( st.issuer, quantity, st.issuer );

    if( to != st.issuer ) {
      SEND_INLINE_ACTION( *this, transfer, { {st.issuer, "active"_n} },
                          { st.issuer, to, quantity, memo }
      );
    }
}

void token::retire( const asset& quantity, const string& memo )
{
    auto sym = quantity.symbol;
    check( sym.is_valid(), "invalid symbol name" );
    check( memo.size() <= 256, "memo has more than 256 bytes" );
    const auto sym_code_raw = sym.code().raw();

    stats statstable( _self, sym_code_raw );
    auto existing = statstable.find( sym_code_raw );
    check( existing != statstable.end(), "token with symbol does not exist" );
    const auto& st = *existing;

    require_auth( st.issuer );
    check( quantity.is_valid(), "invalid quantity" );
    check( quantity.amount > 0, "must retire positive quantity" );

    check( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );

    statstable.modify( st, name{}, [&]( auto& s ) {
       s.supply -= quantity;
    });

    sub_balance( st.issuer, quantity );
}

void token::transfer( const name&    from,
                      const name&    to,
                      const asset&   quantity,
                      const string&  memo )
{
    check( from != to, "cannot transfer to self" );
    require_auth( from );
    check( is_account( to ), "to account does not exist");
    const auto sym_code_raw = quantity.symbol.code().raw();
    stats statstable( _self, sym_code_raw );
    const auto& st = statstable.get( sym_code_raw );

    require_recipient( from );
    require_recipient( to );

    check( quantity.is_valid(), "invalid quantity" );
    check( quantity.amount > 0, "must transfer positive quantity" );
    check( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    check( memo.size() <= 256, "memo has more than 256 bytes" );

    sub_balance( from, quantity );
    add_balance( to, quantity, from );
}

void token::sub_balance( const name& owner, const asset& value ) {
   accounts from_acnts( _self, owner.value );

   const auto& from = from_acnts.get( value.symbol.code().raw(), "no balance object found" );
   check( from.balance.amount >= value.amount, "overdrawn balance" );

   if( from.balance.amount == value.amount ) {
      from_acnts.erase( from );
   } else {
      from_acnts.modify( from, owner, [&]( auto& a ) {
          a.balance -= value;
      });
   }
}

void token::add_balance( const name& owner, const asset& value, const name& ram_payer )
{
   accounts to_acnts( _self, owner.value );
   auto to = to_acnts.find( value.symbol.code().raw() );
   if( to == to_acnts.end() ) {
      to_acnts.emplace( ram_payer, [&]( auto& a ){
        a.balance = value;
      });
   } else {
      to_acnts.modify( to, name{}, [&]( auto& a ) {
        a.balance += value;
      });
   }
}

void token::open( const name& owner, const symbol& symbol, const name& ram_payer )
{
   require_auth( ram_payer );

   const auto sym_code_raw = symbol.code().raw();

   stats statstable( _self, sym_code_raw );
   const auto& st = statstable.get( sym_code_raw, "symbol does not exist" );
   check( st.supply.symbol == symbol, "symbol precision mismatch" );

   accounts acnts( _self, owner.value );
   auto it = acnts.find( sym_code_raw );
   if( it == acnts.end() ) {
      acnts.emplace( ram_payer, [&]( auto& a ){
        a.balance = asset{0, symbol};
      });
   }
}

void token::close( const name& owner, const symbol& symbol )
{
   // as in eosforce, every balance == 0 token will auto del ram, so it is useless in eosforce
   require_auth( owner );
   accounts acnts( _self, owner.value );
   auto it = acnts.find( symbol.code().raw() );
   check( it != acnts.end(), "Balance row already deleted or never existed. Action won't have any effect." );
   check( it->balance.amount == 0, "Cannot close because the balance is not zero." );
   acnts.erase( it );
}

} /// namespace eosio
>>>>>>> upstream/release-v1.1.x
