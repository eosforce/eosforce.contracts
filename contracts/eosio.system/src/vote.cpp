#include <utility>

#include <eosio.system.hpp>

#include <eosio/privileged.hpp>

namespace eosio {

   void system_contract::revote( const account_name& voter,
                                 const account_name& frombp,
                                 const account_name& tobp,
                                 const asset& restake ) {
      require_auth( name{voter} );
      check( frombp != tobp, " from and to cannot same" );
      check( restake.symbol == CORE_SYMBOL, "only support EOS which has 4 precision" );
      check( restake.amount > 0 && ( restake.amount % 10000 == 0 ),
            "need restake quantity > 0.0000 EOS and quantity is integer" );

      bps_table bps_tbl( _self, _self.value );
      const auto& bpf = bps_tbl.get( frombp, "bpname is not registered" );
      const auto& bpt = bps_tbl.get( tobp, "bpname is not registered" );

      const auto curr_block_num = current_block_num();

      // votes_table from bp
      votes_table votes_tbl( _self, voter );
      auto vtsf = votes_tbl.find( frombp );
      check( vtsf != votes_tbl.end(), "no vote on this bp" );
      check( restake <= vtsf->staked, "need restake <= frombp stake" );
      votes_tbl.modify( vtsf, name{0}, [&]( vote_info& v ) {
         v.voteage += v.staked.amount / 10000 * ( curr_block_num - v.voteage_update_height );
         v.voteage_update_height = curr_block_num;
         v.staked -= restake;
      } );

      // votes_table to bp
      auto vtst = votes_tbl.find( tobp );
      if( vtst == votes_tbl.end() ) {
         votes_tbl.emplace( name{voter}, [&]( vote_info& v ) {
            v.bpname = tobp;
            v.staked = restake;
         } );
      } else {
         votes_tbl.modify( vtst, name{0}, [&]( vote_info& v ) {
            v.voteage += v.staked.amount / 10000 * ( curr_block_num - v.voteage_update_height );
            v.voteage_update_height = curr_block_num;
            v.staked += restake;
         } );
      }

      bps_tbl.modify( bpf, name{0}, [&]( bp_info& b ) {
         b.total_voteage += b.total_staked * ( curr_block_num - b.voteage_update_height );
         b.voteage_update_height = curr_block_num;
         b.total_staked -= restake.amount / 10000;
      } );

      bps_tbl.modify( bpt, name{0}, [&]( bp_info& b ) {
         b.total_voteage += b.total_staked * ( curr_block_num - b.voteage_update_height );
         b.voteage_update_height = curr_block_num;
         b.total_staked += restake.amount / 10000;
      } );
   }

   void system_contract::vote( const account_name& voter,
                               const account_name& bpname,
                               const asset& stake ) {
      require_auth( name{voter} );
      accounts_table acnts_tbl( _self, _self.value );
      const auto& act = acnts_tbl.get( voter, "voter is not found in accounts table" );

      bps_table bps_tbl( _self, _self.value );
      const auto& bp = bps_tbl.get( bpname, "bpname is not registered" );

      check( stake.symbol == CORE_SYMBOL, "only support EOS which has 4 precision" );
      check( 0 <= stake.amount && stake.amount % 10000 == 0,
             "need stake quantity >= 0.0000 EOS and quantity is integer" );

      const auto curr_block_num = current_block_num();

      int64_t change = 0;
      votes_table votes_tbl( _self, voter );
      auto vts = votes_tbl.find( bpname );
      if( vts == votes_tbl.end() ) {
         change = stake.amount;
         // act.available is already handling fee
         check( stake.amount <= act.available.amount, "need stake quantity < your available balance" );

         votes_tbl.emplace( name{voter}, [&]( vote_info& v ) {
            v.bpname = bpname;
            v.staked = stake;
         } );
      } else {
         change = stake.amount - vts->staked.amount;
         // act.available is already handling fee
         check( change <= act.available.amount,
                "need stake change quantity < your available balance" );

         votes_tbl.modify( vts, name{0}, [&]( vote_info& v ) {
            v.voteage += v.staked.amount / 10000 * ( curr_block_num - v.voteage_update_height );
            v.voteage_update_height = curr_block_num;
            v.staked = stake;
            if( change < 0 ) {
               v.unstaking.amount += -change;
               v.unstake_height = curr_block_num;
            }
         } );
      }

      blackproducer_table blackproducer( _self, _self.value );
      auto blackpro = blackproducer.find( bpname );
      check(    blackpro == blackproducer.end() 
             || blackpro->isactive 
             || ( !blackpro->isactive && change < 0 ), "bp is not active" );

      if( change > 0 ) {
         acnts_tbl.modify( act, name{0}, [&]( account_info& a ) { 
            a.available.amount -= change; 
         } );
      }

      bps_tbl.modify( bp, name{0}, [&]( bp_info& b ) {
         b.total_voteage += b.total_staked * ( curr_block_num - b.voteage_update_height );
         b.voteage_update_height = curr_block_num;
         b.total_staked += ( change / 10000 );
      } );
   }

   void system_contract::unfreeze( const account_name& voter, const account_name& bpname ) {
      require_auth( name{voter} );
      accounts_table acnts_tbl( _self, _self.value );
      const auto& act = acnts_tbl.get( voter, "voter is not found in accounts table" );

      votes_table votes_tbl( _self, voter );
      const auto& vts = votes_tbl.get( bpname, "voter have not add votes to the the producer yet" );

      check( vts.unstake_height + FROZEN_DELAY < current_block_num(),
            "unfreeze is not available yet" );
      check( 0 < vts.unstaking.amount, "need unstaking quantity > 0.0000 EOS" );

      acnts_tbl.modify( act, name{0}, [&]( account_info& a ) { 
         a.available += vts.unstaking; 
      } );

      votes_tbl.modify( vts, name{0}, [&]( vote_info& v ) { 
         v.unstaking.set_amount( 0 ); 
      } );
   }

