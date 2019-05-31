#include <eosio.system.hpp>
#include <eosio/privileged.hpp>


namespace eosiosystem {
      ACTION system_contract::transfer( const account_name from, const account_name to, const asset quantity, const string memo ){
         require_auth(from);
         accounts_table acnts_tbl(_self, _self.value);
         const auto& from_act = acnts_tbl.get(from.value, "from account is not found in accounts table");
         const auto& to_act = acnts_tbl.get(to.value, "to account is not found in accounts table");

         check(quantity.symbol == CORE_SYMBOL, "only support EOS which has 4 precision");
         //from_act.available is already handling fee
         check(0 <= quantity.amount && quantity.amount <= from_act.available.amount,
                     "need 0.0000 EOS < quantity < available balance");
         check(memo.size() <= 256, "memo has more than 256 bytes");

         require_recipient(from);
         require_recipient(to);

         acnts_tbl.modify(from_act, _self, [&]( account_info& a ) {
            a.available -= quantity;
         });

         acnts_tbl.modify(to_act, _self, [&]( account_info& a ) {
            a.available += quantity;
         });
      }

      ACTION system_contract::updatebp( const account_name bpname, const public_key block_signing_key,
                     const uint32_t commission_rate, const std::string& url ){
         require_auth(bpname);
         check(url.size() < 64, "url too long");
         check(1 <= commission_rate && commission_rate <= 10000, "need 1 <= commission rate <= 10000");

         bps_table bps_tbl(_self, _self.value);
         auto bp = bps_tbl.find(bpname.value);
         if( bp == bps_tbl.end()) {
            bps_tbl.emplace(bpname, [&]( bp_info& b ) {
               b.name = bpname;
               b.update(block_signing_key, commission_rate, url);
            });
         } else {
            bps_tbl.modify(bp, bpname, [&]( bp_info& b ) {
               b.update(block_signing_key, commission_rate, url);
            });
         }
      }

      ACTION system_contract::vote( const account_name voter, const account_name bpname, const asset stake ){
         require_auth(voter);
         accounts_table acnts_tbl(_self, _self.value);
         const auto& act = acnts_tbl.get(voter.value, "voter is not found in accounts table");

         bps_table bps_tbl(_self, _self.value);
         const auto& bp = bps_tbl.get(bpname.value, "bpname is not registered");

         check(stake.symbol == CORE_SYMBOL, "only support EOS which has 4 precision");
         check(0 <= stake.amount && stake.amount % 10000 == 0,
                     "need stake quantity >= 0.0000 EOS and quantity is integer");

         int64_t change = 0;
         votes_table votes_tbl(_self, voter.value);
         auto vts = votes_tbl.find(bpname.value);
         if( vts == votes_tbl.end()) {
            change = stake.amount;
            //act.available is already handling fee
            check(stake.amount <= act.available.amount, "need stake quantity < your available balance");

            votes_tbl.emplace(voter, [&]( vote_info& v ) {
               v.bpname = bpname;
               v.staked = stake;
            });
         } else {
            change = stake.amount - vts->staked.amount;
            //act.available is already handling fee
            check(change <= act.available.amount, "need stake change quantity < your available balance");

            votes_tbl.modify(vts, voter, [&]( vote_info& v ) {
               v.voteage += v.staked.amount / 10000 * ( current_block_num() - v.voteage_update_height );
               v.voteage_update_height = current_block_num();
               v.staked = stake;
               if( change < 0 ) {
                  v.unstaking.amount += -change;
                  v.unstake_height = current_block_num();
               }
            });
         }

         blackproducer_table blackproducer(_self, _self.value);
         auto blackpro = blackproducer.find(bpname.value);
         check(blackpro == blackproducer.end() || blackpro->isactive || (!blackpro->isactive && change < 0), "bp is not active");
         if( change > 0 ) {
            acnts_tbl.modify(act, _self, [&]( account_info& a ) {
               a.available.amount -= change;
            });
         }
         
         bps_tbl.modify(bp, _self, [&]( bp_info& b ) {
            b.total_voteage += b.total_staked * ( current_block_num() - b.voteage_update_height );
            b.voteage_update_height = current_block_num();
            b.total_staked += change / 10000;
         });
      }

