#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>

#include <string>

namespace eosio {

   using std::string;
   using eosio::asset;
   using eosio::print;
   using eosio::bytes;
   using eosio::block_timestamp;
   using std::string;
   using eosio::time_point_sec;

   static constexpr symbol CORE_SYMBOL = symbol(symbol_code("EOS"), 4);
   static constexpr uint32_t FROZEN_DELAY = 3 * 24 * 60 * 20; //3*24*60*20*3s;
   static constexpr int NUM_OF_TOP_BPS = 23;
   static constexpr int BLOCK_REWARDS_BP = 27000 ; //2.7000 EOS
   static constexpr int BLOCK_REWARDS_B1 = 3000; //0.3000 EOS
   static constexpr uint32_t UPDATE_CYCLE = 100; //every 100 blocks update

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
