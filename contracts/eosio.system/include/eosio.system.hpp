#pragma once
<<<<<<< HEAD
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
=======

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>
#include <eosio/system.hpp>

#include <string>

#include <eosforce/assetage.hpp>
#include <native.hpp>

namespace eosio {

   using std::string;

   using eosforce::assetage;
   using eosforce::CORE_SYMBOL;
   using eosforce::CORE_SYMBOL_PRECISION;

   static constexpr uint32_t BLOCK_NUM_PER_DAY = 24 * 60 * 20;
   static constexpr uint32_t FROZEN_DELAY      = 3 * BLOCK_NUM_PER_DAY;
   static constexpr int NUM_OF_TOP_BPS         = 23;
   static constexpr int BLOCK_REWARDS_BP       = 27000;
   static constexpr int BLOCK_REWARDS_B1       = 3000;
   static constexpr uint32_t UPDATE_CYCLE      = NUM_OF_TOP_BPS * 5; // every num blocks update
   static constexpr uint32_t REWARD_B1_CYCLE   = NUM_OF_TOP_BPS * 100;

   static constexpr name eosforce_vote_stat = "eosforce"_n;
   static constexpr name chainstatus_name   = "chainstatus"_n;

   // tables
   struct [[eosio::table, eosio::contract("eosio.system")]] account_info {
      account_name name      = 0;
      asset        available = asset{ 0, CORE_SYMBOL };

      uint64_t primary_key() const { return name; }
   };

   // global_votestate_info some global data to vote state
   struct [[eosio::table, eosio::contract("eosio.system")]] global_votestate_info {
      name    stat_name    = eosforce_vote_stat;
      int64_t total_staked = -1;

      uint64_t primary_key() const { return stat_name.value; }
   };

   struct [[eosio::table, eosio::contract("eosio.system")]] vote_info {
      account_name bpname = 0;
      assetage     voteage;
      asset        unstaking      = asset{ 0, CORE_SYMBOL };
      uint32_t     unstake_height = 0;

      uint64_t primary_key() const { return bpname; }
   };

   struct [[eosio::table, eosio::contract("eosio.system")]] vote4ram_info {
      account_name voter  = 0;
      asset        staked = asset{ 0, CORE_SYMBOL };

      uint64_t primary_key() const { return voter; }
   };

   struct [[eosio::table, eosio::contract("eosio.system")]] bp_info {
      account_name name             = 0;
      public_key   block_signing_key;
      uint32_t     commission_rate  = 0; // 0 - 10000 for 0% - 100%
      int64_t      total_staked     = 0;
      asset        rewards_pool     = asset{ 0, CORE_SYMBOL };
      int64_t      total_voteage    = 0;          // asset.amount * block height
      uint32_t     voteage_update_height = 0; // this should be delete
      string       url;
      bool         emergency             = false;

      // for bp_info, cannot change it table struct
      inline void add_total_staked( const uint32_t curr_block_num, const asset& s );
      inline void add_total_staked( const uint32_t curr_block_num, const int64_t sa );
      inline constexpr int64_t get_age( const uint32_t curr_block_num ) const;

      uint64_t primary_key() const { return name; }
   };

   struct [[eosio::table, eosio::contract("eosio.system")]] producer_blacklist {
      account_name bpname;
      bool         isactive = false;

      uint64_t primary_key() const { return bpname; }
   };

   struct [[eosio::table, eosio::contract("eosio.system")]] schedule_info {
      struct producer {
         account_name bpname;
         uint32_t     amount = 0;

         EOSLIB_SERIALIZE( producer, ( bpname )( amount ) )
      };

      uint64_t              version;
      uint32_t              block_height;
      std::vector<producer> producers;

      uint64_t primary_key() const { return version; }

      EOSLIB_SERIALIZE( schedule_info, ( version )( block_height )( producers ) )
   };

   struct [[eosio::table, eosio::contract("eosio.system")]] chain_status {
      account_name name      = chainstatus_name.value;
      bool         emergency = false;

