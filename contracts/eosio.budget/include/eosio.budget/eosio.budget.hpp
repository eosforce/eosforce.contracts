#pragma once

#include <string>

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>

#include <eosforce/assetage.hpp>

namespace eosio {
   using std::string;
   using std::vector;
   using eosforce::CORE_SYMBOL;

   static constexpr name EOSIO_BUDGET = "eosio.budget"_n;
   static constexpr int64_t APPROVE_BLOCK_NUM = 14 * 28800;
   static constexpr uint32_t MIN_BUDGET_PLEDGE = 100*10000;

   struct [[eosio::table, eosio::contract("eosio.budget")]] committee_info {
      name    budget_name    = EOSIO_BUDGET;
      vector<account_name> member;

      uint64_t primary_key()const { return budget_name.value; }
   };

   struct [[eosio::table, eosio::contract("eosio.budget")]] motion_info {
      uint64_t id;
      string title;
      string content;
      asset quantity;
      account_name proposer;
      uint32_t section;
      uint32_t takecoin_num;
      uint32_t approve_end_block_num;
      uint32_t end_block_num;

      uint64_t primary_key()const { return id; }
   };

   struct [[eosio::table, eosio::contract("eosio.budget")]] approval_info {
      uint64_t id;
      vector<account_name> requested;
      vector<account_name> approved;
      vector<account_name> unapproved;

      uint64_t primary_key()const { return id; }
   };

   struct [[eosio::table, eosio::contract("eosio.budget")]]  takecoin_motion_info{ 
      uint64_t  id;
      uint64_t  montion_id;
      string content;
      asset quantity;
      account_name receiver;
      uint32_t section;
      uint32_t end_block_num;
      vector<account_name> requested;
      vector<account_name> approved;
      vector<account_name> unapproved;
      uint64_t primary_key()const { return id; }
   };

   typedef eosio::multi_index<"committee"_n,   committee_info> committee_table;
   typedef eosio::multi_index<"motions"_n,   motion_info> motion_table;
   typedef eosio::multi_index<"approvers"_n,   approval_info> approver_table;
   typedef eosio::multi_index<"takecoins"_n,   takecoin_motion_info> takecoin_table;

   class [[eosio::contract("eosio.budget")]] budget : public contract {
      public:
         using contract::contract;

         budget( name s, name code, datastream<const char*> ds );
         budget( const budget& ) = default;
         ~budget();

      public:
         [[eosio::action]] void handover( vector<account_name> committeers ,string memo);

         [[eosio::action]] void propose( account_name proposer,string title,string content,asset quantity,uint32_t end_num );

         [[eosio::action]] void approve( account_name approver,uint64_t id ,string memo);

         [[eosio::action]] void unapprove( account_name approver,uint64_t id ,string memo);

         [[eosio::action]] void takecoin( account_name proposer,uint64_t montion_id,string content,asset quantity );

         [[eosio::action]] void agreecoin( account_name approver,account_name proposer,uint64_t id ,string memo);
         
         [[eosio::action]] void unagreecoin( account_name approver,account_name proposer,uint64_t id ,string memo);

         [[eosio::action]] void turndown( uint64_t id ,string memo);
         
   };

} /// namespace eosio
