#pragma once
#include <../../eosforcelib/config.hpp>


namespace eosio {
   using std::vector;
   using std::string;
   class [[eosio::contract("eosio.token")]] token : public contract {
      public:
         using contract::contract;

         ACTION create( account_name issuer,
                  asset        maximum_supply);

         ACTION issue( account_name to, asset quantity, std::string memo );

         ACTION transfer( account_name from,
                        account_name to,
                        asset        quantity,
                        string       memo );

         using create_action = action_wrapper<"create"_n, &token::create>;
         using issue_action = action_wrapper<"issue"_n, &token::issue>;
         using transfer_action = action_wrapper<"transfer"_n, &token::transfer>;
      private:
         TABLE account {
            asset    balance;

            uint64_t primary_key()const { return balance.symbol.code().raw(); }
         };

         TABLE currency_stats {
            asset          supply;
            asset          max_supply;
            account_name   issuer;

            uint64_t primary_key()const { return supply.symbol.code().raw(); }
         };

         typedef eosio::multi_index<"accounts"_n, account> accounts;
         typedef eosio::multi_index<"stat"_n, currency_stats> stats;

         void sub_balance( account_name owner, asset value );
         void add_balance( account_name owner, asset value, account_name ram_payer );
      public:
         static asset get_supply( name token_contract_account, symbol_code sym_code )
         {
            stats statstable( token_contract_account, sym_code.raw() );
            const auto& st = statstable.get( sym_code.raw() );
            return st.supply;
         }

         static asset get_balance( name token_contract_account, name owner, symbol_code sym_code )
         {
            accounts accountstable( token_contract_account, owner.value );
            const auto& ac = accountstable.get( sym_code.raw() );
            return ac.balance;
         }

   };//end class
}//end namespace 