#pragma once

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>

#include <string>

namespace eosio {

   using std::string;

   static constexpr symbol CORE_SYMBOL    = symbol(symbol_code("EOS"), 4);
   static constexpr uint32_t FROZEN_DELAY = 3 * 24 * 60 * 20;               //3*24*60*20*3s;
   static constexpr int NUM_OF_TOP_BPS    = 23;
   static constexpr int BLOCK_REWARDS_BP  = 27000 ;                         //2.7000 EOS
   static constexpr int BLOCK_REWARDS_B1  = 3000;                           //0.3000 EOS
   static constexpr uint32_t UPDATE_CYCLE = 100;                            //every 100 blocks update

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

         using account_name = uint64_t;

      public:
         struct [[eosio::table]] account_info {
            account_name name      = 0;
            asset        available = asset{ 0, CORE_SYMBOL };

            uint64_t primary_key() const { return name; }
         };

         struct [[eosio::table]] vote_info {
            account_name bpname                = 0;
            asset        staked                = asset{ 0, CORE_SYMBOL };
            uint32_t     voteage_update_height = 0;
            int64_t      voteage               = 0;           // asset.amount * block height
            asset        unstaking             = asset{ 0, CORE_SYMBOL };
            uint32_t     unstake_height        = 0;

            uint64_t primary_key() const { return bpname; }

         };

         struct [[eosio::table]] vote4ram_info {
            account_name voter  = 0;
            asset        staked = asset{ 0, CORE_SYMBOL };

            uint64_t primary_key() const { return voter; }
         };

         struct [[eosio::table]] bp_info {
            account_name name = 0;
            public_key block_signing_key;
            uint32_t commission_rate       = 0;                        // 0 - 10000 for 0% - 100%
            int64_t  total_staked          = 0;
            asset    rewards_pool          = asset{ 0, CORE_SYMBOL };
            int64_t  total_voteage         = 0;                        // asset.amount * block height
            uint32_t voteage_update_height = 0;                        // this should be delete
            std:: string url;
            bool emergency = false;

            uint64_t primary_key() const { return name; }
         };

         struct [[eosio::table]] producer {
            account_name bpname;
            uint32_t amount = 0;
         };

         struct [[eosio::table]] producer_blacklist {
            account_name bpname;
            bool isactive = false;

            uint64_t primary_key() const { return bpname; }
            void     deactivate()       {isactive = false;}
         };


         struct [[eosio::table]] schedule_info {
            uint64_t version;
            uint32_t block_height;
            producer producers[NUM_OF_TOP_BPS];

            uint64_t primary_key() const { return version; }
         };

         struct [[eosio::table]] chain_status {
            account_name name = ("chainstatus"_n).value;
            bool emergency = false;

            uint64_t primary_key() const { return name; }
         };
         
         struct [[eosio::table]] heartbeat_info {
            account_name bpname;
            time_point_sec timestamp;
            
            uint64_t primary_key() const { return bpname; }
         };

         typedef eosio::multi_index< "accounts"_n, account_info > accounts_table;
         typedef eosio::multi_index< "votes"_n, vote_info > votes_table;
         typedef eosio::multi_index< "votes4ram"_n, vote_info > votes4ram_table;
         typedef eosio::multi_index< "vote4ramsum"_n, vote4ram_info > vote4ramsum_table;
         typedef eosio::multi_index< "bps"_n, bp_info > bps_table;
         typedef eosio::multi_index< "schedules"_n, schedule_info > schedules_table;
         typedef eosio::multi_index< "chainstatus"_n, chain_status > cstatus_table;
         typedef eosio::multi_index< "heartbeat"_n, heartbeat_info > hb_table;
         typedef eosio::multi_index< "blackpro"_n, producer_blacklist > blackproducer_table;

      public:
         [[eosio::action]]
         void transfer( const name& from, const name& to, const asset& quantity, const string& memo );
   };

} // namespace eosio
