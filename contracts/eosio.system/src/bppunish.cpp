#include <utility>

#include <eosio.system.hpp>

#include <eosio/privileged.hpp>
#include <eosio/producer_schedule.hpp>
#include <../../eosio.pledge/include/eosio.pledge/eosio.pledge.hpp>

namespace eosio {
   void system_contract::punishbp( const account_name& bpname,const account_name& proposaler ) {
      require_auth( name{proposaler} );
      auto monitor_bp = _bpmonitors.find(bpname);
      check( (monitor_bp != _bpmonitors.end()) && (monitor_bp->bp_status == BPSTATUS::LACK_PLEDGE),"the bp can not to be punish" );

      const auto curr_block_num = current_block_num();
      punishbp_table pb_tbl( get_self(), get_self().value );
      auto punish_bp = pb_tbl.find(bpname);
      check( punish_bp == pb_tbl.end() || punish_bp->effective_block_num < curr_block_num,"the bp was Being resolved");
      
      pledges bp_pledge(eosforce::pledge_account,proposaler);
      auto pledge = bp_pledge.find(eosforce::block_out_pledge.value);
      check( pledge != bp_pledge.end() && pledge->pledge > asset(100*10000,CORE_SYMBOL),"the proposaler must pledge 100 EOS on block.out");

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

      auto itr = std::find_if( punish_bp->approve_bp.begin(), punish_bp->approve_bp.end(),
         [&](const account_name& a) { return a == approver; } );
      check( itr == punish_bp->approve_bp.end(), "the bp has approved" );

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
      auto monitor_bp = _bpmonitors.find(bpname);
      check( (monitor_bp != _bpmonitors.end()) && (monitor_bp->bp_status == BPSTATUS::PUNISHED) && (monitor_bp->end_punish_block < curr_block_num)
         ,"the bp can not bail" );

      _bpmonitors.modify(monitor_bp,name{0},[&]( bp_monitor& s ) { 
            s.bp_status = BPSTATUS::NORMAL;
            s.end_punish_block = 0;
         });
   }

   void system_contract::exec_punish_bp( const account_name &bpname ) {
      punishbp_table pb_tbl( get_self(), get_self().value );
      auto punish_bp = pb_tbl.find(bpname);
      const auto curr_block_num = current_block_num();
      check( punish_bp != pb_tbl.end() && punish_bp->effective_block_num > curr_block_num,"the bp was not Being resolved");

      int isize = punish_bp->approve_bp.size();
      int approve_bp_num = 0;
      for (const auto& approve_bp : punish_bp->approve_bp){
         if (is_super_bp(approve_bp)) {
            ++approve_bp_num;
         }
      }
      if (approve_bp_num >= APPROVE_TO_PUNISH_NUM) {
         auto monitor_bp = _bpmonitors.find(bpname);
         check( (monitor_bp != _bpmonitors.end()) && (monitor_bp->bp_status == BPSTATUS::LACK_PLEDGE),"the bp can not to be punish" );
         _bpmonitors.modify(monitor_bp,name{0},[&]( bp_monitor& s ) { 
            s.bp_status = BPSTATUS::PUNISHED;
            s.end_punish_block = curr_block_num + PUNISH_BP_LIMIT;
         });
         
         pledges bp_pledge(eosforce::pledge_account,bpname);
         auto pledge = bp_pledge.find(eosforce::block_out_pledge.value);
         if (pledge == bp_pledge.end() || pledge->deduction <= asset(0,CORE_SYMBOL) || pledge->pledge + pledge->deduction <= asset(0,CORE_SYMBOL)) {
            pb_tbl.erase(punish_bp);
            return ;
         }
         else {
            vector<account_name> reward_account;
            reward_account.push_back(punish_bp->proposaler);
            for (const auto& approve_bp : punish_bp->approve_bp){
               if ( is_super_bp(approve_bp) ) {
                  reward_account.push_back(approve_bp);
               }
            }
            pledge::dealreward_action temp{ eosforce::pledge_account, {  {eosforce::system_account, eosforce::active_permission} } };
            temp.send( eosforce::block_out_pledge,bpname,reward_account,std::string("") );
         }
         pb_tbl.erase(punish_bp);
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

   void system_contract::monitorevise( const account_name& bpname ) {

      require_auth( get_self() );
      auto monitor_bp = _bpmonitors.find(bpname);
      if ( monitor_bp != _bpmonitors.end() && monitor_bp->total_drain_block > WRONG_DRAIN_BLOCK ) {
         auto total_drain_num = drainblock_revise(monitor_bp->bpname);
         _bpmonitors.modify( monitor_bp, name{0}, [&]( bp_monitor& s ) {
            s.total_drain_block = total_drain_num;
            s.bp_status = BPSTATUS::NORMAL;
         });
      }
      else {
         for (auto iter = _bpmonitors.begin();iter != _bpmonitors.end();++iter) {
            if ( iter->total_drain_block > WRONG_DRAIN_BLOCK ) {
               auto total_drain_num = drainblock_revise(iter->bpname);
               _bpmonitors.modify( iter, name{0}, [&]( bp_monitor& s ) {
                  s.total_drain_block = total_drain_num;
                  s.bp_status = BPSTATUS::NORMAL;
               });
            }
         }
      }

   }

   int32_t system_contract::drainblock_revise(const account_name &bpname) {
      bool drain_block_wrong = true;
      int32_t result = 0;
      while(drain_block_wrong) {
            result = 0;
            drain_block_wrong = false;
            drainblock_table drainblock_tbl( get_self(),bpname );
            for (auto itor = drainblock_tbl.begin();itor != drainblock_tbl.end();++itor) {
               if ( itor->drain_block_num > WRONG_DRAIN_BLOCK ) {
                  drainblock_tbl.erase(itor);
                  drain_block_wrong = true;
                  break;
               }
               else {
                  result += itor->drain_block_num;
               }
            }
        }
      return result;
   }

   void system_contract::removepunish( const account_name& bpname ) {
      require_auth( name{bpname} );

      auto monitor_bp = _bpmonitors.find(bpname);
      check( monitor_bp != _bpmonitors.end(),"can not find monitor infomation" );
      check( monitor_bp->bp_status == BPSTATUS::LACK_PLEDGE,"the status of bp is not to be punish");

      // to be add some condition
      auto lastproducer_info = _lastproducers.find(bp_producer_name.value);
      check(lastproducer_info != _lastproducers.end(),"can not find lastproducer information");
      auto itr = std::find_if( lastproducer_info->producers.begin(), lastproducer_info->producers.end(), [&](const account_name& a) { return a == bpname; } );
      check( itr != lastproducer_info->producers.end(), "the bp is not in last producers" );

      _bpmonitors.modify( monitor_bp, name{0}, [&]( bp_monitor& s ) {
         s.bp_status = BPSTATUS::NORMAL;
      });
   }
}