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

EOSForce has designed a voting mechanism which is completely different from EOSIO.
The actions for voting is also very different from EOSIO.

There are two types of voting in EOSForce: current voting and fix-time voting.
User voting informations is stored in the `votes` table and the `fixvotes` table.
In addition, EOSForce can use voting dividends to deduct the RAM rent, it is in the `votes4ram` table and the `vote4ramsum` table.

For example, in the `votes` table, you can get a user voting information:

```bash
./cleos -u https://w1.eosforce.cn:443 get table eosio testd votes
{
  "rows": [{
      "bpname": "biosbpa",
      "voteage": {
        "staked": "200.0000 EOS",
        "age": 0,
        "update_height": 176
      },
      "unstaking": "0.0000 EOS",
      "unstake_height": 176
    },{
      "bpname": "biosbpb",
      "voteage": {
        "staked": "300.0000 EOS",
        "age": 0,
        "update_height": 179
      },
      "unstaking": "0.0000 EOS",
      "unstake_height": 179
    }
  ],
  "more": false
}
```

For BPs, the information for obtaining votes is in the `bps` table, and other information of BPs is also in this table.

The detail information about the above tables is described in the `claim` action.

User voting information can be obtained by cleos get account command (or directly call http api):

```bash
./cleos -u https://w1.eosforce.cn:443 get account testd
created: 2018-05-28T12:00:00.000
permissions:
     owner     1:    1 EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
        active     1:    1 EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
memory:
     quota:         8 KiB    used:     3.262 KiB

EOS balances:
     liquid:        19389.8400 EOS
     total:         19389.8400 EOS

current votes:
     biosbpa          200.0000 EOS     unstaking:         0.0000 EOS
     biosbpb          300.0000 EOS     unstaking:         0.0000 EOS
     biosbpv          100.0000 EOS     unstaking:         0.0000 EOS
```

In EOSForce, the user votes on the core token and implements the “one token one vote” voting method.
For the current voting, the user uses the `vote` action to vote.
After voting, the user can use `revote` action to change the BP he is voting in any time.
When the user decides to withdraw the vote to exchange for the core token,
user must first withdraw the votings originally sent to the BP.
At this time, these vote are in the state of “to be unfreeze”.
After waiting for the lockout period (generally equivalent to the number of blocks in 3 days),
the vote can be convert to the core token by `unfreeze` action.
The process of fix-time voting is basically the same as the current vote.

In EOSForce, all votes are stored and manipulated based on the user's BP. The following is an introduction to the action to voting:

### 3.1 Current Vote

```cpp
   [[eosio::action]] void vote( const account_name& voter,
                                const account_name& bpname,
                                const asset& stake );
```

Modify account's total vote for a block producer.

Parameters:

- voter : account to vote
- bpname : bp 
- stake : the token in vote to change to

Minimum privilege:

- voter@active

Note:

1.`vote` action is to modify the user's vote for a certain BP, that is mean, it will increase the vote  to the bp when the number of stake is greater than the current number of votes, and in other side it will withdrawn when the number of votes is less than the current number of votes.
2. According to the number of votes, user balance will be reduced.
3. Increase the total number of votes for the node, the current total ticketing age of the settlement node

Example:

### 3.2 Revote

**Revote** allows user to change node which has voted, without unfreeze and freeze token.

```cpp
void revote( const account_name voter,
             const account_name frombp,
             const account_name tobp,
             const asset restake ) {
```

Parameter:

- voter : account to vote
- frombp : the from bp which voter voted
- tobp :  the to bp which voter want to vote
- restake : the token change of votes

Minimum privilege:

- voter@active

Example:

Suppose `testa` account vote the `biosbpa` node with `5000.0000 EOS`,
At this point, the user wants to switch votes from `biosbpa` to the `biosbpb` node with  `2000.0000 EOS`, then can execute:

```bash
./cleos -u https://w1.eosforce.cn:443 push action eosio revote '{"voter":"testa","frombp":"biosbpa","tobp":"biosbpb","restake":"2000.0000 EOS"}' -p testa
executed transaction: 526054c5f4a2d5f2aff91abc21699fde0e93a1f7895cb6d9b34742c2834ae2f2  152 bytes  280 us
#         eosio <= eosio::onfee                 {"actor":"testa","fee":"0.2000 EOS","bpname":""}
#         eosio <= eosio::revote                {"voter":"testa","frombp":"biosbpa","tobp":"biosbpb","restake":"2000.0000 EOS"}
warning: transaction executed locally, but may not be confirmed by the network yet         ]
```

