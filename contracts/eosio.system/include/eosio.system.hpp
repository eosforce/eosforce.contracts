#pragma once

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>
#include <eosio/system.hpp>

#include <string>

#include <eosforce/assetage.hpp>
#include "native.hpp"
#include "vote.hpp"

namespace eosio {

   using std::string;
   using std::vector;

   using eosforce::assetage;
   using eosforce::CORE_SYMBOL;
   using eosforce::CORE_SYMBOL_PRECISION;

   static constexpr uint32_t BLOCK_NUM_PER_DAY     = 24 * 60 * 20;
   static constexpr uint32_t FROZEN_DELAY          = 3 * BLOCK_NUM_PER_DAY;
   static constexpr int NUM_OF_TOP_BPS             = 23;
   static constexpr int BLOCK_REWARDS_BP           = 27000;
   static constexpr uint32_t UPDATE_CYCLE          = NUM_OF_TOP_BPS * 5;     // every num blocks update
   static constexpr uint32_t REWARD_B1_CYCLE       = NUM_OF_TOP_BPS * 100;
   static constexpr uint32_t BP_CYCLE_BLOCK_OUT    = 1;
   static constexpr uint32_t BASE_BLOCK_OUT_WEIGHT = 1000;
   static constexpr uint32_t BLOCK_OUT_WEIGHT_LIMIT = 4800;
   static constexpr uint32_t BP_PUBISH_DRAIN_NUM = 9;
   static constexpr uint32_t APPROVE_TO_PUNISH_NUM = 16; 
   static constexpr uint32_t BLOCK_OUT_REWARD = 0; 
   static constexpr uint32_t PUNISH_BP_LIMIT = 28800; 
   static constexpr uint32_t MIN_CLAIM_BP = 100*10000; 
   static constexpr int BLOCK_REWARDS_B1           = 3000;
   
   static constexpr uint32_t BLOCK_BUDGET_REWARD = 15000;
   static constexpr uint32_t DRAIN_BLOCK_PUNISH = ( BLOCK_REWARDS_BP + BLOCK_OUT_REWARD + BLOCK_BUDGET_REWARD ) * 2; 
   static constexpr uint32_t BASE_BLOCK_OUT_PLEDGE = DRAIN_BLOCK_PUNISH * PUNISH_BP_LIMIT / NUM_OF_TOP_BPS ; 

   static constexpr uint32_t WRONG_DRAIN_BLOCK = 10000000;

   static constexpr uint32_t MAX_LAST_PRODUCER_SIZE = 120; 

   static constexpr name eosforce_vote_stat = "eosforce"_n;
   static constexpr name chainstatus_name   = "chainstatus"_n;
   static constexpr name bp_reward_name     = "bpreward"_n;
   static constexpr name bp_producer_name   = "bpproducer"_n;

   enum BPSTATUS : uint32_t { 
      NORMAL = 0,
      LACK_PLEDGE,
      PUNISHED
   };

   enum vote_stake_typ : uint32_t {
      use_account_token   = 1,
      use_unstaking_token = 2
   };

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
      account_name            bpname;
      time_point_sec          timestamp;

