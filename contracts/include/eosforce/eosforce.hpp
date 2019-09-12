#pragma once

#include <eosio/eosio.hpp>

namespace eosforce {
   namespace chain_func {
      static constexpr auto bp_punish = "c.bppunish"_n;

      // f.freeze the function that activite freeze some account in freeze table in freeze contract
      static constexpr auto freeze_account = "f.freeze"_n;
   }
} // namespace eosforce