   void system_contract::vote4ram( const account_name& voter,
                                   const account_name& bpname,
                                   const asset& stake ) {
      require_auth( name{voter} );
      accounts_table acnts_tbl( _self, _self.value );
      const auto& act = acnts_tbl.get( voter, "voter is not found in accounts table" );

      bps_table bps_tbl( _self, _self.value );
      const auto& bp = bps_tbl.get( bpname, "bpname is not registered" );

      check( stake.symbol == CORE_SYMBOL, "only support EOS which has 4 precision" );
      check( 0 <= stake.amount && stake.amount % 10000 == 0,
            "need stake quantity >= 0.0000 EOS and quantity is integer" );

      const auto curr_block_num = current_block_num();

      int64_t change = 0;
      votes4ram_table votes_tbl( _self, voter );
      auto vts = votes_tbl.find( bpname );
      if( vts == votes_tbl.end() ) {
         change = stake.amount;
         // act.available is already handling fee
         check( stake.amount <= act.available.amount, "need stake quantity < your available balance" );

         votes_tbl.emplace( name{voter}, [&]( vote_info& v ) {
            v.bpname = bpname;
            v.staked = stake;
         } );
      } else {
         change = stake.amount - vts->staked.amount;
         // act.available is already handling fee
         check( change <= act.available.amount,
               "need stake change quantity < your available balance" );

         votes_tbl.modify( vts, name{0}, [&]( vote_info& v ) {
            v.voteage += v.staked.amount / 10000 * ( curr_block_num - v.voteage_update_height );
            v.voteage_update_height = curr_block_num;
            v.staked = stake;
            if( change < 0 ) {
               v.unstaking.amount += -change;
               v.unstake_height = curr_block_num;
            }
         } );
      }
      blackproducer_table blackproducer( _self, _self.value );
      auto blackpro = blackproducer.find( bpname );
      check(    blackpro == blackproducer.end() 
             || blackpro->isactive 
             || ( !blackpro->isactive && change < 0 ), "bp is not active" );

      if( change > 0 ) {
         acnts_tbl.modify( act, name{0}, [&]( account_info& a ) { 
            a.available.amount -= change; 
         } );
      }

      bps_tbl.modify( bp, name{0}, [&]( bp_info& b ) {
         b.total_voteage += b.total_staked * ( curr_block_num - b.voteage_update_height );
         b.voteage_update_height = curr_block_num;
         b.total_staked += change / 10000;
      } );

      vote4ramsum_table vote4ramsum_tbl( _self, _self.value );
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
      require_auth( name{voter} );
      accounts_table acnts_tbl( _self, _self.value );
      const auto& act = acnts_tbl.get( voter, "voter is not found in accounts table" );

      votes4ram_table votes_tbl( _self, voter );
      const auto& vts = votes_tbl.get( bpname, "voter have not add votes to the the producer yet" );

      check( vts.unstake_height + FROZEN_DELAY < current_block_num(),
            "unfreeze is not available yet" );
      check( 0 < vts.unstaking.amount, "need unstaking quantity > 0.0000 EOS" );

      acnts_tbl.modify( act, name{0}, [&]( account_info& a ) { 
         a.available += vts.unstaking; 
      } );

      votes_tbl.modify( vts, name{0}, [&]( vote_info& v ) { 
         v.unstaking.set_amount( 0 ); 
      } );
   }

   void system_contract::claim( const account_name& voter, const account_name& bpname ) {
      require_auth( name{voter} );

      const auto curr_block_num = current_block_num();

      accounts_table acnts_tbl( _self, _self.value );
      const auto& act = acnts_tbl.get( voter, "voter is not found in accounts table" );

      bps_table bps_tbl( _self, _self.value );
      const auto& bp = bps_tbl.get( bpname, "bpname is not registered" );

      votes_table votes_tbl( _self, voter );
      const auto& vts = votes_tbl.get( bpname, "voter have not add votes to the the producer yet" );

      const int64_t newest_voteage = vts.voteage + vts.staked.amount / 10000 * ( curr_block_num - vts.voteage_update_height );
      const int64_t newest_total_voteage = bp.total_voteage + bp.total_staked * ( curr_block_num - bp.voteage_update_height );
      check( 0 < newest_total_voteage, "claim is not available yet" );

      int128_t amount_voteage = (int128_t)bp.rewards_pool.amount * (int128_t)newest_voteage;
      const auto& reward = asset( static_cast<int64_t>( (int128_t)amount_voteage / (int128_t)newest_total_voteage ),CORE_SYMBOL );
      check( 0 <= reward.amount && reward.amount <= bp.rewards_pool.amount,
            "need 0 <= claim reward quantity <= rewards_pool" );

      acnts_tbl.modify( act, name{0}, [&]( account_info& a ) { 
         a.available += reward; 
      } );

      votes_tbl.modify( vts, name{0}, [&]( vote_info& v ) {
         v.voteage = 0;
         v.voteage_update_height = curr_block_num;
      } );

      bps_tbl.modify( bp, name{0}, [&]( bp_info& b ) {
         b.rewards_pool -= reward;
         b.total_voteage = newest_total_voteage - newest_voteage;
         b.voteage_update_height = curr_block_num;
      } );
   }

} // namespace eosio
