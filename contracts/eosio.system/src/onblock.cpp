#include <utility>

#include <eosio.system.hpp>

#include <eosio/privileged.hpp>
#include <eosio/producer_schedule.hpp>

namespace eosio {

   void system_contract::onblock( const block_timestamp&,
                                  const account_name& bpname,
                                  const uint16_t,
                                  const checksum256&,
                                  const checksum256&,
                                  const checksum256&,
                                  const uint32_t schedule_version ) {
      bps_table bps_tbl( _self, _self.value );
      accounts_table acnts_tbl( _self, _self.value );
      schedules_table schs_tbl( _self, _self.value );

      const auto curr_block_num = current_block_num();
      const auto& block_producers = get_active_producers();

      if( block_producers.size() < NUM_OF_TOP_BPS ){
         // cannot be happan, but should make a perpar
         // onblock should not error
         return;
      }

      auto sch = schs_tbl.find( uint64_t( schedule_version ) );
      if( sch == schs_tbl.end() ) {
         schs_tbl.emplace( name{bpname}, [&]( schedule_info& s ) {
            s.version = schedule_version;
            s.block_height = curr_block_num;
            for( int i = 0; i < NUM_OF_TOP_BPS; i++ ) {
               s.producers[i].amount = block_producers[i] == name{bpname} ? 1 : 0;
               s.producers[i].bpname = block_producers[i].value;
            }
         } );
      } else {
         schs_tbl.modify( sch, name{0}, [&]( schedule_info& s ) {
            for( int i = 0; i < NUM_OF_TOP_BPS; i++ ) {
               if( s.producers[i].bpname == bpname ) {
                  s.producers[i].amount += 1;
                  break;
               }
            }
         } );
      }

      const auto current_time_sec = time_point_sec( current_time_point() );

      // make cache for vote state
      make_global_votestate( curr_block_num );

      // producer a block is also make a heartbeat
      heartbeat_imp( bpname, current_time_sec );

      // reward bps
      reward_bps( block_producers, curr_block_num, current_time_sec );

      // update schedule
      if( curr_block_num % UPDATE_CYCLE == 0 ) {
         // reward block.one
         const auto& b1 = acnts_tbl.get( ( "b1"_n ).value, "b1 is not found in accounts table" );
         acnts_tbl.modify( b1, name{0}, [&]( account_info& a ) {
            a.available += asset( BLOCK_REWARDS_B1 * UPDATE_CYCLE, CORE_SYMBOL );
         } );

         update_elected_bps();
      }
   }

   void system_contract::update_elected_bps() {
      bps_table bps_tbl( _self, _self.value );

      constexpr auto bps_top_size = static_cast<size_t>( NUM_OF_TOP_BPS );

      std::vector<std::pair<producer_key, int64_t>> vote_schedule;
      vote_schedule.reserve( 32 );

      // Note this output is not same after updated
      blackproducer_table blackproducer( _self, _self.value );

      // TODO: use table sorted datas
      for( const auto& it : bps_tbl ) {
         const auto blackpro = blackproducer.find( it.name );
         if( blackpro != blackproducer.end() && ( !blackpro->isactive ) ) {
            continue;
         }

         const auto vs_size = vote_schedule.size();
         if( vs_size >= bps_top_size && vote_schedule[vs_size - 1].second > it.total_staked ) {
            continue;
         }

         // Just 23 node, just find by for
         for( int i = 0; i < bps_top_size; ++i ) {
            if( vote_schedule[i].second <= it.total_staked ) {
               vote_schedule.insert( vote_schedule.begin() + i,
                  std::make_pair( producer_key{ name{it.name}, it.block_signing_key }, it.total_staked ) );

               if( vote_schedule.size() > bps_top_size ) {
                  vote_schedule.resize( bps_top_size );
               }
               break;
            }
         }
      }

      if( vote_schedule.size() > bps_top_size ) {
         vote_schedule.resize( bps_top_size );
      }

      /// sort by producer name
      std::sort(vote_schedule.begin(), vote_schedule.end(), 
         []( const auto& l, const auto& r ) -> bool {
            return l.first.producer_name < r.first.producer_name;
         } );

      std::vector<eosio::producer_key> vote_schedule_data;
      vote_schedule_data.reserve( vote_schedule.size() );

      for( const auto& v : vote_schedule ) {
         vote_schedule_data.push_back( v.first );
      }

      set_proposed_producers( vote_schedule_data );
   }

