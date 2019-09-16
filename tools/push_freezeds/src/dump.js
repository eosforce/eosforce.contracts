const { Api, JsonRpc, RpcError } = require("eosforcejs2");
const { JsSignatureProvider } = require("eosforcejs2/dist/eosjs-jssig"); // development only
const fetch = require("node-fetch"); // node only; not needed in browsers
const { TextEncoder, TextDecoder } = require("util"); // node only; native TextEncoder/Decoder

const fs = require("fs");

const defaultPrivateKeys = [
  "5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3",
  "5KYQvUwt6vMpLJxqg4jSQNkuRfktDHtYDp8LPoBpYo8emvS1GfG"
];
const signatureProvider = new JsSignatureProvider(defaultPrivateKeys);

const rpc = new JsonRpc("http://127.0.0.1:8001", { fetch });
const api = new Api({
  rpc,
  signatureProvider,
  textDecoder: new TextDecoder(),
  textEncoder: new TextEncoder()
});

const key_name = 'account'

const dump_table_rows = async ( code, scope, table, handler_func ) => {
   let l = ''
   let more = true
   let params = {
     scope: scope,
     code: code,
     table: table,
     limit: 100,
     json: true,
   }

   for( ; more; ){
      params.lower_bound = l
      res = await rpc.get_table_rows(params);
  
      if(res.rows.length !== 0){
        // first will repeat
        start = 1
        if(l == ''){
          start = 0
        }
  
        // to res rows
        for(let i = start; i < res.rows.length; i++){
          handler_func(res.rows[i])
        }
  
        l = res.rows[res.rows.length - 1][key_name]
      }
  
      more = res.more
    }
}

const dump_all = async ( committer ) => {
   let res = []
   await dump_table_rows( 'eosio.freeze', committer, 'freezed', ( row ) => {
      res.push(row[key_name])
   } );

   console.log("get accounts %d, from %s to %s", res.length, res[0], res[res.length - 1])

   let file_name = './dump-table-freezed-' + committer + '.json';

   console.log("dump to file : %s", file_name)
   fs.writeFileSync( file_name, JSON.stringify(res));
}

const check_freeze_table_stat = async ( committer ) => {
   let params = {
      scope: 'eosio.freeze',
      code: 'eosio.freeze',
      table: 'freezedstat',
      lower_bound: committer,
      key_type: "name",
      limit: 1,
      json: true,
    }
   res = await rpc.get_table_rows(params);

   //console.log(res.rows)

   if (res.rows.length == 0 || res.rows[0]['committer'] != committer){
      return false;
   }

   data = res.rows[0];

   is_ok = data['state'] != 0

   if( is_ok ){
      console.log( 'the table in size %d is locked in block #%d', data['freezed_size'], data['locked_block_num'] )
   }

   return is_ok
}

( async ( committer ) =>{
   if( !(await check_freeze_table_stat( committer )) ){
      console.error("freeze table not locked, so the table will change!!!");
      return;
   }
   await dump_all( committer )
})( 'testa' )
