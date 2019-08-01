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

   void system_contract::approvebp( const account_name& bpname,const account_name& approver ) {
      require_auth( name{approver} );
      check( is_super_bp(approver),"only bp can approve punish bp" );
      const auto curr_block_num = current_block_num();
      punishbp_table pb_tbl( get_self(), get_self().value );
      auto punish_bp = pb_tbl.find(bpname);
      check( punish_bp != pb_tbl.end() && punish_bp->effective_block_num > curr_block_num,"the bp was not Being resolved");

      pb_tbl.modify(punish_bp,name{0},[&]( punish_bp_info& s ) { 
         s.approve_bp.push_back(approver);
      });

      if (punish_bp->approve_bp.size() >= 16) {
         exec_punish_bp(bpname);
      }

   }

   void system_contract::bailpunish( const account_name& bpname ) {
      require_auth( name{bpname} );
      const auto curr_block_num = current_block_num();
      bpmonitor_table bpm_tbl( get_self(), get_self().value );
      auto monitor_bp = bpm_tbl.find(bpname);
      check( (monitor_bp != bpm_tbl.end()) && (monitor_bp->bp_status == 2) && (monitor_bp->end_punish_block > curr_block_num)
         ,"the bp can not bail" );
      bpm_tbl.modify(monitor_bp,name{0},[&]( bp_monitor& s ) { 
            s.bp_status = 0;
            s.end_punish_block = 0;
         });
   }

   void system_contract::exec_punish_bp( const account_name &bpname ) {
      punishbp_table pb_tbl( get_self(), get_self().value );
      auto punish_bp = pb_tbl.find(bpname);
      const auto curr_block_num = current_block_num();
      if ( punish_bp == pb_tbl.end() || punish_bp->effective_block_num < curr_block_num ) {
         return ;
      }
      int isize = punish_bp->approve_bp.size();
      int approve_bp_num = 0;
      for (int i = 0; i != isize; ++i ){
         if (is_super_bp(punish_bp->approve_bp[i])) {
            ++approve_bp_num;
         }
      }
      if (approve_bp_num > 16) {
         //开始惩罚
         bpmonitor_table bpm_tbl( get_self(), get_self().value );
         auto monitor_bp = bpm_tbl.find(bpname);
         check( (monitor_bp != bpm_tbl.end()) && (monitor_bp->bp_status == 1),"the bp can not to be punish" );
         bpm_tbl.modify(monitor_bp,name{0},[&]( bp_monitor& s ) { 
            s.bp_status = 2;
            s.end_punish_block = curr_block_num + PUNISH_BP_LIMIT;
         });
      }
   }

   bool system_contract::is_super_bp( const account_name &bpname ) const {
      schedules_table schs_tbl( _self, _self.value );
      auto sch_last = schs_tbl.crbegin();
      auto isize = sch_last->producers.size();
      for (int i = 0; i != isize; ++i) {
         if (bpname == sch_last->producers[i].bpname) {
            return true;
         }
      }
      return false;
   }
}