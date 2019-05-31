#pragma once
#include <../../eosforcelib/config.hpp>
#include <eosio/crypto.hpp>
#include <eosio/system.hpp>

namespace eosiosystem {
   using std::vector;
   static constexpr int BLOCK_REWARDS_BP = 27000 ; //2.7000 EOS
   static constexpr int BLOCK_REWARDS_B1 = 3000; //0.3000 EOS
   class [[eosio::contract("eosio.system")]] system_contract : public contract {
      public:
         using contract::contract;
         
         ACTION transfer( const account_name from, const account_name to, const asset quantity, const string memo );
         ACTION updatebp( const account_name bpname, const public_key producer_key,
                        const uint32_t commission_rate, const std::string& url );
         ACTION vote( const account_name voter, const account_name bpname, const asset stake );
         ACTION unfreeze( const account_name voter, const account_name bpname );
         ACTION vote4ram( const account_name voter, const account_name bpname, const asset stake );
         ACTION unfreezeram( const account_name voter, const account_name bpname );
         ACTION claim( const account_name voter, const account_name bpname );
         ACTION onblock( const block_timestamp, const account_name bpname, const uint16_t,
                     const block_id_type, const checksum256, const checksum256, const uint32_t schedule_version );
         ACTION onfee( const account_name actor, const asset fee, const account_name bpname );
         ACTION setemergency( const account_name bpname, const bool emergency );
         ACTION heartbeat( const account_name bpname, const time_point_sec timestamp );
         ACTION removebp( account_name producer );

         using transfer_action = action_wrapper<"transfer"_n, &system_contract::transfer>;
         using updatebp_action = action_wrapper<"updatebp"_n, &system_contract::updatebp>;
         using vote_action = action_wrapper<"vote"_n, &system_contract::vote>;
         using unfreeze_action = action_wrapper<"unfreeze"_n, &system_contract::unfreeze>;
         using vote4ram_action = action_wrapper<"vote4ram"_n, &system_contract::vote4ram>;
         using unfreezeram_action = action_wrapper<"unfreezeram"_n, &system_contract::unfreezeram>;
         using claim_action = action_wrapper<"claim"_n, &system_contract::claim>;
         using onblock_action = action_wrapper<"onblock"_n, &system_contract::onblock>;
         using onfee_action = action_wrapper<"onfee"_n, &system_contract::onfee>;
         using setemergency_action = action_wrapper<"setemergency"_n, &system_contract::setemergency>;
         using heartbeat_action = action_wrapper<"heartbeat"_n, &system_contract::heartbeat>;
         using removebp_action = action_wrapper<"removebp"_n, &system_contract::removebp>;
      private:
         TABLE account_info {
            account_name name;
            asset available = asset(0, CORE_SYMBOL);

            uint64_t primary_key() const { return name.value; }

            EOSLIB_SERIALIZE(account_info, ( name )(available))
         };

         TABLE vote_info {
            account_name bpname;
            asset staked = asset(0, CORE_SYMBOL);
            uint32_t voteage_update_height = current_block_num();
            int64_t voteage = 0; // asset.amount * block height
            asset unstaking = asset(0, CORE_SYMBOL);
            uint32_t unstake_height = current_block_num();

            uint64_t primary_key() const { return bpname.value; }

            EOSLIB_SERIALIZE(vote_info, ( bpname )(staked)(voteage)(voteage_update_height)(unstaking)(unstake_height))
         };

         TABLE vote4ram_info {
            account_name voter;
            asset staked = asset(0, CORE_SYMBOL);
            uint64_t primary_key() const { return voter.value; }

            EOSLIB_SERIALIZE(vote4ram_info, (voter)(staked))
         };

         TABLE bp_info {
            account_name name;
            public_key block_signing_key;
            uint32_t commission_rate = 0; // 0 - 10000 for 0% - 100%
            int64_t total_staked = 0;
            asset rewards_pool = asset(0, CORE_SYMBOL);
            int64_t total_voteage = 0; // asset.amount * block height
            uint32_t voteage_update_height = current_block_num();
            std::string url;
            bool emergency = false;

            uint64_t primary_key() const { return name.value; }

            void update( public_key key, uint32_t rate, std::string u ) {
               block_signing_key = key;
               commission_rate = rate;
               url = u;
            }

            EOSLIB_SERIALIZE(bp_info, ( name )(block_signing_key)(commission_rate)(total_staked)
                  (rewards_pool)(total_voteage)(voteage_update_height)(url)(emergency))
         };

         TABLE producer {
            account_name bpname;
            uint32_t amount = 0;
         };

         TABLE producer_blacklist {
            account_name bpname;
            bool isactive = false;

            uint64_t primary_key() const { return bpname.value; }
            void     deactivate()       {isactive = false;}

            EOSLIB_SERIALIZE(producer_blacklist, ( bpname )(isactive))
         };


         TABLE schedule_info {
            uint64_t version;
            uint32_t block_height;
            producer producers[config::NUM_OF_TOP_BPS];

            uint64_t primary_key() const { return version; }

            EOSLIB_SERIALIZE(schedule_info, ( version )(block_height)(producers))
         };

         TABLE chain_status {
            account_name name = "chainstatus"_n;
            bool emergency = false;

            uint64_t primary_key() const { return name.value; }

            EOSLIB_SERIALIZE(chain_status, ( name )(emergency))
         };
         
         TABLE heartbeat_info {
            account_name bpname;
            time_point_sec timestamp;
            
            uint64_t primary_key() const { return bpname.value; }
            
            EOSLIB_SERIALIZE(heartbeat_info, ( bpname )(timestamp))
         };

         typedef eosio::multi_index<"accounts"_n, account_info> accounts_table;
         typedef eosio::multi_index<"votes"_n, vote_info> votes_table;
         typedef eosio::multi_index<"votes4ram"_n, vote_info> votes4ram_table;
         typedef eosio::multi_index<"vote4ramsum"_n, vote4ram_info> vote4ramsum_table;
         typedef eosio::multi_index<"bps"_n, bp_info> bps_table;
         typedef eosio::multi_index<"schedules"_n, schedule_info> schedules_table;
         typedef eosio::multi_index<"chainstatus"_n, chain_status> cstatus_table;
         typedef eosio::multi_index<"heartbeat"_n, heartbeat_info> hb_table;
         typedef eosio::multi_index<"blackpro"_n, producer_blacklist> blackproducer_table;

         void update_elected_bps();
         void reward_bps( vector<account_name> block_producers );
         bool is_super_bp( vector<account_name> block_producers, account_name name );
   };//end class
}//end namespace