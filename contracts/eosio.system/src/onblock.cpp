#include <utility>

#include <eosio.system.hpp>

#include <eosio/privileged.hpp>
#include <eosio/producer_schedule.hpp>
#include <../../eosio.pledge/include/eosio.pledge/eosio.pledge.hpp>

namespace eosio {

   void system_contract::onblock( const block_timestamp&,
                                  const account_name& bpname,
                                  const uint16_t,
                                  const checksum256&,
                                  const checksum256&,
                                  const checksum256&,
                                  const uint32_t schedule_version ) {
      const auto curr_block_num = current_block_num();
      const auto& block_producers = get_active_producers();

      if( block_producers.size() < NUM_OF_TOP_BPS ){
         // cannot be happan, but should make a perpar
         // onblock should not error
         return;
      }
      uint32_t pre_block_out = 0;
      schedules_table schs_tbl( _self, _self.value );
      auto sch = schs_tbl.find( uint64_t( schedule_version ) );
      if( sch == schs_tbl.end() ) {
         reward_block( curr_block_num,bpname,schedule_version,true );

         schs_tbl.emplace( eosforce::system_account, [&]( schedule_info& s ) {
            s.version = schedule_version;
            s.block_height = curr_block_num;
            for( int i = 0; i < NUM_OF_TOP_BPS; i++ ) {
               schedule_info::producer temp_producer {
                  block_producers[i].value,
                  block_producers[i] == name{bpname} ? 1u : 0u
               };
               s.producers.push_back(temp_producer);
            }
         });
      } else {
         for( int i = 0; i < NUM_OF_TOP_BPS; i++ ) {
            if( sch->producers[i].bpname == bpname ) {
               pre_block_out = sch->producers[i].amount;
               break;
            }
         }
      }

      uint32_t bp_last_amount = 0;
      auto monitor_bp = _bpmonitors.find(bpname);
      if ( monitor_bp != _bpmonitors.end() ) {
         bp_last_amount = monitor_bp->last_block_num;
      }


      if (pre_block_out > bp_last_amount && pre_block_out - bp_last_amount >= BP_CYCLE_BLOCK_OUT) {
         reward_block(curr_block_num,bpname,schedule_version,false);
      }

      const auto current_time_sec = time_point_sec( current_time_point() );

      // make cache for vote state
      make_global_votestate( curr_block_num );

      // producer a block is also make a heartbeat
      heartbeat_imp( bpname, current_time_sec );

      // reward bps
      reward_bps( block_producers, curr_block_num, current_time_sec );

      if( curr_block_num % UPDATE_CYCLE == 0 ) {
         update_elected_bps();
      }

      if (sch != schs_tbl.end()) {
         schs_tbl.modify( sch, name{0}, [&]( schedule_info& s ) {
            for( int i = 0; i < NUM_OF_TOP_BPS; i++ ) {
               if( s.producers[i].bpname == bpname ) {
                  s.producers[i].amount += 1;
                  break;
               }
            }
         });
      }
   }

