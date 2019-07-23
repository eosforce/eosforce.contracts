#include <utility>

#include <eosio.system.hpp>

#include <eosio/privileged.hpp>

namespace eosio {

   template< typename VOTE_TYP >
   int64_t system_contract::vote_by_typ_imp( const account_name& voter,
                                             const account_name& bpname,
                                             const asset& stake ) {
      require_auth( name{voter} );
      const auto& act = _accounts.get( voter, "voter is not found in accounts table" );
      const auto& bp = _bps.get( bpname, "bpname is not registered" );

      check( stake.symbol == CORE_SYMBOL, "only support EOS which has 4 precision" );
      check( 0 <= stake.amount && stake.amount % CORE_SYMBOL_PRECISION == 0,
             "need stake quantity >= 0.0000 EOS and quantity is integer" );

      const auto curr_block_num = current_block_num();

      int64_t change = 0;
      VOTE_TYP votes_tbl( get_self(), voter );
      auto vts = votes_tbl.find( bpname );
      if( vts == votes_tbl.end() ) {
         change = stake.amount;
         // act.available is already handling fee
         check( stake.amount <= act.available.amount, "need stake quantity < your available balance" );

         votes_tbl.emplace( name{voter}, [&]( vote_info& v ) {
            v.bpname = bpname;
            v.voteage.staked = stake;
         } );
      } else {
         change = stake.amount - vts->voteage.staked.amount;
         // act.available is already handling fee
         check( change <= act.available.amount,
                "need stake change quantity < your available balance" );

         votes_tbl.modify( vts, name{0}, [&]( vote_info& v ) {
            v.voteage.change_staked_to( curr_block_num, stake );
            if( change < 0 ) {
               v.unstaking.amount += -change;
               v.unstake_height = curr_block_num;
            }
         } );
      }

      if( change == 0 ) {
         // if no change vote staked, just return and no error
         return change;
      }

      if( change > 0 ) {
         check( !is_producer_in_blacklist( bpname ),
                "bp is not active, cannot add stake for vote" );
         _accounts.modify( act, name{0}, [&]( account_info& a ) { 
            a.available.amount -= change; 
         } );
      }

      _bps.modify( bp, name{0}, [&]( bp_info& b ) {
         b.add_total_staked( curr_block_num, change );
      } );

      on_change_total_staked( curr_block_num, change );

      return change;
   }


   void system_contract::revote( const account_name& voter,
                                 const account_name& frombp,
                                 const account_name& tobp,
                                 const asset& restake ) {
      require_auth( name{voter} );
      check( frombp != tobp, " from and to cannot same" );
      check( restake.symbol == CORE_SYMBOL, "only support EOS which has 4 precision" );
      check( restake.amount > 0 && ( restake.amount % CORE_SYMBOL_PRECISION == 0 ),
            "need restake quantity > 0.0000 EOS and quantity is integer" );

      const auto& bpf = _bps.get( frombp, "bpname is not registered" );
      const auto& bpt = _bps.get( tobp, "bpname is not registered" );

      const auto curr_block_num = current_block_num();

      // votes_table from bp
      votes_table votes_tbl( _self, voter );
      auto vtsf = votes_tbl.find( frombp );
      check( vtsf != votes_tbl.end(), "no vote on this bp" );
      check( restake <= vtsf->voteage.staked, "need restake <= frombp stake" );
      votes_tbl.modify( vtsf, name{0}, [&]( vote_info& v ) {
         v.voteage.minus_staked_by( curr_block_num, restake );
      } );

      // votes_table to bp
      auto vtst = votes_tbl.find( tobp );
      if( vtst == votes_tbl.end() ) {
         votes_tbl.emplace( name{voter}, [&]( vote_info& v ) {
            v.bpname = tobp;
            v.voteage.staked = restake;
         } );
      } else {
         votes_tbl.modify( vtst, name{0}, [&]( vote_info& v ) {
            v.voteage.add_staked_by( curr_block_num, restake );
         } );
      }

      _bps.modify( bpf, name{0}, [&]( bp_info& b ) {
         b.add_total_staked( curr_block_num, -restake );
      } );

      _bps.modify( bpt, name{0}, [&]( bp_info& b ) {
         b.add_total_staked( curr_block_num, restake );
      } );

      // no change total staked so no need on_change_total_staked
   }

   template< typename VOTE_TYP >
   void system_contract::unfreeze_by_typ_imp( const account_name& voter,
                                              const account_name& bpname ) {
      require_auth( name{voter} );

      const auto& act = _accounts.get( voter, "voter is not found in accounts table" );

      VOTE_TYP votes_tbl( get_self(), voter );
      const auto& vts = votes_tbl.get( bpname, "voter have not add votes to the the producer yet" );

      check( vts.unstake_height + FROZEN_DELAY < current_block_num(),
            "unfreeze is not available yet" );
      check( 0 < vts.unstaking.amount, "need unstaking quantity > 0.0000 EOS" );

      _accounts.modify( act, name{0}, [&]( account_info& a ) { 
         a.available += vts.unstaking; 
      } );

      votes_tbl.modify( vts, name{0}, [&]( vote_info& v ) { 
         v.unstaking.set_amount( 0 ); 
      } );
   }


   void system_contract::vote( const account_name& voter,
                               const account_name& bpname,
                               const asset& stake ) {
      vote_by_typ_imp<votes_table>( voter, bpname, stake );
   }

