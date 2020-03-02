# eosio.freeze contract

This contract used to implement the proposal [FIP #11] (https://github.com/eosforce/FIPs/blob/master/FIP%2311.md).

## 1. Freeze process

Freezing users will divided into the following steps:

- Create an eosio.freeze account, deploy the contract.
- Committer submits a list of frozen accounts.
- Committer checks the list of frozen accounts and locks the list after confirming that it is correct.
- (Possible) If the list is found to be incorrect after committer lock the list, you can unblock the list by bp multi-sign.
- Block producers multi-sign to confirmation of a committer's frozen account list as a basis to frozen the account, thereby activating the frozen account function.

The committer can be any account, and the committer needs to pay the RAM of the committed list, which is about 15 - 25 MB from the current situation.

The block producer needs to check if the list data is accurate after the committer locks the frozen account list.

> The frozen account list comfirmed by the block producers is responsible for all the results caused by the freezing process and the FIP 11 proposal, **Please check carefully whether the list data is accurate**.

Due to the large number of accounts, you can refer to the push_freezeds script in tools to get all the information of the locked account list.

In the process of freezing, until the last bp multi-sign confirmation, the 'inactive' account can activate the account by executing the `confirmact` action. This type of account may appear in the frozen list due to the time difference problem. Execute `confirmact`, after the freeze has taken effect, you can execute the `actconfirmed` action to release the account freeze status.

## 2. Actions

### 2.1 addfreezed : push account to frozen list

```bash
./cleos -u https://w3.eosforce.cn:443  push action eosio.freeze addfreezed '{"committer":"testa","freezeds":["taa3","taa2","taa1"],"memo":"1"}' -p testa
```

- committer: committer
- freezeds: the array of frozen accounts
- memo

> action will error after committer lock the table

Due to the large number of accounts, you can refer to the push_freezeds script in tools to commit accounts to frozen table.

### 2.2 delfreezed : delete account from frozen table

```bash
./cleos -u https://w3.eosforce.cn:443  push action eosio.freeze delfreezed '{"committer":"testd","freezeds":"aaaa1tr5zz5","memo":"dd"}' -p testd
```

- committer
- freezeds
- memo

> action will error after committer lock the table

### 2.3 lockfreezed : lock the frozen account table

```bash
./cleos -u https://w3.eosforce.cn:443  push action eosio.freeze lockfreezed '{"committer":"testa","is_locked":true}' -p testa
```

- committer
- is_locked: true mean lock the table, false mean unlock the table, unlock need eosio auth by block producers
- memo

### 2.4 actfreezed : activite the frozen account table

```bash
./cleos -u https://w3.eosforce.cn:443  push action eosio.freeze actfreezed '{"committer":"testd"}' -p eosio
```

- committer

> only the table lock by committer can be activited.

### 2.5 confirmact : activite the account when frozen

```bash
./cleos -u https://w3.eosforce.cn:443  push action eosio.freeze confirmact '{"account":"aaaa1tj2zj4"}' -p aaaa1tj2zj4
```

- account

### 2.6 actconfirmed : confirm activite account after frozen

```bash
./cleos -u https://w3.eosforce.cn:443  push action eosio.freeze actconfirmed '{"account":"aaaa1tj2zz4"}' -p aaaa1tj2zz4
```

> only success the account activited by `confirmact` action before activite frozen account can use this action.

