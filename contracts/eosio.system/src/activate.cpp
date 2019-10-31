#include <utility>

#include <eosio.system.hpp>

#include <eosio/../../capi/eosio/crypto.h>

namespace eosio {
   namespace internal_use_do_not_use {
      extern "C" {
      __attribute__((eosio_wasm_import))
      bool is_feature_activated( const ::capi_checksum256* feature_digest );

      __attribute__((eosio_wasm_import))
      void preactivate_feature( const ::capi_checksum256* feature_digest );
      }
   }
}

namespace eosio {
   bool is_feature_activated(const eosio::checksum256 &feature_digest) {
      auto feature_digest_data = feature_digest.extract_as_byte_array();
      return internal_use_do_not_use::is_feature_activated(
            reinterpret_cast<const ::capi_checksum256 *>( feature_digest_data.data())
      );
   }

   void preactivate_feature(const eosio::checksum256 &feature_digest) {
      auto feature_digest_data = feature_digest.extract_as_byte_array();
      internal_use_do_not_use::preactivate_feature(
            reinterpret_cast<const ::capi_checksum256 *>( feature_digest_data.data())
      );
   }
}

namespace eosio {

   void system_contract::activate( const eosio::checksum256& feature_digest ) {
      require_auth( get_self() );
      preactivate_feature( feature_digest );
   }

   void system_contract::reqactivated( const eosio::checksum256& feature_digest ) {
      check( is_feature_activated( feature_digest ), "protocol feature is not activated" );
   }

} /// namespace eosio


