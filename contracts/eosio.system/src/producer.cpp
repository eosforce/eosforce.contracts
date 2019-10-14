#include <utility>

#include <eosio.system.hpp>

#include <eosio/privileged.hpp>

namespace eosio {

   void system_contract::updatebp( const account_name& bpname,
                                   const public_key& block_signing_key,
                                   const uint32_t commission_rate,
                                   const std::string& url ) {
      require_auth( name{bpname} );
      check( url.size() < 64, "url too long" );
      check( 1 <= commission_rate && commission_rate <= 10000, "need 1 <= commission rate <= 10000" );

      auto bp = _bps.find( bpname );
      if( bp == _bps.end() ) {
         _bps.emplace( name{bpname}, [&]( bp_info& b ) {
            b.name = bpname;
            b.block_signing_key = block_signing_key;
            b.commission_rate = commission_rate;
            b.url = url;
         } );
      } else {
         _bps.modify( bp, name{}, [&]( bp_info& b ) {
            b.block_signing_key = block_signing_key;
            b.commission_rate = commission_rate;
            b.url = url;
         } );
      }
   }

   void system_contract::setemergency( const account_name& bpname, const bool emergency ) {
      require_auth( name{bpname} );

      const auto& bp = _bps.get( bpname, "bpname is not registered" );

      cstatus_table cstatus_tbl( _self, _self.value );
      const auto& cstatus = cstatus_tbl.get( chainstatus_name.value, "get chain status fatal" );

      _bps.modify( bp, name{0}, [&]( bp_info& b ) { 
         b.emergency = emergency; 
      } );

      const auto& block_producers = get_active_producers();

      int proposal = 0;
      for( const auto& name : block_producers ) {
         const auto& b = _bps.get( name.value, "setemergency: bpname is not registered" );
         proposal += b.emergency ? 1 : 0;
      }

      cstatus_tbl.modify( cstatus, name{0}, [&]( chain_status& cs ) { 
         cs.emergency = proposal > (NUM_OF_TOP_BPS * 2 / 3); 
      } );
   }

   void system_contract::heartbeat( const account_name& bpname, const time_point_sec& timestamp ) {
      require_auth( name{bpname} );

      check( _bps.find( bpname ) != _bps.end(), "bpname is not registered" );

      const auto current_time_sec = time_point_sec( current_time_point() );

      const auto diff_time = current_time_sec.sec_since_epoch() - timestamp.sec_since_epoch();
      // TODO: use diff_time to make a more precise time

      heartbeat_imp( bpname, current_block_num(), current_time_sec );
   }

   void system_contract::removebp( const account_name& bpname ) {
      require_auth( _self );

      auto bp = _blackproducers.find( bpname );
      if( bp == _blackproducers.end() ) {
         _blackproducers.emplace( name{bpname}, [&]( producer_blacklist& b ) {
            b.bpname = bpname;
            b.isactive = false;
         } );
      } else {
         _blackproducers.modify( bp, name{0}, [&]( producer_blacklist& b ) { 
            b.isactive = false;
         } );
      }
   }

   void system_contract::bpclaim( const account_name& bpname ) {
      require_auth( name{bpname} );
      const auto& act = _accounts.get( bpname, "bpname is not found in accounts table" );

      auto reward_block = asset(0,CORE_SYMBOL);
      blockreward_table br_tbl( get_self(), get_self().value );
      auto cblockreward = br_tbl.find( bp_reward_name.value );
      auto monitor_bp = _bpmonitors.find( bpname );
      if ( cblockreward != br_tbl.end() && monitor_bp != _bpmonitors.end() ) {
         int128_t reward_amount = static_cast<int128_t>(monitor_bp->bock_age) * static_cast<int128_t>(cblockreward->reward_block_out.amount) / static_cast<int128_t>(cblockreward->total_block_age);
         reward_block = asset(static_cast<int64_t>(reward_amount),CORE_SYMBOL);
         //reward_block = monitor_bp->bock_age * cblockreward->reward_block_out / cblockreward->total_block_age;
         check( reward_block < cblockreward->reward_block_out,"need reward_block < total_block_out");

         br_tbl.modify( cblockreward, name{0}, [&]( block_reward& s ) { 
            s.total_block_age -= monitor_bp->bock_age;
            s.reward_block_out -= reward_block;
         } );

         _bpmonitors.modify( monitor_bp, name{0}, [&]( bp_monitor& s ) {
            s.bock_age = 0;
          } );
      }

      auto reward_bp = asset(0,CORE_SYMBOL);
      bpreward_table bprewad_tbl( _self, _self.value );
      auto bp_reward_info = bprewad_tbl.find(bpname);
      if ( bp_reward_info != bprewad_tbl.end() ) {
         reward_bp = bp_reward_info->reward;

         bprewad_tbl.modify( bp_reward_info, name{0}, [&]( bps_reward& b ) { 
            b.reward = asset( 0, CORE_SYMBOL ); 
         } );
      }

      auto total_reward = reward_bp + reward_block;
      check( MIN_CLAIM_BP < total_reward.amount,"need 100 EOSC < reward" );

      _accounts.modify( act, name{0}, [&]( account_info& a ) { 
         a.available += total_reward; 
      } );

   }

} // namespace eosio