      uint64_t primary_key() const { return bpname; }
   };

   struct [[eosio::table, eosio::contract("eosio.system")]] block_reward {
      account_name   name    = bp_reward_name.value;
      account_name   last_standard_bp;
      asset          reward_block_out;
      asset          reward_budget;

      uint64_t       total_block_age;
      uint32_t       last_reward_block_num;

      uint64_t primary_key() const { return name; }
   };

   struct [[eosio::table, eosio::contract("eosio.system")]] bp_monitor {
      account_name bpname;
      uint32_t       last_block_num;               //The number of blocks in the previous round
      uint32_t       consecutive_drain_block;      // Number of consecutive drain blocks
      uint32_t       consecutive_produce_block;    // Number of consecutive produce blocks
      uint32_t       total_drain_block;            // Number of total drain blocks
      uint32_t       stability;
      uint64_t       bock_age;
      uint32_t       bp_status;
      uint32_t       end_punish_block;

      uint64_t primary_key() const { return bpname; }
   };

   struct [[eosio::table, eosio::contract("eosio.system")]] drain_block_info {
      uint64_t       current_block_num;
      uint32_t       drain_block_num;

      uint64_t primary_key() const { return current_block_num; }
   };

   struct [[eosio::table, eosio::contract("eosio.system")]] punish_bp_info {
      account_name punish_bp_name;
      account_name proposaler;
      vector<account_name> approve_bp;
      uint32_t     effective_block_num;

      uint64_t primary_key() const { return punish_bp_name; }
   };

   struct [[eosio::table, eosio::contract("eosio.system")]] bps_reward {
      account_name   bpname;
      asset          reward;

      uint64_t primary_key() const { return bpname; }
   };

   struct [[eosio::table, eosio::contract("eosio.system")]] last_producer {
      account_name   name;
      uint32_t next_index;
      uint32_t max_size;
      std::vector<account_name> producers;

      uint64_t primary_key() const { return name; }
   };

   struct [[eosio::table, eosio::contract("eosio.system")]] heartbind_info {
      account_name   bpname;
      account_name   user;

      uint64_t primary_key() const { return user; }
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
   typedef eosio::multi_index<"blockreward"_n, block_reward>          blockreward_table;
   typedef eosio::multi_index<"bpmonitor"_n,   bp_monitor>            bpmonitor_table;
   typedef eosio::multi_index<"drainblocks"_n, drain_block_info>      drainblock_table;
   typedef eosio::multi_index<"punishbps"_n,   punish_bp_info>        punishbp_table;
   typedef eosio::multi_index<"bpsreward"_n,   bps_reward>            bpreward_table;
   typedef eosio::multi_index<"lastproducer"_n,last_producer>         lastproducer_table;
   typedef eosio::multi_index<"heartbind"_n,   heartbind_info>        heartbind_table;
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
         bpmonitor_table     _bpmonitors;
         lastproducer_table  _lastproducers;
         heartbind_table     _heartbinds;

      private:
         // to bps in onblock
         void update_elected_bps();
         void reward_bps( const std::vector<name>& block_producers,
                          const uint32_t curr_block_num,
                          const time_point_sec& current_time_sec );

         void reward_block(const uint32_t curr_block_num,
                           const account_name& bpname,
                           const uint32_t schedule_version,
                           const bool is_change_producers);

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
         inline void heartbeat_imp( const account_name& bpname,
                                    const uint32_t& curr_block_num,
                                    const time_point_sec& timestamp );
         inline bool is_producer_in_blacklist( const account_name& bpname ) const;
         inline bool is_producer_in_punished( const account_name& bpname ) const;

         bool is_super_bp( const account_name &bpname ) const;
         void exec_punish_bp( const account_name &bpname );

         bool is_reward_block(const bool &is_change_sch,const uint32_t &block_amount,const account_name &bpname);
         int32_t cal_drain_num(const bool &is_change_sch,const uint32_t index,const uint32_t &ifirst,const uint32_t &ilast,const uint32_t &pre_block_amount,const uint32_t &current_block_amount);

         int32_t drainblock_revise(const account_name &bpname);

         bool is_account_freezed( const account_name& account, const uint32_t curr_block_num );

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

         // make a fix-time vote by voter to bpname with stake token
         // type is fix-time type, 
         [[eosio::action]] void votefix( const account_name& voter,
                                         const account_name& bpname,
                                         const name& type,
                                         const asset& stake,
                                         const uint32_t& stake_typ );

         [[eosio::action]] void revotefix( const account_name& voter,
                                           const uint64_t& key,
                                           const account_name& bpname );

         // take out stake to a fix-time vote by voter after vote is timeout
         [[eosio::action]] void outfixvote( const account_name& voter,
                                            const uint64_t& key );

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
         [[eosio::action]] void punishbp( const account_name& bpname,const account_name& proposaler );
         [[eosio::action]] void approvebp( const account_name& bpname,const account_name& approver );
         [[eosio::action]] void bailpunish( const account_name& bpname );

         [[eosio::action]] void bpclaim( const account_name& bpname );

         [[eosio::action]] void monitorevise( const account_name& bpname );
         [[eosio::action]] void removepunish( const account_name& bpname );

         [[eosio::action]] void heartbind( const account_name& bpname,const account_name &user );
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
   using votefix_action      = eosio::action_wrapper<"votefix"_n,      &system_contract::votefix>;
   using revotefix_action    = eosio::action_wrapper<"revotefix"_n,    &system_contract::revotefix>;
   using outfixvote_action   = eosio::action_wrapper<"outfixvote"_n,   &system_contract::outfixvote>;
   using punishbp_action     = eosio::action_wrapper<"punishbp"_n,     &system_contract::punishbp>;
   using approvebp_action    = eosio::action_wrapper<"approvebp"_n,    &system_contract::approvebp>;
   using bailpunish_action   = eosio::action_wrapper<"bailpunish"_n,   &system_contract::bailpunish>;
   using bpclaim_action      = eosio::action_wrapper<"bpclaim"_n,      &system_contract::bpclaim>;
   using removepunish_action = eosio::action_wrapper<"removepunish"_n, &system_contract::removepunish>;
   using heartbind_action    = eosio::action_wrapper<"heartbind"_n,    &system_contract::heartbind>;

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

   inline void system_contract::heartbeat_imp( const account_name& bpname,
                                               const uint32_t& curr_block_num,
                                               const time_point_sec& timestamp ) {
      hb_table hb_tbl( _self, _self.value );

      const auto hb_itr = hb_tbl.find( bpname );
      if( hb_itr == hb_tbl.end() ) {
         hb_tbl.emplace( name{ bpname }, [&]( heartbeat_info& hb ) {
            hb.bpname = bpname;
            hb.timestamp = timestamp;
         } );
      } else {
         hb_tbl.modify( hb_itr, name{ bpname }, [&]( heartbeat_info& hb ) {
            hb.timestamp = timestamp;
         } );
      }
   }

   inline bool system_contract::is_producer_in_blacklist( const account_name& bpname ) const {
      const auto itr = _blackproducers.find( bpname );
      // Note isactive is false mean bp is ban
      return itr != _blackproducers.end() && ( !itr->isactive );
   }

   inline bool system_contract::is_producer_in_punished( const account_name& bpname ) const {
      const auto itr = _bpmonitors.find( bpname );
      // Note bp_status is 2 mean bp is punished
      return itr != _bpmonitors.end() && ( itr->bp_status == BPSTATUS::PUNISHED );
   }
} // namespace eosio