      uint64_t primary_key() const { return name; }
   };

   struct [[eosio::table, eosio::contract("eosio.system")]] heartbeat_info {
      account_name   bpname;
      time_point_sec timestamp;

      uint64_t primary_key() const { return bpname; }
   };

   // system contract tables
   typedef eosio::multi_index<"accounts"_n,    account_info>          accounts_table;
   typedef eosio::multi_index<"votes"_n,       vote_info>             votes_table;
   typedef eosio::multi_index<"votes4ram"_n,   vote_info>             votes4ram_table;
   typedef eosio::multi_index<"vote4ramsum"_n, vote4ram_info>         vote4ramsum_table;
   typedef eosio::multi_index<"bps"_n,         bp_info>               bps_table;
   typedef eosio::multi_index<"schedules"_n,   schedule_info>         schedules_table;
   typedef eosio::multi_index<"chainstatus"_n, chain_status>          cstatus_table;
   typedef eosio::multi_index<"heartbeat"_n,   heartbeat_info>        hb_table;
   typedef eosio::multi_index<"blackpro"_n,    producer_blacklist>    blackproducer_table;
   typedef eosio::multi_index<"gvotestat"_n,   global_votestate_info> global_votestate_table;

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
      using contract::contract;

      public:
         // WARNNING : EOSForce is different to eosio, which system will not call native contract in chain
         // so native is just to make abi, system_contract no need contain navtive
         system_contract( name s, name code, datastream<const char*> ds );
         system_contract( const system_contract& ) = default;
         ~system_contract();

      private:
         // tables for self, self, just some muit-used tables
         accounts_table      _accounts;
         bps_table           _bps;
         blackproducer_table _blackproducers;

      private:
         // to bps in onblock
         void update_elected_bps();
         void reward_bps( const std::vector<name>& block_producers,
                          const uint32_t curr_block_num,
                          const time_point_sec& current_time_sec );

         template< typename VOTE_TYP >
         int64_t vote_by_typ_imp( const account_name& voter,
                                  const account_name& bpname,
                                  const asset& stake );
         template< typename VOTE_TYP >
         void unfreeze_by_typ_imp( const account_name& voter,
                                   const account_name& bpname );

         // imps
         inline const global_votestate_info get_global_votestate( const uint32_t curr_block_num );
         inline void make_global_votestate( const uint32_t curr_block_num );
         inline void on_change_total_staked( const uint32_t curr_block_num, const int64_t& deta );
         inline void heartbeat_imp( const account_name& bpname, const time_point_sec& timestamp );
         inline bool is_producer_in_blacklist( const account_name& bpname ) const;

      public:
         [[eosio::action]] void transfer( const account_name& from,
                                          const account_name& to,
                                          const asset& quantity,
                                          const string& memo );

         [[eosio::action]] void updatebp( const account_name& bpname,
                                          const public_key& block_signing_key,
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

         [[eosio::action]] void onblock( const block_timestamp& timestamp,
                                         const account_name&    bpname,
                                         const uint16_t         confirmed,
                                         const checksum256&     previous,
                                         const checksum256&     transaction_mroot,
                                         const checksum256&     action_mroot,
                                         const uint32_t         schedule_version );

         [[eosio::action]] void setemergency( const account_name& bpname, const bool emergency );

         [[eosio::action]] void heartbeat( const account_name& bpname,
                                           const time_point_sec& timestamp );

         [[eosio::action]] void removebp( const account_name& bpname );
   };

