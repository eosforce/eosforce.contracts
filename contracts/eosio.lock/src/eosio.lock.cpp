#include <../../eosforcelib/config.hpp>

using eosio::asset;

//static constexpr uint64_t SYMBOL = S(4, EOSLOCK);
static constexpr symbol SYMBOL = symbol(symbol_code("EOSLOCK"), 4);

class [[eosio::contract("eosio.lock")]] eoslock : public contract {
   public:
      using contract::contract;

   private:
      TABLE account {
         account_name owner;
         asset     balance = asset(0, SYMBOL);

         uint64_t primary_key()const { return owner.value; }
      };

      eosio::multi_index<"accounts"_n, account> _accounts;

};
