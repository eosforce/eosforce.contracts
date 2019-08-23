#include <eosio.budget/eosio.budget.hpp>
#include <../../eosio.system/include/eosio.system.hpp>

namespace eosio {

   budget::budget( name s, name code, datastream<const char*> ds ) 
   : contract( s, code, ds )
      {}

   budget::~budget() {}


   void budget::handover( vector<account_name> committeers ) {
      require_auth( get_self() );
      committee_table  committee_tbl( get_self(), get_self().value );

      
   }

   void budget::propose( account_name proposer,string title,string content,asset quantity,uint32_t end_num ) {

   }

   void budget::approve( account_name approver,uint64_t id ) {

   }

   void budget::unapprove( account_name approver,uint64_t id ) {

   }

   void budget::takecoin( account_name proposer,uint64_t id ) {

   }

   void budget::agreecoin( account_name approver,uint64_t id ) {

   }

   void budget::unagreecoin( account_name approver,uint64_t id ) {

   }

   void budget::turndown( uint64_t id ) {

   }



} /// namespace eosio
