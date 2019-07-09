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

      bps_table bps_tbl( _self, _self.value );
      auto bp = bps_tbl.find( bpname );
      if( bp == bps_tbl.end() ) {
         bps_tbl.emplace( name{bpname}, [&]( bp_info& b ) {
            b.name = bpname;
            b.block_signing_key = block_signing_key;
            b.commission_rate = commission_rate;
            b.url = url;
         } );
      } else {
         bps_tbl.modify( bp, name{}, [&]( bp_info& b ) {
            b.block_signing_key = block_signing_key;
            b.commission_rate = commission_rate;
            b.url = url;
         } );
      }
   }

   void system_contract::setemergency( const account_name& bpname, const bool emergency ) {
      require_auth( name{bpname} );
      bps_table bps_tbl( _self, _self.value );
      const auto& bp = bps_tbl.get( bpname, "bpname is not registered" );

      cstatus_table cstatus_tbl( _self, _self.value );
      const auto& cstatus = cstatus_tbl.get( ("chainstatus"_n).value, "get chainstatus fatal" );

      bps_tbl.modify( bp, name{0}, [&]( bp_info& b ) { 
         b.emergency = emergency; 
      } );

      const auto& block_producers = get_active_producers();

      int proposal = 0;
      for( const auto& name : block_producers ) {
         const auto& b = bps_tbl.get( name.value, "setemergency: bpname is not registered" );
         proposal += b.emergency ? 1 : 0;
      }

      cstatus_tbl.modify( cstatus, name{0}, [&]( chain_status& cs ) { 
         cs.emergency = proposal > (NUM_OF_TOP_BPS * 2 / 3); 
      } );
   }

   void system_contract::heartbeat( const account_name& bpname, const time_point_sec& timestamp ) {
      bps_table bps_tbl( _self, _self.value );
      check( bps_tbl.find( bpname ) != bps_tbl.end(), "bpname is not registered" );
      heartbeat_imp( bpname, timestamp );
   }

   void system_contract::removebp( const account_name& bpname ) {
      require_auth( _self );

      blackproducer_table blackproducer( _self, _self.value );
      auto bp = blackproducer.find( bpname );
      if( bp == blackproducer.end() ) {
         blackproducer.emplace( name{bpname}, [&]( producer_blacklist& b ) {
            b.bpname = bpname;
            b.isactive = false;
         } );
      } else {
         blackproducer.modify( bp, name{0}, [&]( producer_blacklist& b ) { 
            b.isactive = false;
         } );
      }
   }

} // namespace eosio
