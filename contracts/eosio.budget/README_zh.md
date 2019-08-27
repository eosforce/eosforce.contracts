# eosio.budget 合约说明文档

## 0. 合约介绍

eosio.budget是预算提案合约，给用户提供申请提案并根据提案申请代币的功能。

## 1. 基础功能

### handover

handover 提供更新议案成员的功能，合约需要一次把新议案成员通过该功能更新到合约表格里面

参数：

+ committeers 议案成员，vector<account_name> 类型
+ memo  备注，string类型

```bash
./cleos push action eosio.budget handover '{"committeers":["eosforce","testa","testb","testc","biosbpa"],"memo":"first handover"}' -p eosio.budget
```

### propose

propose 是用户提出提案需要调用的功能。

参数：

+ proposer  提案人
+ title     提案标题
+ content   提案内容
+ quantity  整个提案所需代币总数
+ end_num   提案失效块高度

```bash
./cleos push action eosio.budget propose '{"proposer":"eosforce","title":"test title","content":"test content","quantity":"500.0000 EOS","end_num":10000}' -p eosforce
```

### approve

approve 给委员会成员提供通过提案的功能

参数：

+ approver  通过提案的委员会成员的帐户名
+ id        提案编号
+ memo      通过的理由

```bash
./cleos  push action eosio.budget approve '{"approver":"eosforce","id":0,"memo":"test approve"}' -p eosforce	
```

### unapprove

unapprove 给委员会成员提供驳回提案的功能

参数：

+ approver  驳回提案的委员会成员的帐户名
+ id        提案编号
+ memo      驳回的理由

```bash
./cleos  push action eosio.budget unapprove '{"approver":"testa","id":0,"memo":"test unapprove"}' -p testa	
```

**通过议案需要委员会三分之二以上的成员同意，驳回需要三分之一以上的成员驳回**

### takecoin

提案通过后，提案人可以通过takecoin发起一个领取币的提案

参数：

+ proposer          提案人
+ montion_id        提案编号
+ content           提案内容
+ quantity          提案领取的代币数量

```bash
./cleos  push action eosio.budget takecoin '{"proposer":"eosforce","montion_id":0,"content":"test takecoin","quantity":"10.0000 EOS"}' -p eosforce	
```

### agreecoin

agreecoin 给委员会成员提供一个通过提币议案的功能

参数：

+ approver         通过提币议案的委员会成员的帐户名
+ proposer         提出提币议案的帐户名
+ id               提币议案的编号
+ memo             通过的理由

```bash
./cleos  push action eosio.budget agreecoin '{"approver":"eosforce","proposer":"eosforce","id":0,"memo":"test agree"}' -p eosforce	
```

### unagreecoin

agreecoin 给委员会成员提供一个通过提币议案的功能

参数：

+ approver         驳回提币议案的委员会成员的帐户名
+ proposer         提出提币议案的帐户名
+ id               提币议案的编号
+ memo             驳回的理由

```bash
./cleos  push action eosio.budget unagreecoin '{"approver":"testa","proposer":"eosforce","id":0,"memo":"test agree"}' -p testa	
```

**通过议案需要委员会三分之二以上的成员同意，通过之后会直接把币打给提议帐户，驳回需要三分之一以上的成员驳回**

### turndown

turndown 关闭提案

参数：

+ id               提案的ID 
+ memo             关闭的理由

```bash
./cleos  push action eosio.budget turndown '{"id":0,"memo":"test turn down"}' -p eosio.budget	
```

**提案关闭后，提案人不能从该提案申请币**
