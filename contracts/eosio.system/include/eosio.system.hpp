#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>

#include <string>

namespace eosio {

using std::string;

static constexpr symbol CORE_SYMBOL = symbol(symbol_code("EOS"), 4);

/**
 * @defgroup system_contract eosio.system
 * @ingroup eosiocontracts
 *
 * eosio.system contract
 *
 * @details eosio.system is the system contract for eosforce
 * @{
 */
class[[eosio::contract( "eosio.system" )]] system_contract : public contract {
   public:
      using contract::contract;

   public:
      struct [[eosio::table]] account_info {
         uint64_t name      = 0;
         asset    available = asset{ 0, CORE_SYMBOL };

         uint64_t primary_key() const { return name; }
      };
      
      typedef eosio::multi_index< "accounts"_n, account_info > accounts_table;

   public:
      [[eosio::action]]
      void transfer( const name& from, const name& to, const asset& quantity, const string& memo );
};

} // namespace eosio