      ACTION system_contract::unfreeze( const account_name voter, const account_name bpname ){
         require_auth(voter);
         accounts_table acnts_tbl(_self, _self.value);
         const auto& act = acnts_tbl.get(voter.value, "voter is not found in accounts table");

         votes_table votes_tbl(_self, voter.value);
         const auto& vts = votes_tbl.get(bpname.value, "voter have not add votes to the the producer yet");

         check(vts.unstake_height + config::FROZEN_DELAY < current_block_num(), "unfreeze is not available yet");
         check(0 < vts.unstaking.amount, "need unstaking quantity > 0.0000 EOS");

         acnts_tbl.modify(act, _self, [&]( account_info& a ) {
            a.available += vts.unstaking;
         });

         votes_tbl.modify(vts, _self, [&]( vote_info& v ) {
            v.unstaking.set_amount(0);
         });
      }

      ACTION system_contract::vote4ram( const account_name voter, const account_name bpname, const asset stake ){
         require_auth(voter);
         accounts_table acnts_tbl(_self, _self.value);
         const auto& act = acnts_tbl.get(voter.value, "voter is not found in accounts table");

         bps_table bps_tbl(_self, _self.value);
         const auto& bp = bps_tbl.get(bpname.value, "bpname is not registered");

         check(stake.symbol == CORE_SYMBOL, "only support EOS which has 4 precision");
         check(0 <= stake.amount && stake.amount % 10000 == 0,
                     "need stake quantity >= 0.0000 EOS and quantity is integer");

         int64_t change = 0;
         votes4ram_table votes_tbl(_self, voter.value);
         auto vts = votes_tbl.find(bpname.value);
         if( vts == votes_tbl.end()) {
            change = stake.amount;
            //act.available is already handling fee
            check(stake.amount <= act.available.amount, "need stake quantity < your available balance");

            votes_tbl.emplace(voter, [&]( vote_info& v ) {
               v.bpname = bpname;
               v.staked = stake;
            });
         } else {
            change = stake.amount - vts->staked.amount;
            //act.available is already handling fee
            check(change <= act.available.amount, "need stake change quantity < your available balance");

            votes_tbl.modify(vts, voter, [&]( vote_info& v ) {
               v.voteage += v.staked.amount / 10000 * ( current_block_num() - v.voteage_update_height );
               v.voteage_update_height = current_block_num();
               v.staked = stake;
               if( change < 0 ) {
                  v.unstaking.amount += -change;
                  v.unstake_height = current_block_num();
               }
            });
         }
         blackproducer_table blackproducer(_self, _self.value);
         auto blackpro = blackproducer.find(bpname.value);
         check(blackpro == blackproducer.end() || blackpro->isactive || (!blackpro->isactive && change < 0), "bp is not active");
         if( change > 0 ) {
            acnts_tbl.modify(act, _self, [&]( account_info& a ) {
               a.available.amount -= change;
            });
         }

         bps_tbl.modify(bp, _self, [&]( bp_info& b ) {
            b.total_voteage += b.total_staked * ( current_block_num() - b.voteage_update_height );
            b.voteage_update_height = current_block_num();
            b.total_staked += change / 10000;
         });

         vote4ramsum_table vote4ramsum_tbl(_self, _self.value);
         auto vtss = vote4ramsum_tbl.find(voter.value);
         if(vtss == vote4ramsum_tbl.end()){
            vote4ramsum_tbl.emplace(voter, [&]( vote4ram_info& v ) {
               v.voter = voter;
               v.staked = stake; // for first vote all staked is stake
            });
         }else{
            vote4ramsum_tbl.modify(vtss, voter, [&]( vote4ram_info& v ) {
               v.staked += asset{change,CORE_SYMBOL};
            });
         }
//todo
         set_need_check_ram_limit( voter.value );
      }

