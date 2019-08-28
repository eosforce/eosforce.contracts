#include <eosio.budget/eosio.budget.hpp>
#include <../../eosio.system/include/eosio.system.hpp>
#include <../../eosio.pledge/include/eosio.pledge/eosio.pledge.hpp>

namespace eosio {

   budget::budget( name s, name code, datastream<const char*> ds ) 
   : contract( s, code, ds )
      {}

   budget::~budget() {}


   void budget::handover( vector<account_name> committeers ,string memo) {
      require_auth( get_self() );
      committee_table  committee_tbl( get_self(), get_self().value );
      auto committee = committee_tbl.find( EOSIO_BUDGET.value );

      if ( committee == committee_tbl.end() ) {
         committee_tbl.emplace( _self, [&]( auto& s ) { 
            s.member.assign(committeers.begin(), committeers.end());
         } );
      }
      else {
         committee_tbl.modify(committee, name{}, [&]( auto& s ) { 
            s.member.clear();
            s.member.assign(committeers.begin(), committeers.end());
         } );
      }
      
   }

   void budget::propose( account_name proposer,string title,string content,asset quantity,uint32_t end_num ) {
      require_auth( name{proposer} );

      pledges bp_pledge(eosforce::pledge_account,proposer);
      auto pledge = bp_pledge.find(eosforce::block_out_pledge.value);
      check( pledge != bp_pledge.end(),"the proposer do not have pledge on block.out" );
      check( pledge->pledge.amount > MIN_BUDGET_PLEDGE,"the pledge of proposer is not bigger then 100.0000 EOSC" );

      motion_table motion_tbl( get_self(), get_self().value );

      auto currnet_block = current_block_num();
      auto id_temp = motion_tbl.available_primary_key();
      motion_tbl.emplace( name{proposer} ,[&]( auto& s ) { 
         s.id = id_temp;
         s.root_id = 0;
         s.title = title;
         s.content = content;
         s.quantity = quantity;
         s.proposer = proposer;
         s.section = 0;
         s.takecoin_num = 0;
         s.approve_end_block_num = currnet_block + APPROVE_BLOCK_NUM;
         s.end_block_num = end_num;
         s.extern_data.clear();
      });

      committee_table  committee_tbl( get_self(), get_self().value );
      auto committee = committee_tbl.find( EOSIO_BUDGET.value );
      check( committee != committee_tbl.end(),"the committee do not exist" );

      approver_table approve_tbl( get_self(), proposer );
      approve_tbl.emplace( name{proposer} ,[&]( auto& s ) { 
         s.id = id_temp;
         s.requested.assign( committee->member.begin() , committee->member.end() );
      });

   }

   void budget::approve( account_name approver,uint64_t id ,string memo) {
      require_auth( name{approver} );
      auto currnet_block = current_block_num();
      motion_table motion_tbl( get_self(), get_self().value );
      auto montion = motion_tbl.find( id );
      check( montion != motion_tbl.end(), "no motion find");
      check( montion->approve_end_block_num > currnet_block,"the motion has exceeded the approve deadline" );
      check( montion->section == 0, "the section of motion is not Pending review");
      
      approver_table approve_tbl( get_self(), montion->proposer );
      auto approve_info = approve_tbl.find( id );
      check(approve_info != approve_tbl.end(),"no approve find");

      auto itr = std::find_if( approve_info->requested.begin(), approve_info->requested.end(), [&](const account_name& a) { return a == approver; } );
      check( itr != approve_info->requested.end(), "approval is not on the list of requested approvals" );

      approve_tbl.modify( approve_info, name{}, [&]( auto& a ) {
         a.approved.push_back( approver );
         a.requested.erase( itr );
      });

      auto isize = approve_info->requested.size() + approve_info->approved.size() + approve_info->unapproved.size();
      if ( approve_info->approved.size() > (isize * 2 / 3) ) {
         motion_tbl.modify( montion, name{}, [&]( auto& a ) { 
            a.section = 1;
         });
      }
   }

   void budget::unapprove( account_name approver,uint64_t id ,string memo) {
      require_auth( name{approver} );
      auto currnet_block = current_block_num();
      motion_table motion_tbl( get_self(), get_self().value );
      auto montion = motion_tbl.find( id );
      check( montion != motion_tbl.end(), "no motion find");
      check( montion->approve_end_block_num > currnet_block,"the motion has exceeded the approve deadline" );
      

      approver_table approve_tbl( get_self(), montion->proposer );
      auto approve_info = approve_tbl.find( id );
      check(approve_info != approve_tbl.end(),"no approve find");

      auto itr = std::find_if( approve_info->requested.begin(), approve_info->requested.end(), [&](const account_name& a) { return a == approver; } );
      check( itr != approve_info->requested.end(), "approval is not on the list of requested approvals" );

      approve_tbl.modify( approve_info, name{}, [&]( auto& a ) {
         a.unapproved.push_back( approver );
         a.requested.erase( itr );
      });

      auto isize = approve_info->requested.size() + approve_info->approved.size() + approve_info->unapproved.size();
      if ( approve_info->unapproved.size() > (isize  / 3) ) {
         motion_tbl.modify( montion, name{}, [&]( auto& a ) { 
            a.section = 2;
         });
      }
   }