   void system_contract::update_elected_bps() {
      constexpr auto bps_top_size = static_cast<size_t>( NUM_OF_TOP_BPS );

      std::vector<std::pair<producer_key, int64_t>> vote_schedule;
      vote_schedule.reserve( 32 );

      // Note this output is not same after updated
      // TODO: use table sorted datas
      for( const auto& it : _bps ) {
         if( is_producer_in_blacklist( it.name ) ) {
            continue;
         }

         if ( is_producer_in_punished(it.name) ) {
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
      hb_table hb_tbl( _self, _self.value );

      const auto& global_votestate = get_global_votestate( curr_block_num );
      const auto& staked_all_bps = global_votestate.total_staked;

      if( staked_all_bps <= 0 ) {
         return;
      }
      // 0.5% of staked_all_bps
      //const auto rewarding_bp_staked_threshold = staked_all_bps / 200;
      const auto rewarding_bp_staked_threshold = INCOME_MIN_SHAKE;

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
      bpreward_table bprewad_tbl( _self, _self.value );
      for( auto it = _bps.cbegin(); it != _bps.cend(); ++it ) {
         if(    is_producer_in_blacklist( it->name )
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

         pledges bp_pledge(eosforce::pledge_account,it->name);
         auto pledge = bp_pledge.find(eosforce::block_out_pledge.value);
         if ( (pledge == bp_pledge.end() || pledge->pledge.amount <= BASE_BLOCK_OUT_PLEDGE)  && (is_func_open(eosforce::bp_punish)) ) {
            continue;
         }

         if ( is_producer_in_punished(it->name) ) {
            continue;
         }

         const auto bp_reward = static_cast<int64_t>( BLOCK_REWARDS_BP * double( it->total_staked ) /double( staked_all_bps ) );

         auto bp_reward_info = bprewad_tbl.find(it->name);
         if ( bp_reward_info == bprewad_tbl.end() ) {
            bprewad_tbl.emplace( get_self(), [&]( bps_reward& s ) {
               s.bpname = it->name;
               s.reward = asset(0,CORE_SYMBOL);
            } );
            bp_reward_info = bprewad_tbl.find(it->name);
         }

         auto bp_account_reward = bp_reward / 6 + bp_reward * 5 / 6 * it->commission_rate / 10000;
         bprewad_tbl.modify( bp_reward_info, name{0}, [&]( bps_reward& b ) { 
            b.reward += asset( bp_account_reward, CORE_SYMBOL ); 
         } );

         // reward pool
         const auto bp_rewards_pool = bp_reward * 5 / 6 * ( 10000 - it->commission_rate ) / 10000;
         const auto& bp = _bps.get( it->name, "bpname is not registered" );
         _bps.modify( bp, name{0}, [&]( bp_info& b ) { 
            b.rewards_pool += asset( bp_rewards_pool, CORE_SYMBOL ); 
         } );

         sum_bps_reward += ( bp_account_reward + bp_rewards_pool );
      }

      // reward eosfund
      const auto total_eosfund_reward = BLOCK_REWARDS_BP - sum_bps_reward;
      if( total_eosfund_reward > 0 ) {
         const auto& eosfund = _accounts.get( ( "devfund"_n ).value, "devfund is not found in accounts table" );
         _accounts.modify( eosfund, name{0}, [&]( account_info& a ) { 
            a.available += asset( total_eosfund_reward, CORE_SYMBOL ); 
         } );
      }
   }

   void system_contract::reward_block( const uint32_t curr_block_num,
                                       const account_name& bpname,
                                       const uint32_t schedule_version,
                                       const bool is_change_producers) {
      // if blockreward_table no record return ,if change producer add a record to blockreward_table
      blockreward_table br_tbl( get_self(), get_self().value );
      auto cblockreward = br_tbl.find( bp_reward_name.value );
      if ( cblockreward == br_tbl.end() ) {
         if ( is_change_producers ) {
            br_tbl.emplace( get_self(), [&]( block_reward& s ) {
               s.name = bp_reward_name.value;
               s.reward_block_out = asset(0, CORE_SYMBOL);
               s.reward_budget = asset(0, CORE_SYMBOL);
               s.last_standard_bp = bpname;
               s.total_block_age = 0;
               s.last_reward_block_num = curr_block_num;
            });
         }
         return ;
      }
      // if change producer ,use last version
      auto last_version = schedule_version;
      if ( is_change_producers ) {
         last_version -= 1;
      } 
      schedules_table schs_tbl( get_self(), get_self().value );
      auto sch = schs_tbl.find( uint64_t( last_version ) );
      //find the first and the last
      uint32_t ifirst = 0,ilast = 0;
      for( int i = 0; i < NUM_OF_TOP_BPS; i++ ) {
         if( sch->producers[i].bpname == bpname ) {
            ilast = i;
         }
         if (sch->producers[i].bpname == cblockreward->last_standard_bp) {
            ifirst = i;
         }
      }

      uint64_t total_bp_age = 0;
      for( int i = 0; i < NUM_OF_TOP_BPS; i++ ) {
         // if no monitor_bp record,add one
         auto monitor_bp = _bpmonitors.find( sch->producers[i].bpname );
         if ( monitor_bp == _bpmonitors.end() ) {
            _bpmonitors.emplace( get_self(), [&]( bp_monitor& s ) {
               s.bpname = sch->producers[i].bpname;
               s.last_block_num = 0;
               s.consecutive_drain_block = 0;
               s.consecutive_produce_block = 0;
               s.total_drain_block = 0;
               s.stability = BASE_BLOCK_OUT_WEIGHT;
               s.bock_age = 0;
               s.bp_status = BPSTATUS::NORMAL;
               s.end_punish_block = 0;
            });
            monitor_bp = _bpmonitors.find(sch->producers[i].bpname);
         }

         auto drain_num = monitor_bp->last_block_num + BP_CYCLE_BLOCK_OUT - sch->producers[i].amount;
         auto producer_num = BP_CYCLE_BLOCK_OUT - drain_num;
         // between first and last drain one block      
         if ( ifirst <= i && i < ilast ){  
            drain_num += 1;
         }
         else if ( ifirst > ilast && (ifirst <= i || i < ilast) ) {
            drain_num += 1;
         }
         // if change producer all producer do not drain one block
         if ( is_change_producers ) { drain_num -= 1; }

         if ( drain_num <= 0 && monitor_bp->consecutive_drain_block > 2 ) {
            drainblock_table drainblock_tbl( get_self(),sch->producers[i].bpname );
            drainblock_tbl.emplace( get_self(), [&]( drain_block_info& s ) { 
               s.current_block_num = static_cast<uint64_t>(curr_block_num);
               s.drain_block_num = monitor_bp->consecutive_drain_block;
            });
         }
         // if no pledge block reward will not get
         auto bp_age = producer_num * monitor_bp->stability;
         pledges bp_pledge(eosforce::pledge_account,sch->producers[i].bpname);
         auto pledge = bp_pledge.find(eosforce::block_out_pledge.value);
         if ( pledge == bp_pledge.end() || pledge->pledge.amount <= BASE_BLOCK_OUT_PLEDGE ) {
            bp_age = 0;
         }
         total_bp_age += bp_age;

         _bpmonitors.modify( monitor_bp, name{0}, [&]( bp_monitor& s ) {
            if (is_change_producers) {
               s.last_block_num = 0;
            }
            else {
               s.last_block_num = sch->producers[i].amount;
            }
            s.bock_age += bp_age;
            if (drain_num > 0) {
               s.consecutive_drain_block += drain_num;
               s.total_drain_block += drain_num;
               s.consecutive_produce_block = 0;
               s.stability = BASE_BLOCK_OUT_WEIGHT;
            }
            else {
               s.consecutive_drain_block = 0;
               s.consecutive_produce_block += producer_num;
            }
            // if drain block bigger then BP_PUBISH_DRAIN_NUM status is one
            if ( s.consecutive_drain_block > BP_PUBISH_DRAIN_NUM && s.bp_status == BPSTATUS::NORMAL) {
               s.bp_status = BPSTATUS::LACK_PLEDGE;
            }
            // if consecutive produce block is Multiple of BP_PUBISH_DRAIN_NUM stability add one
            if ( (s.consecutive_produce_block + 1) % BP_PUBISH_DRAIN_NUM == 0 && s.stability < BLOCK_OUT_WEIGHT_LIMIT) {
               s.stability += 1;
            }
         });
         // deduction pledge
         if ( drain_num > 0 ) {
            if ( pledge != bp_pledge.end() ) {
               pledge::deduction_action temp{ eosforce::pledge_account, {  {eosforce::system_account, eosforce::active_permission} } };
               temp.send( eosforce::block_out_pledge,sch->producers[i].bpname,asset(drain_num*DRAIN_BLOCK_PUNISH,CORE_SYMBOL),std::string("drain block punish")  );
            }
         }
      }
      // reward block out
      auto total_producer_num = curr_block_num - cblockreward->last_reward_block_num;
      br_tbl.modify( cblockreward, name{0}, [&]( block_reward& s ) {
         s.last_standard_bp = bpname;
         s.last_reward_block_num = curr_block_num;
         s.total_block_age += total_bp_age;
         s.reward_block_out += asset(total_producer_num * BLOCK_OUT_REWARD,CORE_SYMBOL);
      });
   }
} /// namespace eosio