   void system_contract::reward_bps( const std::vector<name>& block_producers,
                                     const uint32_t curr_block_num,
                                     const time_point_sec& current_time_sec ) {
      bps_table bps_tbl( _self, _self.value );
      accounts_table acnts_tbl( _self, _self.value );
      hb_table hb_tbl( _self, _self.value );

      const auto& global_votestate = get_global_votestate( curr_block_num );
      const auto& staked_all_bps = global_votestate.total_staked;

      if( staked_all_bps <= 0 ) {
         return;
      }
      // 0.5% of staked_all_bps
      const auto rewarding_bp_staked_threshold = staked_all_bps / 200;

      auto hb_max = get_num_config_on_chain( "hb.max"_n );
      if( hb_max < 0 ) {
         hb_max = 3600;
      }

      std::set<uint64_t> super_bps;
      for( int i = 0; i < NUM_OF_TOP_BPS; i++ ) {
         super_bps.insert( block_producers[i].value );
      }

      // reward bps, (bp_reward => bp_account_reward + bp_rewards_pool + eosfund_reward;)
      auto sum_bps_reward = 0;
      blackproducer_table blackproducer( _self, _self.value );
      for( auto it = bps_tbl.cbegin(); it != bps_tbl.cend(); ++it ) {
         const auto blackpro = blackproducer.find( it->name );
         if(    ( blackpro != blackproducer.end() && !blackpro->isactive )
             || it->total_staked <= rewarding_bp_staked_threshold
             || it->commission_rate < 1
             || it->commission_rate > 10000 ) {
            continue;
         }

         auto hb = hb_tbl.find( it->name );
         if(    hb == hb_tbl.end() 
             || ( hb->timestamp + static_cast<uint32_t>( hb_max ) ) < current_time_sec ) {
            continue;
         }

         const auto bp_reward = static_cast<int64_t>( BLOCK_REWARDS_BP * double( it->total_staked ) /double( staked_all_bps ) );

         // reward bp account
         auto bp_account_reward = bp_reward * 15 / 100 + bp_reward * 70 / 100 * it->commission_rate / 10000;
         if( super_bps.find( it->name ) != super_bps.end() ) {
            bp_account_reward += bp_reward * 15 / 100;
         }

         const auto& act = acnts_tbl.get( it->name, "bpname is not found in accounts table" );
         acnts_tbl.modify(act, name{0}, [&]( account_info& a ) { 
            a.available += asset( bp_account_reward, CORE_SYMBOL ); 
         } );

         // reward pool
         const auto bp_rewards_pool = bp_reward * 70 / 100 * ( 10000 - it->commission_rate ) / 10000;
         const auto& bp = bps_tbl.get( it->name, "bpname is not registered" );
         bps_tbl.modify( bp, name{0}, [&]( bp_info& b ) { 
            b.rewards_pool += asset( bp_rewards_pool, CORE_SYMBOL ); 
         } );

         sum_bps_reward += ( bp_account_reward + bp_rewards_pool );
      }

      // reward eosfund
      const auto total_eosfund_reward = BLOCK_REWARDS_BP - sum_bps_reward;
      if( total_eosfund_reward > 0 ) {
         const auto& eosfund = acnts_tbl.get( ( "devfund"_n ).value, "devfund is not found in accounts table" );
         acnts_tbl.modify( eosfund, name{0}, [&]( account_info& a ) { 
            a.available += asset( total_eosfund_reward, CORE_SYMBOL ); 
         } );
      }
   }

   const system_contract::global_votestate_info system_contract::get_global_votestate( const uint32_t curr_block_num ) {
      global_votestate_table votestat( get_self(), get_self().value );
      const auto it = votestat.find( eosforce_vote_stat.value );
      if( it == votestat.end() ) {
         global_votestate_info res;

         bps_table bps_tbl( get_self(), get_self().value );
         // calculate total staked all of the bps
         int64_t staked_for_all_bps = 0;
         for( const auto& bp : bps_tbl ) {
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

   void system_contract::onfee( const account_name& actor,
                                const asset& fee,
                                const account_name& bpname ) {
      accounts_table acnts_tbl( _self, _self.value );
      const auto& act = acnts_tbl.get( actor, "account is not found in accounts table" );
      check( fee.amount <= act.available.amount, "overdrawn available balance" );

      bps_table bps_tbl( _self, _self.value );
      const auto& bp = bps_tbl.get( bpname, "bpname is not registered" );

      acnts_tbl.modify( act, name{0}, [&]( account_info& a ) { 
         a.available -= fee; 
      } );

      bps_tbl.modify( bp, name{0}, [&]( bp_info& b ) { 
         b.rewards_pool += fee; 
      } );
   }
} /// namespace eosio
