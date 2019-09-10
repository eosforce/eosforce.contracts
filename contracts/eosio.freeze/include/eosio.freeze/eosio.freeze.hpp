/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>

#include <string>
#include <vector>

namespace eosiosystem {
   class system_contract;
}

namespace eosio {

   // tables for lock token
   static constexpr symbol EOSLOCK_SYMBOL = symbol(symbol_code("EOSLOCK"), 4);

   // tables for freeze token
   struct [[eosio::table, eosio::contract("eosio.freeze")]] freezed {
      account_name account;
      uint32_t     unfreeze_block_num = 0; // unfreeze_block_num, 0 - 1024 is freeze forever
      uint32_t     unused_            = 0;
      uint64_t primary_key()const { return account; }
   };
   typedef eosio::multi_index< "freezed"_n, freezed > freezeds;

   enum freezed_table_stat_t : uint32_t {
      committing = 0,
      locked     = 1,
      actived    = 2,
      banned     = 3
   };

   struct [[eosio::table, eosio::contract("eosio.freeze")]] freezed_table_state {
      account_name committer;
      uint32_t     last_commit_block_num = 0; //
      uint32_t     locked_block_num      = 0; // --> very important, so need three block num in chain
      uint32_t     actived_block_num     = 0; //
      uint32_t     state                 = static_cast<uint32_t>(freezed_table_stat_t::committing);
      uint32_t     freezed_size          = 0;

      // no use right now
      std::vector<account_name> confirmed;
      std::vector<uint64_t>     ext_info;

      uint64_t primary_key()const { return committer; }
   };
   typedef eosio::multi_index< "freezedstat"_n, freezed_table_state > freezed_stat;

   struct [[eosio::table, eosio::contract("eosio.freeze")]] freezed_state {
      account_name committer = 0; // if is not 0, mean freezed table actived

      EOSLIB_SERIALIZE( freezed_state, (committer) )
   };
   typedef eosio::singleton< "gfstate"_n, freezed_state > global_freezed_state;

   struct [[eosio::table, eosio::contract("eosio.freeze")]] activited_account {
      account_name account;

      uint64_t primary_key()const { return account; }
   };
   typedef eosio::multi_index< "actaccounts"_n, activited_account > activited_accounts;

   // lock_token contract for lock accounts
   class [[eosio::contract("eosio.freeze")]] lock_token : public contract {
      public:
         using contract::contract;

         // Actions to make freezed account list by block producer

         // addfreezed add freezed account to freeze account table by committer
         [[eosio::action]] void addfreezed( const account_name& committer,
                                            const std::vector<account_name>& freezeds,
                                            const std::string& memo );

         // delfreezed del freezed account to freeze account table by committer
         [[eosio::action]] void delfreezed( const account_name& committer,
                                            const account_name& freezeds,
                                            const std::string& memo );

         // lockfreezed lock freeze account table by committer
         [[eosio::action]] void lockfreezed( const account_name& committer, const bool is_locked );

         // actfreezed activite freeze account table
         [[eosio::action]] void actfreezed( const account_name& committer );

         // confirmact confirm a account is active from freeze account table
         [[eosio::action]] void confirmact( const account_name& account );

      private:

   };

} /// namespace eosio