After action execution, the account token vote to the `biosbpa` node is `3000.0000 EOS`,
and the token vote to for the `biosbpb` node is increased by `2000.0000 EOS`.

### 3.3 Fix-time Vote

User can selects a different voting fix-time to vote. The token is locked during fix-time voting and can be unlocked when it expires.
Different fixed time will correspond to different power for vote, user can claim fix-time vote rewards at any time, note the votes will be calc to zero after the fix-time voting expires.

When user make a fix-time vote, the RAM is borne by the user. after block height number reach the unlocked height of a fix-time vote,
the user needs to manually cancel the fix-time vote, in which the locked token will be counted to the token in redemption, and can be redeemed as the system token after the redemption freeze period arrives.

Because system need calc voting reward even after the user cancels a fix-time voting, the fix-time vote information will not be deleted. The system simply sets a flag to indicate that a voting has expired.
After the user claim the voting reward, the information will be deleted and returned to the user RAM.

Fix-time voting information is stored in the `fixvotes` table and can be queried:

```bash
./cleos -u https://w1.eosforce.cn:443 get table eosio testd fixvotes
{
  "rows": [{
      "key": 0,
      "voter": "testd",
      "bpname": "biosbpc",
      "fvote_typ": "fvote.a",
      "votepower_age": {
        "staked": "500.0000 EOS",
        "age": 0,
        "update_height": 444
      },
      "vote": "100.0000 EOS",
      "start_block_num": 444,
      "withdraw_block_num": 2592444,
      "is_withdraw": 0
    },{
      "key": 1,
      "voter": "testd",
      "bpname": "biosbpc",
      "fvote_typ": "fvote.a",
      "votepower_age": {
        "staked": "500.0000 EOS",
        "age": 0,
        "update_height": 446
      },
      "vote": "100.0000 EOS",
      "start_block_num": 446,
      "withdraw_block_num": 2592446,
      "is_withdraw": 0
    }
  ],
  "more": false
}
```

The fields information is as follows:

- key : id used to indicate each vote. Note that this key is not unique for all each vote. It may be re-joined after old vote being deleted.
- voter : account to vote
- bpname : the BP to vote for
- fvote_typ : fix-time voting type
- votepower_age : the powered asset age generated by the vote, used to calculate the voting reward
- vote : the actual system token to vote
- start_block_num : the starting block height number of the vote
- withdraw_block_num : the block height number of the vote can be withdraw
- is_withdraw : Whether vote has been revoked, if true,  it mean the vote has been withdrawn, waiting to be removed after receiving the reward.

The current **fix-time voting type** is as follows:

| Type | Locked Block Height Number | Power |
|----------|------------------|-----------|
| fvote.a | 2592000 (about 90 days) | 1 |
| fvote.b | 5184000 (about 180 days) | 2 |
| fvote.c | 10368000 (about 360 days) | 4 |
| fvote.d | 20736000 (about 720 days) | 8 |

Fix-time voting is based on the `votefix` action:

```cpp
         [[eosio::action]] void votefix( const account_name& voter,
                                         const account_name& bpname,
                                         const name& type,
                                         const asset& stake );
```

Parameter:

- voter : account to vote
- bpname : bp name
- type : fix-time voting type
- stake : core token to vote

Minimum privilege:

- voter@active

Example:

```bash
./cleos -u https://w1.eosforce.cn:443 push action eosio votefix '["testd", "biosbpc", "fvote.a", "100.0000 EOS"]' -p testd
executed transaction: 505b30ff5c3676197a48c9ea7d0e8ef8cd8457681797a4ce2878eff637a4c55d  152 bytes  419 us
#         eosio <= eosio::onfee                 {"actor":"testd","fee":"0.2500 EOS","bpname":""}
#         eosio <= eosio::votefix               {"voter":"testd","bpname":"biosbpc","type":"fvote.a","stake":"100.0000 EOS"}
warning: transaction executed locally, but may not be confirmed by the network yet
```

## BP related

The following is the bp related action:

### 4.1 Register BP

The account can be registered as bp by `updatebp`, and the BP information will be modified if BP is already registered.