   using transfer_action     = eosio::action_wrapper<"transfer"_n,     &system_contract::transfer>;
   using updatebp_action     = eosio::action_wrapper<"updatebp"_n,     &system_contract::updatebp>;
   using vote_action         = eosio::action_wrapper<"vote"_n,         &system_contract::vote>;
   using revote_action       = eosio::action_wrapper<"revote"_n,       &system_contract::revote>;
   using unfreeze_action     = eosio::action_wrapper<"unfreeze"_n,     &system_contract::unfreeze>;
   using vote4ram_action     = eosio::action_wrapper<"vote4ram"_n,     &system_contract::vote4ram>;
   using unfreezeram_action  = eosio::action_wrapper<"unfreezeram"_n,  &system_contract::unfreezeram>;
   using claim_action        = eosio::action_wrapper<"claim"_n,        &system_contract::claim>;
   using setemergency_action = eosio::action_wrapper<"setemergency"_n, &system_contract::setemergency>;
   using heartbeat_action    = eosio::action_wrapper<"heartbeat"_n,    &system_contract::heartbeat>;
   using removebp_action     = eosio::action_wrapper<"removebp"_n,     &system_contract::removebp>;

   // for bp_info, cannot change it table struct
   inline void bp_info::add_total_staked( const uint32_t curr_block_num, const asset& s ) {
      total_voteage += total_staked * ( curr_block_num - voteage_update_height );
      voteage_update_height = curr_block_num;
      // JUST CORE_TOKEN can vote to bp
      total_staked += s.amount / CORE_SYMBOL_PRECISION;
   }

   inline void bp_info::add_total_staked( const uint32_t curr_block_num, const int64_t sa ) {
      total_voteage += total_staked * ( curr_block_num - voteage_update_height );
      voteage_update_height = curr_block_num;
      // JUST CORE_TOKEN can vote to bp
      total_staked += sa / CORE_SYMBOL_PRECISION;
   }

   inline constexpr int64_t bp_info::get_age( const uint32_t curr_block_num ) const {
      return ( total_staked * ( curr_block_num - voteage_update_height ) ) + total_voteage;
   }

   inline void system_contract::make_global_votestate( const uint32_t curr_block_num ) {
      get_global_votestate( curr_block_num );
   }

   inline const global_votestate_info system_contract::get_global_votestate( const uint32_t curr_block_num ) {
      global_votestate_table votestat( get_self(), get_self().value );
      const auto it = votestat.find( eosforce_vote_stat.value );
      if( it == votestat.end() ) {
         global_votestate_info res;

         // calculate total staked all of the bps
         int64_t staked_for_all_bps = 0;
         for( const auto& bp : _bps ) {
            staked_for_all_bps += bp.total_staked;
         }
         res.total_staked = staked_for_all_bps;

         votestat.emplace( eosforce_vote_stat, [&]( global_votestate_info& g ) { 
            g = res;
         } );

         return res;
      }

      return *it;
   }

   inline void system_contract::on_change_total_staked( const uint32_t curr_block_num, const int64_t& deta ) {
      make_global_votestate( curr_block_num );
      global_votestate_table votestat( get_self(), get_self().value );
      const auto it = votestat.find( eosforce_vote_stat.value );
      check( it != votestat.end(), "make_global_votestate failed" );

      votestat.modify( it, name{}, [&]( global_votestate_info& g ) {
         g.total_staked += ( deta / CORE_SYMBOL_PRECISION );
      } );
   }

   inline void system_contract::heartbeat_imp( const account_name& bpname, const time_point_sec& timestamp ) {
      hb_table hb_tbl( _self, _self.value );

      const auto hb_itr = hb_tbl.find( bpname );
      if( hb_itr == hb_tbl.end() ) {
         hb_tbl.emplace( name{ bpname }, [&]( heartbeat_info& hb ) {
            hb.bpname = bpname;
            hb.timestamp = timestamp;
         } );
      } else {
         hb_tbl.modify( hb_itr, name{}, [&]( heartbeat_info& hb ) { hb.timestamp = timestamp; } );
      }
   }

   inline bool system_contract::is_producer_in_blacklist( const account_name& bpname ) const {
      const auto itr = _blackproducers.find( bpname );
      // Note isactive is false mean bp is ban
      return itr != _blackproducers.end() && ( !itr->isactive );
   }
} // namespace eosio
>>>>>>> upstream/release-v1.1.x
