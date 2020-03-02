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
      bool is_change_sch = false;
      schedules_table schs_tbl( _self, _self.value );
      auto sch = schs_tbl.find( uint64_t( schedule_version ) );
      if( sch == schs_tbl.end() ) {
         is_change_sch = true;

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

      if ( is_reward_block(is_change_sch,pre_block_out,bpname) ) {
         reward_block( curr_block_num, bpname, schedule_version, is_change_sch );
      }

      const auto current_time_sec = time_point_sec( current_time_point() );

      // make cache for vote state
      make_global_votestate( curr_block_num );

      // producer a block is also make a heartbeat
      heartbeat_imp( bpname, curr_block_num, current_time_sec );

      // reward bps
      reward_bps( block_producers, curr_block_num, current_time_sec );

      const auto open_update_block_num = get_num_config_on_chain( "update.bp"_n );
      if( curr_block_num % UPDATE_CYCLE == 0 ) {
         // open_update_block_num not zero, it will update bps beforce open_update_block_num
         if ( open_update_block_num < 0
           || open_update_block_num <= static_cast<int64_t>(curr_block_num) ) {
            update_elected_bps();
         }
      }

      auto lastproducer_info = _lastproducers.find(bp_producer_name.value);
      if (lastproducer_info == _lastproducers.end()) {
         _lastproducers.emplace( eosforce::system_account, [&]( auto& s ) { 
            s.name = bp_producer_name.value;
            s.max_size = MAX_LAST_PRODUCER_SIZE;
            s.next_index = 1;
            s.producers.resize(MAX_LAST_PRODUCER_SIZE);
            s.producers[0] = bpname;
         } );
      }
      else {
         _lastproducers.modify( lastproducer_info, name{0}, [&]( auto& s ) { 
            s.producers[s.next_index] = bpname;
            ++s.next_index;
            s.next_index %= MAX_LAST_PRODUCER_SIZE;
         } );
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
      const auto rewarding_bp_staked_threshold = staked_all_bps / BLOCK_REWARD_VOTER;

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
         if(    (hb == hb_tbl.end() 
             || ( hb->timestamp + static_cast<uint32_t>( hb_max ) ) < current_time_sec) && (is_func_open(eosforce::chain_func::heartbeat)) ) {
            continue;
         }

         pledges bp_pledge(eosforce::pledge_account,it->name);
         auto pledge = bp_pledge.find(eosforce::block_out_pledge.value);
         if ( (pledge == bp_pledge.end() || pledge->pledge.amount <= BASE_BLOCK_OUT_PLEDGE)  && (is_func_open(eosforce::chain_func::bp_punish)) ) {
            continue;
         }

         if ( is_producer_in_punished(it->name) ) {
            continue;
         }

         const auto voter_reward = static_cast<int64_t>( BLOCK_REWARD_VOTER * double( it->total_staked ) /double( staked_all_bps ) );
         const auto bp_reward = static_cast<int64_t>( BLOCK_REWARD_BPS_VOTER * double( it->total_staked ) /double( staked_all_bps ) );

         auto bp_account_reward = bp_reward + voter_reward * it->commission_rate / 10000;

         bpreward_table bprewad_tbl( _self, _self.value );
         auto bp_reward_info = bprewad_tbl.find(it->name);
         if ( bp_reward_info == bprewad_tbl.end() ) {
            bprewad_tbl.emplace( get_self(), [&]( auto& s ) { 
               s.bpname = it->name;
               s.reward = asset( bp_account_reward, CORE_SYMBOL );
            } );
         }
         else {
            bprewad_tbl.modify( bp_reward_info,name{}, [&]( auto& s ) { 
               s.reward += asset( bp_account_reward, CORE_SYMBOL );
            } );
         }

         // reward pool
         const auto bp_rewards_pool = voter_reward * ( 10000 - it->commission_rate ) / 10000;
         const auto& bp = _bps.get( it->name, "bpname is not registered" );
         _bps.modify( bp, name{0}, [&]( bp_info& b ) { 
            b.rewards_pool += asset( bp_rewards_pool, CORE_SYMBOL ); 
         } );

         sum_bps_reward += ( bp_account_reward + bp_rewards_pool );
      }

      // reward budget_account
      const auto total_eosbudget_reward = BLOCK_REWARD_VOTER + BLOCK_REWARD_BPS_VOTER - sum_bps_reward + BLOCK_BUDGET_REWARD;
      if( total_eosbudget_reward > 0 ) {
         const auto& eosbudget = _accounts.get( eosforce::budget_account.value, "devfund is not found in accounts table" );
         _accounts.modify( eosbudget, name{0}, [&]( account_info& a ) { 
            a.available += asset( total_eosbudget_reward, CORE_SYMBOL ); 
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

      auto blockout_weight_limit = BLOCK_OUT_WEIGHT_LIMIT;
      auto config_info = _systemconfig.find(CONFIG_BLOCK_OUT_WEIGHT_LIMIT.value);
      if ( config_info != _systemconfig.end() ) {
         blockout_weight_limit = config_info->number_value;
      }

      auto reset_block_out_weight_num = 1;
      auto config_reset_info = _systemconfig.find(CONFIG_RESET_BLOCK_WEIGHT_NUM.value);
      if ( config_reset_info != _systemconfig.end() ) {
         reset_block_out_weight_num = config_reset_info->number_value;
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

         int64_t drain_num = cal_drain_num(is_change_producers,i,ifirst,ilast,monitor_bp->last_block_num,sch->producers[i].amount);
         auto producer_num = sch->producers[i].amount - monitor_bp->last_block_num; 

         if ( drain_num <= 0 && monitor_bp->consecutive_drain_block > reset_block_out_weight_num ) {
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
               //s.stability = BASE_BLOCK_OUT_WEIGHT;
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
            if ( (s.consecutive_produce_block + 1) % BP_PUBISH_DRAIN_NUM == 0 && s.stability < blockout_weight_limit) {
               s.stability += 1;
            }

            if (s.consecutive_drain_block > reset_block_out_weight_num) {
               s.stability = BASE_BLOCK_OUT_WEIGHT;
            }

            if ( s.stability > blockout_weight_limit ) {
               s.stability = blockout_weight_limit;
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

   bool system_contract::is_reward_block(const bool &is_change_sch,const uint32_t &block_amount,const account_name &bpname) {
      bool result = false;
      if ( is_change_sch ) {
         result = true;
         return result;
      }

      uint32_t bp_last_amount = 0;
      auto monitor_bp = _bpmonitors.find(bpname);
      if ( monitor_bp != _bpmonitors.end() ) {
         bp_last_amount = monitor_bp->last_block_num;
      }

      if ( block_amount > bp_last_amount && block_amount - bp_last_amount >= BP_CYCLE_BLOCK_OUT ) {
         result = true;
      }
      return result;
   }

   int32_t system_contract::cal_drain_num(const bool &is_change_sch,const uint32_t index,const uint32_t &ifirst,const uint32_t &ilast,const uint32_t &pre_block_amount,const uint32_t &current_block_amount) {
      int32_t result = 0;
      result = pre_block_amount + BP_CYCLE_BLOCK_OUT - current_block_amount;
       // between first and last drain one block      
      if ( ifirst <= index && index < ilast ){  
         result += 1;
      }
      else if ( ifirst > ilast && (ifirst <= index || index < ilast) ) {
         result += 1;
      }
      // if change producer all producer do not drain one block
      if ( is_change_sch && ifirst != ilast ) { result -= 1; }

      if (result > BP_CYCLE_BLOCK_OUT) { result = BP_CYCLE_BLOCK_OUT; }
      if (result < 0) { result = 0; }
      return result;
   }
} /// namespace eosio
