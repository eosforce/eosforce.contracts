/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>

#include <string>

namespace eosiosystem {
   class system_contract;
}

namespace eosio {

   static constexpr symbol EOSLOCK_SYMBOL = symbol(symbol_code("EOSLOCK"), 4);

   struct [[eosio::table, eosio::contract("eosio.lock")]] account {
      asset    balance;
      uint64_t primary_key()const { return balance.symbol.code().raw(); }
   };

   struct [[eosio::table, eosio::contract("eosio.lock")]] currency_stats {
      asset    max_locked;
      asset    current_unlocked;

      uint64_t primary_key()const { return max_locked.symbol.code().raw(); }
   };

   typedef eosio::multi_index< "accounts"_n, account > accounts;
   typedef eosio::multi_index< "stat"_n, currency_stats > stats;


   class [[eosio::contract("eosio.lock")]] lock_token : public contract {
      public:
         using contract::contract;

      private:
   };

} /// namespace eosio
