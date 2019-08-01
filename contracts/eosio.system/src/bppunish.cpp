#include <utility>

#include <eosio.system.hpp>

#include <eosio/privileged.hpp>
#include <eosio/producer_schedule.hpp>
#include <../../eosio.pledge/include/eosio.pledge/eosio.pledge.hpp>

namespace eosio {
   void system_contract::punishbp( const account_name& bpname,const account_name& proposaler ) {
      require_auth( name{proposaler} );
      bpmonitor_table bpm_tbl( get_self(), get_self().value );
      auto monitor_bp = bpm_tbl.find(bpname);
      check( (monitor_bp != bpm_tbl.end()) && (monitor_bp->bp_status == 1),"the bp can not to be punish" );

      const auto curr_block_num = current_block_num();
      punishbp_table pb_tbl( get_self(), get_self().value );
      auto punish_bp = pb_tbl.find(bpname);
      check( punish_bp == pb_tbl.end() || punish_bp->effective_block_num < curr_block_num,"the bp was Being resolved");
      // 校验是否有抵押

      if ( punish_bp != pb_tbl.end() ) {
         pb_tbl.erase(punish_bp);
      }

      pb_tbl.emplace( eosforce::system_account, [&]( punish_bp_info& s ) { 
         s.punish_bp_name = bpname;
         s.proposaler = proposaler;
         s.effective_block_num = curr_block_num + PUNISH_BP_LIMIT;
      });
   }

}