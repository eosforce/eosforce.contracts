#include <utility>

#include <eosio.system.hpp>

namespace eosio {

   // WARNNING : EOSForce is different to eosio, which system will not call native contract in chain
   // so native is just to make abi, system_contract no need contain navtive
   system_contract::system_contract( name s, name code, datastream<const char*> ds ) 
      : contract( s, code, ds )
      , _accounts( get_self(), get_self().value )
      , _bps( get_self(), get_self().value )
      , _blackproducers( get_self(), get_self().value )
      , _bpmonitors( get_self(), get_self().value )
      , _lastproducers( get_self(), get_self().value )
      , _systemconfig( get_self(), get_self().value ) {}

   system_contract::~system_contract() {}

   void system_contract::updateconfig( const name& config,const uint64_t &number_value,const string &string_value ) {
      require_auth( _self );

      auto config_info = _systemconfig.find(config.value);
      if ( config_info == _systemconfig.end() ) {
         _systemconfig.emplace( get_self(), [&]( auto& s ) { 
            s.config_name = config;
            s.number_value = number_value;
            s.string_value = string_value;
         });
      }
      else {
         _systemconfig.modify( config_info,name{}, [&]( auto& s ) { 
            s.number_value = number_value;
            s.string_value = string_value;
         });
      }
   }

   void system_contract::reqauth( name from ) {
      require_auth( from );
   }
} /// namespace eosio
