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
      uint64_t root_id;
      string title;
      string content;
      asset quantity;
      account_name proposer;
      uint32_t section;
      uint32_t takecoin_num;
      uint32_t approve_end_block_num;
      vector< vector<char> > extern_data;

      uint64_t primary_key()const { return id; }
      uint64_t get_root_id()const { return root_id; }
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

   struct [[eosio::table, eosio::contract("eosio.budget")]] budget_config {
      name config_name;
      uint64_t number_value;
      string string_value;;

      uint64_t primary_key() const { return config_name.value; }
   };

   typedef eosio::multi_index<"committee"_n,   committee_info> committee_table;
   typedef eosio::multi_index<"motions"_n,   motion_info,
      indexed_by< "byroot"_n,
                  eosio::const_mem_fun<motion_info, uint64_t, &motion_info::get_root_id >>> motion_table;
   typedef eosio::multi_index<"approvers"_n,   approval_info> approver_table;
   typedef eosio::multi_index<"takecoins"_n,   takecoin_motion_info> takecoin_table;
   typedef eosio::multi_index<"budgetconfig"_n,   budget_config> budgetconfig_table;

   class [[eosio::contract("eosio.budget")]] budget : public contract {
      public:
         using contract::contract;

         budget( name s, name code, datastream<const char*> ds );
         budget( const budget& ) = default;
         ~budget();

      private:
         budgetconfig_table     _budgetconfig;

      public:
         [[eosio::action]] void handover(const vector<account_name>& committeers ,const string& memo);

         [[eosio::action]] void propose(const account_name& proposer,const string& title,const string& content,const asset& quantity );

         [[eosio::action]] void approve(const account_name& approver,const uint64_t& id ,const string& memo);

         [[eosio::action]] void unapprove(const account_name& approver,const uint64_t& id ,const string& memo);

         [[eosio::action]] void takecoin(const account_name& proposer,const uint64_t& montion_id,const string& content,const asset& quantity );

         [[eosio::action]] void agreecoin(const account_name& approver,const account_name& proposer,const uint64_t& id ,const string& memo);
         
         [[eosio::action]] void unagreecoin(const account_name& approver,const account_name& proposer,const uint64_t& id ,const string& memo);

         [[eosio::action]] void turndown(const uint64_t& id ,const string& memo);

         [[eosio::action]] void closemotion(const uint64_t& id ,const string& memo);

         [[eosio::action]] void closecoin(const account_name& proposer,const uint64_t& id ,const string& memo);
         
         [[eosio::action]] void updateconfig( const name& config,const uint64_t &number_value,const string &string_value );
   };

} /// namespace eosio