      ACTION system_contract::unfreezeram( const account_name voter, const account_name bpname ){
         require_auth(voter);
         accounts_table acnts_tbl(_self, _self.value);
         const auto& act = acnts_tbl.get(voter.value, "voter is not found in accounts table");

         votes4ram_table votes_tbl(_self, voter.value);
         const auto& vts = votes_tbl.get(bpname.value, "voter have not add votes to the the producer yet");

         check(vts.unstake_height + config::FROZEN_DELAY < current_block_num(), "unfreeze is not available yet");
         check(0 < vts.unstaking.amount, "need unstaking quantity > 0.0000 EOS");

         acnts_tbl.modify(act, _self, [&]( account_info& a ) {
            a.available += vts.unstaking;
         });

         votes_tbl.modify(vts, _self, [&]( vote_info& v ) {
            v.unstaking.set_amount(0);
         });
      }

      ACTION system_contract::claim( const account_name voter, const account_name bpname ){
         require_auth(voter);
         accounts_table acnts_tbl(_self, _self.value);
         const auto& act = acnts_tbl.get(voter.value, "voter is not found in accounts table");

         bps_table bps_tbl(_self, _self.value);
         const auto& bp = bps_tbl.get(bpname.value, "bpname is not registered");

         votes_table votes_tbl(_self, voter.value);
         const auto& vts = votes_tbl.get(bpname.value, "voter have not add votes to the the producer yet");

         int64_t newest_voteage =
               vts.voteage + vts.staked.amount / 10000 * ( current_block_num() - vts.voteage_update_height );
         int64_t newest_total_voteage =
               bp.total_voteage + bp.total_staked * ( current_block_num() - bp.voteage_update_height );
         check(0 < newest_total_voteage, "claim is not available yet");

         int128_t amount_voteage = ( int128_t ) bp.rewards_pool.amount * ( int128_t ) newest_voteage;
         asset reward = asset(static_cast<int64_t>(( int128_t ) amount_voteage / ( int128_t ) newest_total_voteage ),
                              CORE_SYMBOL);
         check(0 <= reward.amount && reward.amount <= bp.rewards_pool.amount,
                     "need 0 <= claim reward quantity <= rewards_pool");

         acnts_tbl.modify(act, _self, [&]( account_info& a ) {
            a.available += reward;
         });

         votes_tbl.modify(vts, _self, [&]( vote_info& v ) {
            v.voteage = 0;
            v.voteage_update_height = current_block_num();
         });

         bps_tbl.modify(bp, _self, [&]( bp_info& b ) {
            b.rewards_pool -= reward;
            b.total_voteage = newest_total_voteage - newest_voteage;
            b.voteage_update_height = current_block_num();
         });
      }
      ACTION system_contract::onblock( const block_timestamp, const account_name bpname, const uint16_t,
         const block_id_type, const checksum256, const checksum256, const uint32_t schedule_version ){
         bps_table bps_tbl(_self, _self.value);
         accounts_table acnts_tbl(_self, _self.value);
         schedules_table schs_tbl(_self, _self.value);

         //account_name block_producers[config::NUM_OF_TOP_BPS] = {};
         auto block_producers = get_active_producers();

         auto sch = schs_tbl.find(uint64_t(schedule_version));
         if( sch == schs_tbl.end()) {
            schs_tbl.emplace(bpname, [&]( schedule_info& s ) {
               s.version = schedule_version;
               s.block_height = current_block_num();
               for( int i = 0; i < config::NUM_OF_TOP_BPS; i++ ) {
                  s.producers[i].amount = block_producers[i] == bpname ? 1 : 0;
                  s.producers[i].bpname = block_producers[i];
               }
            });
         } else {
            schs_tbl.modify(sch, _self, [&]( schedule_info& s ) {
               for( int i = 0; i < config::NUM_OF_TOP_BPS; i++ ) {
                  if( s.producers[i].bpname == bpname ) {
                     s.producers[i].amount += 1;
                     break;
                  }
               }
            });
         }
         heartbeat( bpname, time_point_sec(now() ));

         //reward bps
         reward_bps(block_producers);



         //update schedule
         if( current_block_num() % config::UPDATE_CYCLE == 0 ) {
            //reward block.one
            name b1_account = name("b1"_n);
            const auto& b1 = acnts_tbl.get(b1_account.value, "b1 is not found in accounts table");
            acnts_tbl.modify(b1, _self, [&]( account_info& a ) {
               a.available += asset(BLOCK_REWARDS_B1 * config::UPDATE_CYCLE,CORE_SYMBOL);
            });

            update_elected_bps();
         }
      }