   void budget::takecoin( account_name proposer,uint64_t montion_id,string content,asset quantity ) {
      require_auth( name{proposer} );
      pledges bp_pledge(eosforce::pledge_account,proposer);
      auto pledge = bp_pledge.find(eosforce::block_out_pledge.value);
      check( pledge != bp_pledge.end(),"the proposer do not have pledge on block.out" );
      check( pledge->pledge.amount > MIN_BUDGET_PLEDGE,"the pledge of proposer is not bigger then 100.0000 EOSC" );

      auto currnet_block = current_block_num();
      motion_table motion_tbl( get_self(), get_self().value );
      auto montion = motion_tbl.find( montion_id );
      check( montion != motion_tbl.end(), "no motion find");
      check( montion->end_block_num > currnet_block,"the motion has exceeded the approve deadline" );
      check( montion->approve_end_block_num < currnet_block,"The motion has not passed the publicity period" );
      check( montion->section == 1,"the motion section is not passed" );
      check( montion->proposer == proposer,"the takecoin proposer must be the motion proposer" );
      check( montion->quantity.symbol == quantity.symbol,"the symbol should be the same with motion quantity symbol");
      check( montion->quantity >= quantity,"the quantity must not be bigger then montion quantity");

      committee_table  committee_tbl( get_self(), get_self().value );
      auto committee = committee_tbl.find( EOSIO_BUDGET.value );
      check( committee != committee_tbl.end(),"the committee do not exist" );

      takecoin_table takecoin_tbl(get_self(),proposer);
      takecoin_tbl.emplace( name{proposer} ,[&]( auto& s ) { 
         s.id = takecoin_tbl.available_primary_key();
         s.montion_id = montion_id;
         s.content = content;
         s.quantity = quantity;
         s.receiver = proposer;
         s.end_block_num = currnet_block + APPROVE_BLOCK_NUM;
         s.requested.assign( committee->member.begin() , committee->member.end() );
         s.section = 0; 
      });

   }

   void budget::agreecoin( account_name approver,account_name proposer,uint64_t id ,string memo) {
      require_auth( name{approver} );
      auto currnet_block = current_block_num();
      takecoin_table takecoin_tbl(get_self(),proposer);
      auto takecoin_info = takecoin_tbl.find( id );
      check(takecoin_info != takecoin_tbl.end(),"take coin motion not find");
      check(takecoin_info->end_block_num > currnet_block,"the motion has exceeded the approve deadline");
      check(takecoin_info->section == 0,"the motion section is not propose");

      auto itr = std::find_if( takecoin_info->requested.begin(), takecoin_info->requested.end(), [&](const account_name& a) { return a == approver; } );
      check( itr != takecoin_info->requested.end(), "approval is not on the list of requested approvals" );

      takecoin_tbl.modify( takecoin_info, name{}, [&]( auto& a ) { 
         a.approved.push_back( approver );
         a.requested.erase( itr );
      });

      auto isize = takecoin_info->requested.size() + takecoin_info->approved.size() + takecoin_info->unapproved.size();
      if ( takecoin_info->approved.size() > (isize * 2 / 3) ) {
         motion_table motion_tbl( get_self(), get_self().value );
         auto montion = motion_tbl.find( takecoin_info->montion_id );
         check( montion != motion_tbl.end(),"the montion do not exist");
         check( montion->quantity >= takecoin_info->quantity,"the take coin quantity is bigger then motion quantity");
         check( montion->section == 1,"the montion section is not passed");
         auto takecoin_sec = takecoin_tbl.find( id );
         takecoin_tbl.modify( takecoin_sec, name{}, [&]( auto& a ) { 
            a.section = 1;
         });

         motion_tbl.modify( montion, name{}, [&]( auto& a ) { 
            a.quantity -= takecoin_info->quantity;
         });

         transfer_action temp { 
            eosforce::system_account, 
            {  { get_self(), eosforce::active_permission } } 
         };
         temp.send( get_self().value, takecoin_info->receiver, takecoin_info->quantity, std::string( "budget take coin" ) );
      }

   }

   void budget::unagreecoin( account_name approver,account_name proposer,uint64_t id ,string memo) {
      require_auth( name{approver} );
      auto currnet_block = current_block_num();
      takecoin_table takecoin_tbl(get_self(),proposer);
      auto takecoin_info = takecoin_tbl.find( id );
      check(takecoin_info != takecoin_tbl.end(),"take coin motion not find");
      check(takecoin_info->end_block_num > currnet_block,"the motion has exceeded the approve deadline");
      check(takecoin_info->section == 0,"the motion section is not propose");

      auto itr = std::find_if( takecoin_info->requested.begin(), takecoin_info->requested.end(), [&](const account_name& a) { return a == approver; } );
      check( itr != takecoin_info->requested.end(), "approval is not on the list of requested approvals" );

      takecoin_tbl.modify( takecoin_info, name{}, [&]( auto& a ) { 
         a.unapproved.push_back( approver );
         a.requested.erase( itr );
      });

      auto isize = takecoin_info->requested.size() + takecoin_info->approved.size() + takecoin_info->unapproved.size();
      if ( takecoin_info->unapproved.size() > (isize / 3) ) {
         auto takecoin_sec = takecoin_tbl.find( id );
         takecoin_tbl.modify( takecoin_sec, name{}, [&]( auto& a ) { 
            a.section = 2;
         });
      }
   }

   void budget::turndown( uint64_t id ,string memo) {
      require_auth( get_self() );
      motion_table motion_tbl( get_self(), get_self().value );
      auto montion = motion_tbl.find( id );
      check( montion != motion_tbl.end(),"the montion do not exist");

      motion_tbl.modify( montion, name{}, [&]( auto& a ) { 
            a.section = 3;
         });
   }



} /// namespace eosio
