#pragma once

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>
#include <eosio/system.hpp>

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
            std::string url;
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

         // tables
         typedef eosio::multi_index< "accounts"_n, account_info > accounts_table;
         typedef eosio::multi_index< "votes"_n, vote_info > votes_table;
         typedef eosio::multi_index< "votes4ram"_n, vote_info > votes4ram_table;
         typedef eosio::multi_index< "vote4ramsum"_n, vote4ram_info > vote4ramsum_table;
         typedef eosio::multi_index< "bps"_n, bp_info > bps_table;
         typedef eosio::multi_index< "schedules"_n, schedule_info > schedules_table;
         typedef eosio::multi_index< "chainstatus"_n, chain_status > cstatus_table;
         typedef eosio::multi_index< "heartbeat"_n, heartbeat_info > hb_table;
         typedef eosio::multi_index< "blackpro"_n, producer_blacklist > blackproducer_table;

      private:
         void update_elected_bps();
         void reward_bps( const std::vector<name>& block_producers, const time_point_sec& current_time_sec );

         inline void heartbeat_imp( const account_name& bpname, const time_point_sec& timestamp ) {
            hb_table hb_tbl( _self, _self.value );

            const auto hb_itr = hb_tbl.find( bpname );
            if( hb_itr == hb_tbl.end() ) {
               hb_tbl.emplace( name{ bpname }, [&]( heartbeat_info& hb ) {
                  hb.bpname = bpname;
                  hb.timestamp = timestamp;
               } );
            } else {
               hb_tbl.modify( hb_itr, name{}, [&]( heartbeat_info& hb ) { 
                  hb.timestamp = timestamp; 
               } );
            }
         }

      public:
         [[eosio::action]] void transfer( const account_name& from,
                                          const account_name& to,
                                          const asset& quantity,
                                          const string& memo );

         [[eosio::action]] void updatebp( const account_name& bpname,
                                          const public_key& producer_key,
                                          const uint32_t commission_rate,
                                          const std::string& url );

         [[eosio::action]] void vote( const account_name& voter, 
                                      const account_name& bpname, 
                                      const asset& stake );

         [[eosio::action]] void revote( const account_name& voter,
                                        const account_name& frombp,
                                        const account_name& tobp,
                                        const asset& restake );

         [[eosio::action]] void unfreeze( const account_name& voter, const account_name& bpname );

         [[eosio::action]] void vote4ram( const account_name& voter, 
                                          const account_name& bpname, 
                                          const asset& stake );

         [[eosio::action]] void unfreezeram( const account_name& voter, const account_name& bpname );

         [[eosio::action]] void claim( const account_name& voter, const account_name& bpname );

         [[eosio::action]] void onblock( const block_timestamp&,
                                         const account_name& bpname,
                                         const uint16_t,
                                         const checksum256&,
                                         const checksum256&,
                                         const checksum256&,
                                         const uint32_t schedule_version );

         [[eosio::action]] void onfee( const account_name& actor, 
                                       const asset& fee, 
                                       const account_name& bpname );

         [[eosio::action]] void setemergency( const account_name& bpname, const bool emergency );

         [[eosio::action]] void heartbeat( const account_name& bpname,
                                           const time_point_sec& timestamp );

         [[eosio::action]] void removebp( const account_name& producer );
   };

} // namespace eosio
