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
   struct [[eosio::table, eosio::contract("eosio.system")]] fix_time_votes {

      // one fix time vote
      struct fix_time_vote_info {
         enum state {
            nil = 0,
            voting,
            wait_withdraw,
            wait_claim, wait_unfreeze,
            unfreezed
         };

         name     fvote_typ          = name{ 0 };
         assetage votepower_age;
         asset    vote               = asset{ 0, CORE_SYMBOL };
         uint32_t start_block_num    = 0;
         uint32_t withdraw_block_num = 0;
         bool     is_withdraw        = false;

         // TODO: need use vote type by fvote_typ
         inline state get_state( const uint32_t curr_block_num ) const { return state::nil; }
      };

      account_name                    voter = 0;
      std::vector<fix_time_vote_info> votes;
   };

} // namespace eosio