      ACTION system_contract::onfee( const account_name actor, const asset fee, const account_name bpname ){
         accounts_table acnts_tbl(_self, _self.value);
         const auto& act = acnts_tbl.get(actor.value, "account is not found in accounts table");
         eosio::check(fee.amount <= act.available.amount, "overdrawn available balance");

         bps_table bps_tbl(_self, _self.value);
         const auto& bp = bps_tbl.get(bpname.value, "bpname is not registered");

         acnts_tbl.modify(act, _self, [&]( account_info& a ) {
            a.available -= fee;
         });

         bps_tbl.modify(bp, _self, [&]( bp_info& b ) {
            b.rewards_pool += fee;
         });
      }

      ACTION system_contract::setemergency( const account_name bpname, const bool emergency ){
         require_auth(bpname);
         bps_table bps_tbl(_self, _self.value);
         const auto& bp = bps_tbl.get(bpname.value, "bpname is not registered");

         cstatus_table cstatus_tbl(_self, _self.value);
         auto chainstatus = name("chainstatus"_n);
         const auto& cstatus = cstatus_tbl.get(chainstatus.value, "get chainstatus fatal");

         bps_tbl.modify(bp, _self, [&]( bp_info& b ) {
            b.emergency = emergency;
         });

        // account_name block_producers[config::NUM_OF_TOP_BPS] = {};
         auto block_producers = get_active_producers();

         int proposal = 0;
         for( auto name : block_producers ) {
            const auto& b = bps_tbl.get(name.value, "setemergency: bpname is not registered");
            proposal += b.emergency ? 1 : 0;
         }

         cstatus_tbl.modify(cstatus, _self, [&]( chain_status& cs ) {
            cs.emergency = proposal > config::NUM_OF_TOP_BPS * 2 / 3;
         });
      }

      ACTION system_contract::heartbeat( const account_name bpname, const time_point_sec timestamp ){
         bps_table bps_tbl(_self, _self.value);
         hb_table hb_tbl(_self, _self.value);
         const auto& bp = bps_tbl.get(bpname.value, "bpname is not registered");

         auto hb = hb_tbl.find(bpname.value);
         if( hb == hb_tbl.end()) {
            hb_tbl.emplace(bpname, [&]( heartbeat_info& hb ) {
               hb.bpname = bpname;
               hb.timestamp = timestamp;
            });
         } else {
            hb_tbl.modify(hb, _self, [&]( heartbeat_info& hb ) {
               hb.timestamp = timestamp;
            });
         }
      }

      ACTION system_contract::removebp( account_name bpname ){
         require_auth(_self);

         blackproducer_table blackproducer(_self, _self.value);
         auto bp = blackproducer.find(bpname.value);
         if( bp == blackproducer.end()) {
         blackproducer.emplace(bpname, [&]( producer_blacklist& b ) {
               b.bpname = bpname;
               b.deactivate();
            });
         } else {
            blackproducer.modify(bp, _self, [&]( producer_blacklist& b ) {
               b.deactivate();
            });
         }
      }

