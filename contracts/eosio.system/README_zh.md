# EOSForce系统合约说明文档

## 0. 系统合约介绍

在EOSForce中, 系统合约承载了整个链的治理机制, 其中主要包括bp的选举,核心代币以及bp的监控。

以下是系统合约中各个action和table的介绍。

## 1. 基础功能

在系统合约中, `onblock`是一个一个特殊的action, 每个区块中都会自动执行一次`onblock`, 通过这个action, 系统可以完成一些例行操作,其中主要就是分配块产生的核心代币增发。

```cpp
         [[eosio::action]] void onblock( const block_timestamp& timestamp,
                                         const account_name&    bpname,
                                         const uint16_t         confirmed,
                                         const checksum256&     previous,
                                         const checksum256&     transaction_mroot,
                                         const checksum256&     action_mroot,
                                         const uint32_t         schedule_version );
```

注意：

- `onblock`的参数实际上的类型为`block_header`类型, 由于fc的序列化是顺序的, 所以这里使用参数列表和结构是等价的。
- `onblock`只能由系统直接触发，不能采取其他方式调用。

## 2. 核心代币

EOSForce的核心代币符号为EOS, 与EOSIO不同, EOSForce的核心代币不是基于eosio.token合约发行, 而是以系统内置实现的方式工作.

在EOSForce主网启动时, nodeos会以固定的方式在C++层完成核心代币状态的初始化, 系统对于核心代币并没有直接提供增发功能, 以此防止BP通过简单的多签随意更改核心代币状态.

账户的核心代币存储于系统账户下的`accounts`表中, 可以通过命令获取用户核心代币信息:

```bash
./cleos -u https://w1.eosforce.cn:443 get table eosio eosio accounts -k eosforce
{
  "rows": [{
      "name": "eosforce",
      "available": "0.0100 EOS"
    }
  ],
  "more": true
}
```

使用cleos的get account命令也可以获取核心代币信息：

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

EOSForce核心代币的转账通过`transfer`完成:

```cpp
         [[eosio::action]] void transfer( const account_name& from,
                                          const account_name& to,
                                          const asset& quantity,
                                          const string& memo );
```

参数：

- from : 转账账号
- to : 收款账号
- quantity : 金额，必须是 EOS资产
- memo : 备注, 必须小于256字节

最小权限:

- from@active

实例:

```bash
 ./cleos -u https://w1.eosforce.cn:443 push action eosio transfer '{ "from":"testa", "to":"testb", "quantity":"1000.0000 EOS", "memo":"memo to trx" }' -p testa
executed transaction: 268c6221153111f56cbf11684647a9a0cc1988210a6ee62ca35851421aaee02d  152 bytes  214 us
#         eosio <= eosio::onfee                 "000000000093b1ca640000000000000004454f53000000000000000000000000"
#         eosio <= eosio::transfer              {"from":"testa","to":"testb","quantity":"1000.0000 EOS","memo":"memo to trx"}
#         testa <= eosio::transfer              {"from":"testa","to":"testb","quantity":"1000.0000 EOS","memo":"memo to trx"}
#         testb <= eosio::transfer              {"from":"testa","to":"testb","quantity":"1000.0000 EOS","memo":"memo to trx"}
warning: transaction executed locally, but may not be confirmed by the network yet         ]
```

在cleos中也可以使用集成的命令直接转账:

```bash
 ./cleos -u https://w1.eosforce.cn:443 transfer testd testc "10.0000 EOS" "memos"
executed transaction: 0abe9f8b84f7f2f53b9577a8322e7f7cb55672562c5cecae163935b6e6724d46  152 bytes  264 us
#         eosio <= eosio::onfee                 "000000008094b1ca640000000000000004454f53000000000000000000000000"
#         eosio <= eosio::transfer              {"from":"testd","to":"testc","quantity":"10.0000 EOS","memo":"memos"}
#         testd <= eosio::transfer              {"from":"testd","to":"testc","quantity":"10.0000 EOS","memo":"memos"}
#         testc <= eosio::transfer              {"from":"testd","to":"testc","quantity":"10.0000 EOS","memo":"memos"}
warning: transaction executed locally, but may not be confirmed by the network yet         ]
```

## 3. 选举机制

## 4. BP监控机制
