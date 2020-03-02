#include <eosio.freeze/eosio.freeze.hpp>

namespace eosio {

   static constexpr eosio::name system_account{"eosio"_n};

   // addfreezed add freezed account to freeze account table by committer
   void lock_token::addfreezed( const account_name& committer,
                                const std::vector<account_name>& freezed_accounts,
                                const std::string& memo ) {
      require_auth( name{committer} );

      check( freezed_accounts.size() <= 256, "freezed account size need <= 256" );

      global_freezed_state gfstate( get_self(), get_self().value );
      check( ((!gfstate.exists()) || (gfstate.get().committer == 0)),
             "freezed table has activited" );

      freezed_stat freezed_stat_tbl( get_self(), get_self().value );
      auto fstat = freezed_stat_tbl.find( committer );
      if( fstat == freezed_stat_tbl.end() ) {
         freezed_stat_tbl.emplace( name{committer}, [&]( freezed_table_state& v ) {
            v.committer = committer;
         } );
      } else {
         check( fstat->state == static_cast<uint32_t>(freezed_table_stat_t::committing),
                "freezed state should be committing" );
      }

      freezeds freeze_tbl( get_self(), committer );
      uint32_t added = 0;
      for( const auto& f : freezed_accounts ) {
         if ( freeze_tbl.find( f ) == freeze_tbl.end() ) {
            freeze_tbl.emplace( name{committer}, [&]( freezed& v ) {
               v.account = f;
            } );
            added++;
         }
      }

      auto fstat_up = freezed_stat_tbl.find( committer );
      if( fstat_up == freezed_stat_tbl.end() ){
         freezed_stat_tbl.emplace( name{committer}, [&]( freezed_table_state& fs ) {
            fs.committer = committer;
            fs.last_commit_block_num = current_block_num();
            fs.freezed_size = added;
         } );
      }else{
         freezed_stat_tbl.modify( fstat_up, name{committer}, [&]( freezed_table_state& fs ) {
            fs.last_commit_block_num = current_block_num();
            fs.freezed_size += added;
         } );
      }

      return;
   }

   // delfreezed del freezed account to freeze account table by committer
   void lock_token::delfreezed( const account_name& committer,
                                const account_name& del_acc,
                                const std::string& memo ) {
      require_auth( name{committer} );

      global_freezed_state gfstate( get_self(), get_self().value );
      check( ((!gfstate.exists()) || (gfstate.get().committer == 0)),
             "freezed table has activited" );

      freezed_stat freezed_stat_tbl( get_self(), get_self().value );
      auto fstat = freezed_stat_tbl.find( committer );
      if( fstat == freezed_stat_tbl.end() ) {
         freezed_stat_tbl.emplace( name{committer}, [&]( freezed_table_state& v ) {
            v.committer = committer;
         } );
      } else {
         check( fstat->state == static_cast<uint32_t>(freezed_table_stat_t::committing),
                "freezed state should be committing" );
      }

      freezeds freeze_tbl( get_self(), committer );

      auto del_acc_itr = freeze_tbl.find( del_acc );
      check( del_acc_itr != freeze_tbl.end(), "del account not in table" );
      freeze_tbl.erase(del_acc_itr);

      freezed_stat_tbl.modify( fstat, name{committer}, [&]( freezed_table_state& fs ) {
         fs.last_commit_block_num = current_block_num();
         fs.freezed_size -= 1;
      } );
      return;
   }

   // lockfreezed lock freeze account table by committer
   void lock_token::lockfreezed( const account_name& committer, const bool is_locked ) {
      if( is_locked ) {
         // lock table just need committer sign
         require_auth( name{committer} );
      } else {
         // unlock need bps multisig
         require_auth( name{system_account} );
      }

      global_freezed_state gfstate( get_self(), get_self().value );
      check( ((!gfstate.exists()) || (gfstate.get().committer == 0)),
             "freezed table has activited" );

      freezed_stat freezed_stat_tbl( get_self(), get_self().value );
      auto fstat = freezed_stat_tbl.find( committer );
      check( fstat != freezed_stat_tbl.end(), "committer table no found" );

      check( fstat->state != static_cast<uint32_t>(freezed_table_stat_t::actived), "table has actived" );
      check( fstat->state != static_cast<uint32_t>(freezed_table_stat_t::banned), "table has banned" );

      const auto is_has_locked = fstat->state != static_cast<uint32_t>(freezed_table_stat_t::committing);
      check( is_has_locked != is_locked, "no chang state" );

      freezed_stat_tbl.modify( fstat, name{committer}, [&]( freezed_table_state& fs ) {
         fs.state = is_locked ? static_cast<uint32_t>(freezed_table_stat_t::locked)
                              : static_cast<uint32_t>(freezed_table_stat_t::committing);
         fs.locked_block_num = current_block_num();
      } );

      return;
   }

   // actfreezed activite freeze account table
   void lock_token::actfreezed( const account_name& committer ) {
      require_auth( name{system_account} );

      global_freezed_state gfstate( get_self(), get_self().value );
      check( ((!gfstate.exists()) || (gfstate.get().committer == 0)),
             "freezed table has activited" );

      freezed_stat freezed_stat_tbl( get_self(), get_self().value );
      auto fstat = freezed_stat_tbl.find( committer );
      check( fstat != freezed_stat_tbl.end(), "committer table no found" );
      check( fstat->state == static_cast<uint32_t>(freezed_table_stat_t::locked), "table not locked" );

      freezed_stat_tbl.modify( fstat, name{committer}, [&]( freezed_table_state& fs ) {
         fs.state = static_cast<uint32_t>(freezed_table_stat_t::actived);
         fs.actived_block_num = current_block_num();
      } );

      auto state = gfstate.get_or_create( system_account );
      state.committer = committer;
      gfstate.set( state, system_account );

      return;
   }

   // confirm a account is active from freeze account table
   void lock_token::confirmact( const account_name& account ) {
      require_auth( name{account} );

      global_freezed_state gfstate( get_self(), get_self().value );
      check( ((!gfstate.exists()) || (gfstate.get().committer == 0)), "freezed table has activited" );

      activited_accounts act_account_tbl( get_self(), get_self().value );
      check( act_account_tbl.find( account ) == act_account_tbl.end(), "account has confirm actived" );

      act_account_tbl.emplace( name{account}, [&]( activited_account& v ) {
         v.account = account;
      } );

      return;
   }

   // actconfirmed activite confirmed account after freeze account table actived
   void lock_token::actconfirmed( const account_name& account ) {
      require_auth( name{account} );

      global_freezed_state gfstate( get_self(), get_self().value );
      check( gfstate.exists() , "freezed table not activited" );
      
      const auto act_table_committer = gfstate.get().committer;
      check( act_table_committer != 0 , "freezed table not activited" );

      activited_accounts act_account_tbl( get_self(), get_self().value );
      auto itr = act_account_tbl.find( account );
      check( itr != act_account_tbl.end(), "account not confirm actived" );

      act_account_tbl.erase(itr);

      freezed_stat freezed_stat_tbl( get_self(), get_self().value );
      auto fstat = freezed_stat_tbl.find( act_table_committer );
      check( fstat != freezed_stat_tbl.end(), "no freezed stat found" );

      freezeds freeze_tbl( get_self(), act_table_committer );

      auto del_acc_itr = freeze_tbl.find( account );
      check( del_acc_itr != freeze_tbl.end(), "del account not in table" );
      freeze_tbl.erase(del_acc_itr);

      freezed_stat_tbl.modify( fstat, name{}, [&]( freezed_table_state& fs ) {
         fs.freezed_size -= 1;
      } );
   }

} /// namespace eosio
