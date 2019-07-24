#pragma once

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>
#include <eosio/system.hpp>

#include <string>

#include <eosforce/assetage.hpp>

namespace eosio {
   using eosforce::assetage;
   using eosforce::CORE_SYMBOL;
   using eosforce::CORE_SYMBOL_PRECISION;

   static constexpr uint64_t FIX_TIME_VOTE_POWER_M = 5;  // now vote power will be 0.2, so fix-time vote power will * 5


   /**
    *  From EOSForce FIP#7, There is two type vote : current voting and fix-time voting
    *  
    *  - Current voting: There is no fixed period for users to participate in voting.
    *  - Fix-time voting: Users choose to vote in different voting time periods.
    * 
    *  For Different voting, the power of per stake token is different
    */

   /*
      A fix-time vote state:

        `start` -- fix-time vote --> `voting` -- curr block num after (start_bnum + fix_bnum) --> `wait withdraw( no reward )`   -------
                                                                                                                                       |
         `wait unfreeze` <-- user claim --  `wait claim ( no reward and no vote power )`  <-- user withdraw (set withdraw_block_num) --
                |
                ------ curr block num after withdraw_block_num + unfreeze_num --->  `unfreezed` -- user unfreeze --> `end ( data deleted )`
   */


   // fix_time_votes : fix time vote infos for a account
   struct [[eosio::table, eosio::contract("eosio.system")]] fix_time_vote_info {
      enum state {
         nil = 0,
         voting,
         wait_withdraw,
         wait_claim, wait_unfreeze,
         unfreezed
      };

      // fix vote type, NOTE Just Can Add New TYPE, NO CHANGE LAST!!!!!
      // only can change fix-time num
      using fix_vote_data_t = std::tuple<name, uint32_t, uint64_t>;
      constexpr static auto fix_vote_typ_data = std::array<fix_vote_data_t, 4>{{
         { "fvote.a"_n,   3 * 30,   1 * FIX_TIME_VOTE_POWER_M }, 
         { "fvote.b"_n,   6 * 30,   2 * FIX_TIME_VOTE_POWER_M }, 
         { "fvote.c"_n,  12 * 30,   4 * FIX_TIME_VOTE_POWER_M }, 
         { "fvote.d"_n,  24 * 30,   8 * FIX_TIME_VOTE_POWER_M }
      }};
      // fix-time type not too much ( <= 8 ), so just find by for each
      inline static std::optional<fix_vote_data_t> get_data_by_typ( const name& typ ) {
         for( const auto& i : fix_vote_typ_data ) {
            if( std::get<0>(i) == typ ) {
               return i;
            }
         }

         return std::optional<fix_vote_data_t>();
      }


      uint64_t     key                = 0; // add by available_primary_key()
      account_name voter              = 0;
      account_name bpname             = 0;
      name         fvote_typ          = name{ 0 };
      assetage     votepower_age;
      asset        vote               = asset{ 0, CORE_SYMBOL };
      uint32_t     start_block_num    = 0;
      uint32_t     withdraw_block_num = 0;
      bool         is_withdraw        = false;

      uint64_t primary_key() const { return key; }

      // TODO: need use vote type by fvote_typ
      inline state get_state( const uint32_t curr_block_num ) const { return state::nil; }
   };
   typedef eosio::multi_index<"fixvotes"_n, fix_time_vote_info> fix_time_votes_table;
} // namespace eosio