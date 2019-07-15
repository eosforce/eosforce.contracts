#pragma once

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>

namespace eosforce {

   constexpr auto CORE_SYMBOL = eosio::symbol(eosio::symbol_code("EOS"), 4);

   namespace utils{
      constexpr inline int64_t precision( const uint8_t decimals ) {
         constexpr uint8_t res_size = 18;
         constexpr int64_t res[res_size] = 
            {  1, 10, 100, 1000, 10000, 
               100000, 
               1000000, 
               10000000,
               100000000,
               1000000000,
               10000000000,
               100000000000,
               1000000000000,
               10000000000000,
               100000000000000,
               1000000000000000,
               10000000000000000,
               100000000000000000 };

         if( decimals < res_size ) {
            return res[decimals];
         } else {
            auto p10 = res[res_size - 1];
            for( auto p = static_cast<int64_t>(decimals - res_size + 1); 
               p > 0; --p ) {
               p10 *= 10;
            }
            return p10;
         }
      }
   }

   using eosio::symbol;
   using eosio::symbol_code;
   using eosio::asset;

   // asset age : asset age is a value equal asset * age by block num, in eosforce
   //             it used in many pos.
   struct assetage {
      constexpr int64_t get_age( const uint32_t block_num ) const {
         return ((staked.amount / utils::precision(staked.symbol.precision())) * ( block_num - update_height )) + age;
      }


      asset     staked;
      uint32_t  update_height = 0;
      int64_t   age           = 0;

      EOSLIB_SERIALIZE( assetage, (staked)(update_height)(age) )
   };

} // namespace eosforce