```cpp
   [[eosio::action]] void updatebp( const account_name& bpname,
                                    const public_key& block_signing_key,
                                    const uint32_t commission_rate,
                                    const std::string& url );
```

parameter:

- bpname : BP name
- block_signing_key : Blocked signature public key, used for exporting
- commission_rate : Distribution rate
- url : Bp node URL information

Least privilege:

- voter@active

Example:

```bash
./cleos -u https://w1.eosforce.cn:443 push action eosio updatebp \
'{ "bpname":"testd","block_signing_key":"EOS83e8NsiUvAi4aCePsGsbyxxiwQomsqcz2cLnHwCNjtUQVmDy3c","commission_rate":1,"url":"http://eosforce.io" }' -p testd
executed transaction: 35b43489ce9743ea74192cec6b1f8ddff9a94270f4090fd56d73c218eafcf9e8  176 bytes  236 us
#         eosio <= eosio::onfee                 {"actor":"testd","fee":"100.0000 EOS","bpname":""}
#         eosio <= eosio::updatebp              {"bpname":"testd","block_signing_key":"EOS83e8NsiUvAi4aCePsGsbyxxiwQomsqcz2cLnHwCNjtUQVmDy3c","commi...
warning: transaction executed locally, but may not be confirmed by the network yet         ]
```

### 4.2 BP heartbeat

In order to ensure the stability of the EOSForce main network, the EOSForce system requires nodes to submit heartbeat actions to the chain.
Each BP executes the `heartbeat` action every ten minutes, usually this operation is based on the heartbeat_plugin operation, so that the BP node is running.
Although bp can accomplish similar functions by developing a timing program, its overall cost will be higher than the actual node, so this method can effectively promote the stability of the entire network.

If the node does not complete `heartbeat` on time, the system will deduct its revenue.

Because BP accounts involve reward tokens, they often don't expose private keys to some form of "hot" wallet, or even use BP accounts.
Therefore, `heartbeat` cannot be operated directly based on the BP account, so BP needs to establish an account with the permission of the public key account to execute `heartbeat`,

Configure heartbeat_plugin as follows:

It is assumed here that the account name of BP is bp.name, and the public key of the block signature is EOS83e8NsiUvAi4aCePsGsbyxxiwQomsqcz2cLnHwCNjtUQVmDy3c
BP establishes a bp.ping account whose public key is EOS83e8NsiUvAi4aCePsGsbyxxiwQomsqcz2cLnHwCNjtUQVmDy3c, which is used to send heartbeats.

```bash
bp-mapping=bp.name=KEY:bp.ping
plugin=eosio::heartbeat_plugin
```

> Note: Because the heartbeat also requires a fee, the node needs to ensure that the tokens sent by the heartbeat account are sufficient.

```cpp
   [[eosio::action]] void heartbeat( const account_name& bpname,
                                     const time_point_sec& timestamp );
```

parameter:

- bpname : BP name
- time_point_sec : No parameters are used, subsequent can be used for chain time calibration, currently not in effect

Least privilege:

- bppingname@active

### 4.3 Node penalty

Node penalty related functions are implemented based on related functions such as deposit system and node monitoring.

### 4.3.1 Deposit system

The block node and the income node need to pay a portion of the deposit as a deposit to obtain the proceeds. The contract for the deposit is eosio.pledge, the type of the deposit is block.out, and all nodes need to transfer at least 12522 EOSCs to eosio.pledge to obtain the proceeds.

```bash
../../build/programs/cleos/cleos --wallet-url http://127.0.0.1:6666 --url http://127.0.0.1:8001 push action eosio.pledge open '["block.out","biosbpa","testopen"]' -p biosbpa
../../build/programs/cleos/cleos --wallet-url http://127.0.0.1:6666 --url http://127.0.0.1:8001 transfer biosbpa eosio.pledge "20000.0000 EOS" "block.out"
```

The deposit type information for block.out is as follows

```bash
{
      "pledge_name": "block.out",
      "deduction_account": "eosio",
      "pledge": "2.0000 EOS"
    }
```

Deduction_account is eosio, which has the authority to deduct the deposit and distribute the fine. Only eosio has

### 4.3.2 Node monitoring

The node monitoring function is a function of monitoring whether the node continuously exports the block and records the missing information of the node. After the combination of the deposit system, the deposit of 10.0000 EOSC is deducted for each missing block.
Node monitoring information is above the table bpmonitor