   void system_contract::unfreeze( const account_name& voter, const account_name& bpname ) {
      unfreeze_by_typ_imp<votes_table>( voter, bpname );
   }

   void system_contract::vote4ram( const account_name& voter,
                                   const account_name& bpname,
                                   const asset& stake ) {
      const auto change = vote_by_typ_imp<votes4ram_table>( voter, bpname, stake );

      vote4ramsum_table vote4ramsum_tbl( get_self(), get_self().value );
      auto vtss = vote4ramsum_tbl.find( voter );
      if( vtss == vote4ramsum_tbl.end() ) {
         vote4ramsum_tbl.emplace( name{voter}, [&]( vote4ram_info& v ) {
            v.voter = voter;
            v.staked = stake; // for first vote all staked is stake
         } );
      } else {
         vote4ramsum_tbl.modify( vtss, name{0}, [&]( vote4ram_info& v ) { 
            v.staked += asset{ change, CORE_SYMBOL }; 
         } );
      }

      set_need_check_ram_limit( name{voter} );
   }

   void system_contract::unfreezeram( const account_name& voter, const account_name& bpname ) {
      unfreeze_by_typ_imp<votes4ram_table>( voter, bpname );
   }

   void system_contract::claim( const account_name& voter, const account_name& bpname ) {
      require_auth( name{voter} );

      const auto curr_block_num = current_block_num();

      const auto& act = _accounts.get( voter, "voter is not found in accounts table" );

      const auto& bp = _bps.get( bpname, "bpname is not registered" );

      votes_table votes_tbl( _self, voter );
      const auto& vts = votes_tbl.get( bpname, "voter have not add votes to the the producer yet" );

      const int64_t newest_voteage = vts.voteage.get_age( curr_block_num );
      const int64_t newest_total_voteage = bp.get_age( curr_block_num );
      check( 0 < newest_total_voteage, "claim is not available yet" );

      int128_t amount_voteage = (int128_t)bp.rewards_pool.amount * (int128_t)newest_voteage;
      const auto& reward = asset( static_cast<int64_t>( (int128_t)amount_voteage / (int128_t)newest_total_voteage ),CORE_SYMBOL );
      check( 0 <= reward.amount && reward.amount <= bp.rewards_pool.amount,
            "need 0 <= claim reward quantity <= rewards_pool" );

      _accounts.modify( act, name{0}, [&]( account_info& a ) { 
         a.available += reward; 
      } );

      votes_tbl.modify( vts, name{0}, [&]( vote_info& v ) {
         v.voteage.clean_age( curr_block_num );
      } );

      _bps.modify( bp, name{0}, [&]( bp_info& b ) {
         b.rewards_pool -= reward;
         b.total_voteage = newest_total_voteage - newest_voteage;
         b.voteage_update_height = curr_block_num;
      } );
   }

   // make a fix-time vote by voter to bpname with stake token
   // type is fix-time type, 
   void system_contract::votefix( const account_name& voter,
                                  const account_name& bpname,
                                  const name& type,
                                  const asset& stake ) {
      require_auth( name{voter} );

      // All fix-time voting is new
      const auto& vote_data = fix_time_vote_info::get_data_by_typ( type );
      check( static_cast<bool>(vote_data), "fix time vote type error" );

      const auto& freeze_block_num = std::get<1>( *vote_data ) * BLOCK_NUM_PER_DAY;
      const auto& vote_power = std::get<2>( *vote_data );

      //print_f( "vote fix % % by % %\n", 
      //         name{voter}, name{bpname}, freeze_block_num, vote_power );

      const auto& act = _accounts.get( voter, "voter is not found in accounts table" );
      const auto& bp = _bps.get( bpname, "bpname is not registered" );

      check( stake.symbol == CORE_SYMBOL, "only support CORE SYMBOL token" );
      check( 0 < stake.amount && stake.amount % 10000 == 0,
             "need stake quantity >= 0 and quantity is integer" );
      check( stake <= act.available, "need stake quantity <= your available balance" );

      const auto curr_block_num = current_block_num();

      check( !is_producer_in_blacklist( bpname ),
             "bp is not active, cannot add stake for vote" );

      const auto vote_stake_by_power = stake * vote_power;

      // Add fix vote data
      fix_time_votes_table fix_time_votes_tbl( get_self(), voter );
      fix_time_votes_tbl.emplace( get_self(), [&]( fix_time_vote_info& fvi ) { 
         fvi.key                  = fix_time_votes_tbl.available_primary_key();
         fvi.voter                = voter;
         fvi.fvote_typ            = type;
         fvi.start_block_num      = curr_block_num;
         fvi.withdraw_block_num   = curr_block_num + freeze_block_num;
         fvi.vote                 = stake;
         fvi.votepower_age.staked = vote_stake_by_power;
      } );

      // modify account token
      _accounts.modify( act, name{0}, [&]( account_info& a ) { 
         a.available -= stake; 
      } );

      _bps.modify( bp, name{0}, [&]( bp_info& b ) {
         b.add_total_staked( curr_block_num, vote_stake_by_power.amount );
      } );

      on_change_total_staked( curr_block_num, vote_stake_by_power.amount );
   }

   void system_contract::revotefix( const account_name& voter,
                                    const uint64_t& key,
                                    const account_name& bpname ) {
      require_auth( name{voter} );
   }

   // take out stake to a fix-time vote by voter after vote is timeout
   void system_contract::outfixvote( const account_name& voter,
                                     const uint64_t& key ) {
      require_auth( name{voter} );
   }



} // namespace eosio
