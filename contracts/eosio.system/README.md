# EOSForce System Contracts

## 0. System Contract

In EOSForce, the system contract carries the entire chain governance mechanism, which mainly includes bp election, core token and bp monitoring.

The following is an introduction to each action and table in the system contract.

## 1. Basic

In the system contract, `onblock` action is a special action, each block will automatically execute an `onblock` action, through this action, the system can do some routine operations, which is mainly the processing for the block rewards.

```cpp
   [[eosio::action]] void onblock( const block_timestamp& timestamp,
                                   const account_name&    bpname,
                                   const uint16_t         confirmed,
                                   const checksum256&     previous,
                                   const checksum256&     transaction_mroot,
                                   const checksum256&     action_mroot,
                                   const uint32_t         schedule_version );
```

NOTE:

- The argument to `onblock` is actually of type `block_header`. Since the serialization of fc is sequential, to use arguments and structures is equivalent.
- `onblock` can only be triggered by the system and cannot be called in other ways.

## 2. Core Token

EOSForce's core token symbol is EOS. Unlike EOSIO, EOSForce's core token is not based on eosio.token contract, but works in a system built-in implementation.

When the EOSForce main network is started, the nodeos will initialize the core token status in the C++ layer by a fixed way. The system does not directly provide the extension function for the core tokens, thus preventing bps changing the core tokens status by simple multi-sign.

The core token of the account is stored in the `accounts` table under the system account. Developer can obtain the user core token information by command:

```bash
./cleos -u https://w1.eosforce.cn:443 get table eosio eosio accounts -L eosforce -l 1
{
  "rows": [{
      "name": "eosforce",
      "available": "0.0100 EOS"
    }
  ],
  "more": true
}
```

Core token information can also be obtained using the cleos get account command:

```bash
./cleos -u https://w1.eosforce.cn:443 get account testd
created: 2018-05-28T12:00:00.000
permissions:
     owner     1:    1 EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
        active     1:    1 EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
memory:
     quota:         8 KiB    used:      2.66 KiB

EOS balances:
     liquid:        19989.9900 EOS
     total:         19989.9900 EOS
```

### 2.1 transfer

The transfer of EOSForce core tokens is done by `transfer` action:

```cpp
         [[eosio::action]] void transfer( const account_name& from,
                                          const account_name& to,
                                          const asset& quantity,
                                          const string& memo );
```

Parameter:

- from : From account
- to : To account
- quantity : Transfer asset, must be EOS asset
- memo : Remark string, must be less than 256 bytes

Minimum privilege:

- from@active

Example:

```bash
 ./cleos -u https://w1.eosforce.cn:443 push action eosio transfer '{ "from":"testa", "to":"testb", "quantity":"1000.0000 EOS", "memo":"memo to trx" }' -p testa
executed transaction: 268c6221153111f56cbf11684647a9a0cc1988210a6ee62ca35851421aaee02d  152 bytes  214 us
#         eosio <= eosio::onfee                 "000000000093b1ca640000000000000004454f53000000000000000000000000"
#         eosio <= eosio::transfer              {"from":"testa","to":"testb","quantity":"1000.0000 EOS","memo":"memo to trx"}
#         testa <= eosio::transfer              {"from":"testa","to":"testb","quantity":"1000.0000 EOS","memo":"memo to trx"}
#         testb <= eosio::transfer              {"from":"testa","to":"testb","quantity":"1000.0000 EOS","memo":"memo to trx"}
warning: transaction executed locally, but may not be confirmed by the network yet         ]
```

In cleos you can also transfer funds directly using integrated commands:

```bash
 ./cleos -u https://w1.eosforce.cn:443 transfer testd testc "10.0000 EOS" "memos"
executed transaction: 0abe9f8b84f7f2f53b9577a8322e7f7cb55672562c5cecae163935b6e6724d46  152 bytes  264 us
#         eosio <= eosio::onfee                 "000000008094b1ca640000000000000004454f53000000000000000000000000"
#         eosio <= eosio::transfer              {"from":"testd","to":"testc","quantity":"10.0000 EOS","memo":"memos"}
#         testd <= eosio::transfer              {"from":"testd","to":"testc","quantity":"10.0000 EOS","memo":"memos"}
#         testc <= eosio::transfer              {"from":"testd","to":"testc","quantity":"10.0000 EOS","memo":"memos"}
warning: transaction executed locally, but may not be confirmed by the network yet         ]
```

## 3. Vote

## 4. BP Manager