```bash
../../build/programs/cleos/cleos --wallet-url http://127.0.0.1:6666 --url http://127.0.0.1:8001 get table eosio eosio bpmonitor
...
{
      "bpname": "eosforce",
      "last_block_num": 0,
      "consecutive_drain_block": 18,
      "consecutive_produce_block": 0,
      "total_drain_block": 18,
      "stability": 1000,
      "bock_age": 0,
      "bp_status": 1,
      "end_punish_block": 0
    }
...

```

The above eosforce node continuously leaks 18 blocks, and a total of 18 blocks are leaked. The bp_status is 1 and can be punished.
Node history leak blocks are recorded on the table drainblocks

```bash
../../build/programs/cleos/cleos --wallet-url http://127.0.0.1:6666 --url http://127.0.0.1:8001 get table eosio eosforce drainblocks
{
  "rows": [{
      "current_block_num": 1214,
      "drain_block_num": 26
    }
  ],
  "more": false
}
```

### 4.3.4 Proposed penalty node

On the node monitoring, if a BP consecutively leaks more than 9 blocks, BP's bp_status is set to 1, which is the state to be punished. The user can propose to punish the node. The function of the penalty node is punishbp. The user who proposes to punish the node needs to have a deposit of 100 EOSC on the block.out.

```bash
../../build/programs/cleos/cleos --wallet-url http://127.0.0.1:6666 --url http://127.0.0.1:8001 push action eosio punishbp '["eosforce","biosbpa"]' -p biosbpa
```

A node can only have one penalty action at a time. After the proposed penalty, the proposed penalty node will be added to the table punishbps and the passed node and the effective block height.

```bash
../../build/programs/cleos/cleos --wallet-url http://127.0.0.1:6666 --url http://127.0.0.1:8001 get table eosio eosio punishbps
{
  "rows": [{
      "punish_bp_name": "eosforce",
      "proposaler": "biosbpa",
      "approve_bp": [],
      "effective_block_num": 30155
    }
  ],
  "more": false
}
```

The table above shows that biosbpa proposes to punish eosforce, which will expire after 30,155 blocks.

### 4.3.5 Agree to punish node

After the node is proposed to be punished, it needs 16 outbound nodes to agree to the node to be punished. The function of agreeing to punish the node is approvebp

```bash
../../build/programs/cleos/cleos --wallet-url http://127.0.0.1:6666 --url http://127.0.0.1:8001 push action eosio approvebp '["eosforce","biosbpa"]' -p biosbpa
```

When the 16th outbound node agrees that the node that will be punished in the future will be automatically punished, the contract will automatically assign the node's penalty reward to the BP that proposes to punish the node and agree to punish the node (if the node does not pay the penalty, then There will be any rewards)

### 4.3.6 Punished node recovery

When the node is punished, it will set bp_status to 2, that is, it is being punished. End_punish_block is the block height that can end the penalty. When the block height is reached, the node that is punished can restore itself to normal through the bailpunish function.

```bash
../../build/programs/cleos/cleos --wallet-url http://127.0.0.1:6666 --url http://127.0.0.1:8001 get table eosio eosio bpmonitor
...
{
      "bpname": "eosforce",
      "last_block_num": 0,
      "consecutive_drain_block": 15,
      "consecutive_produce_block": 0,
      "total_drain_block": 41,
      "stability": 1000,
      "bock_age": 0,
      "bp_status": 2,
      "end_punish_block": 30310
    }
...
```

```bash
../../build/programs/cleos/cleos --wallet-url http://127.0.0.1:6666 --url http://127.0.0.1:8001 push action eosio bailpunish '["eosforce"]' -p eosforce
```

## 4.4 Dividend change

+ Increase the block dividend, 0.5 EOSC per block into the block dividend pool
+ The voting dividends increased to 3 EOSC per block, of which 0.5 EOSCs were distributed to the nodes as dividends, and 2.5 EOSCs were entered into the voting pool.

### 4.4.1 BP receives block dividends and dividends

```bash
../../build/programs/cleos/cleos --wallet-url http://127.0.0.1:6666 --url http://127.0.0.1:8001 push action eosio bpclaim '["biosbpa"]' -p biosbpa
```

**This part of BP's dividends can only be collected if it exceeds 100 EOSC.**