      void system_contract::update_elected_bps(){
         bps_table bps_tbl(_self, _self.value);

         std::vector<eosio::producer_key> vote_schedule;
         std::vector<int64_t> sorts(config::NUM_OF_TOP_BPS, 0);

         for( auto it = bps_tbl.cbegin(); it != bps_tbl.cend(); ++it ) {
            for( int i = 0; i < config::NUM_OF_TOP_BPS; ++i ) {
               blackproducer_table blackproducer(_self, _self.value);
               auto blackpro = blackproducer.find(it->name.value);

               if( sorts[size_t(i)] <= it->total_staked && (blackpro == blackproducer.end() || blackpro->isactive)) {
                  eosio::producer_key key;
                  key.producer_name = it->name;
                  key.block_signing_key = it->block_signing_key;
                  vote_schedule.insert(vote_schedule.begin() + i, key);
                  sorts.insert(sorts.begin() + i, it->total_staked);
                  break;
               }
            }
         }

         if( vote_schedule.size() > config::NUM_OF_TOP_BPS ) {
            vote_schedule.resize(config::NUM_OF_TOP_BPS);
         }

         /// sort by producer name
         std::sort(vote_schedule.begin(), vote_schedule.end());
         set_proposed_producers(vote_schedule);
      }
      void system_contract::reward_bps( vector<account_name> block_producers ){
         bps_table bps_tbl(_self, _self.value);
         accounts_table acnts_tbl(_self, _self.value);
         schedules_table schs_tbl(_self, _self.value);
         hb_table hb_tbl(_self, _self.value);

         //calculate total staked all of the bps
         int64_t staked_all_bps = 0;
         for( auto it = bps_tbl.cbegin(); it != bps_tbl.cend(); ++it ) {
            staked_all_bps += it->total_staked;
         }
         if( staked_all_bps <= 0 ) {
            return;
         }
         //0.5% of staked_all_bps
         const auto rewarding_bp_staked_threshold = staked_all_bps / 200;

         //reward bps, (bp_reward => bp_account_reward + bp_rewards_pool + eosfund_reward;)
         auto sum_bps_reward = 0;
         for( auto it = bps_tbl.cbegin(); it != bps_tbl.cend(); ++it ) {
            blackproducer_table blackproducer(_self, _self.value);
            auto blackpro = blackproducer.find(it->name.value);
            if(( blackpro != blackproducer.end() && !blackpro->isactive) || it->total_staked <= rewarding_bp_staked_threshold || it->commission_rate < 1 ||
               it->commission_rate > 10000 ) {
               continue;
            }
            
            auto hbmax = name("hb.max"_n);
            int64_t hb_max = get_num_config_on_chain(hbmax.value);
            if (hb_max == -1) hb_max = 3600;
            auto hb = hb_tbl.find(it->name.value);
            if( hb == hb_tbl.end() || hb->timestamp + hb_max < time_point_sec( now() ) ) {
               continue;
            }

            auto bp_reward = static_cast<int64_t>( BLOCK_REWARDS_BP * double(it->total_staked) / double(staked_all_bps));

            //reward bp account
            auto bp_account_reward = bp_reward * 15 / 100 + bp_reward * 70 / 100 * it->commission_rate / 10000;
            if( is_super_bp(block_producers, it->name)) {
               bp_account_reward += bp_reward * 15 / 100;
            }
            const auto& act = acnts_tbl.get(it->name.value, "bpname is not found in accounts table");
            acnts_tbl.modify(act, it->name, [&]( account_info& a ) {
               a.available += asset(bp_account_reward, CORE_SYMBOL);
            });

            //reward pool
            auto bp_rewards_pool = bp_reward * 70 / 100 * ( 10000 - it->commission_rate ) / 10000;
            const auto& bp = bps_tbl.get(it->name.value, "bpname is not registered");
            bps_tbl.modify(bp, it->name, [&]( bp_info& b ) {
               b.rewards_pool += asset(bp_rewards_pool, CORE_SYMBOL);
            });

            sum_bps_reward += ( bp_account_reward + bp_rewards_pool );
         }

         //reward eosfund
         account_name devfund = name("devfund"_n);
         const auto& eosfund = acnts_tbl.get(devfund.value, "devfund is not found in accounts table");
         auto total_eosfund_reward = BLOCK_REWARDS_BP - sum_bps_reward;
         acnts_tbl.modify(eosfund, devfund, [&]( account_info& a ) {
            a.available += asset(total_eosfund_reward, CORE_SYMBOL);
         });
      }
      bool system_contract::is_super_bp( vector<account_name> block_producers, account_name name ){
         for( int i = 0; i < config::NUM_OF_TOP_BPS; i++ ) {
            if( name == block_producers[i] ) {
               return true;
            }
         }
         return false;
      }

}

EOSIO_DISPATCH( eosiosystem::system_contract,(transfer)(updatebp)(vote)(unfreeze)(vote4ram)(unfreezeram)(claim)(onblock)(onfee)(setemergency)(heartbeat)(removebp) )