# eosio.freeze 合约

用于实现提案 [FIP #11](https://github.com/eosforce/FIPs/blob/master/FIP%2311_zh.md).

## 1. 冻结流程

冻结用户分以下几个步骤:

- 创建eosio.freeze合约, 部署合约.
- 提交者提交冻结账户列表.
- 提交者检查冻结账户列表, 确认无误之后锁定列表.
- (可能) 如果冻结列表之后发现列表有误, 可以通过bp多签解除列表锁定.
- bp多签确认一个提交者的冻结账户列表作为冻结账户依据, 由此激活冻结账户功能.

提交者可以是任何账户, 提交者需要支付提交的列表的RAM, 从目前情况看需要大约 15 - 25 MB.

超级节点需要在提交者锁定冻结账户列表之后, 检查列表数据是否准确.

> 确认冻结账户列表的超级节点需要为冻结过程及FIP 11提案所造成的所有结果负责, **请谨慎检查列表数据是否准确**.

由于账户数较多, 可以参照 tools 中的 push_freezeds 脚本来获取锁定后的冻结账户列表的全部信息.

在冻结的过程中, 至最后bp多签确认之前, ‘未激活’的创世账户可以通过执行`confirmact` action来激活账户, 这一类账户可能由于时间差的问题出现在冻结列表中, 如果之前执行过`confirmact`, 在冻结生效之后可以执行`actconfirmed` action来解除账户冻结状态.

## 2. Actions

### 2.1 addfreezed 添加账户到冻结列表

```bash
./cleos -u https://w3.eosforce.cn:443  push action eosio.freeze addfreezed '{"committer":"testa","freezeds":["taa3","taa2","taa1"],"memo":"1"}' -p testa
```

- committer: 提交者
- freezeds: 锁定的账户名数组
- memo: 备注

> 当锁定冻结列表之后, 此action无效

由于账户数较多, 可以参照 tools 中的 push_freezeds 脚本来批量提交账户.

### 2.2 delfreezed 删去冻结列表中的账户

```bash
./cleos -u https://w3.eosforce.cn:443  push action eosio.freeze delfreezed '{"committer":"testd","freezeds":"aaaa1tr5zz5","memo":"dd"}' -p testd
```

- committer: 提交者
- freezeds: 锁定的账户名
- memo: 备注

> 当锁定冻结列表之后, 此action无效

### 2.3 lockfreezed 锁定冻结列表

```bash
./cleos -u https://w3.eosforce.cn:443  push action eosio.freeze lockfreezed '{"committer":"testa","is_locked":true}' -p testa
```

- committer: 提交者
- is_locked: true 为锁定列表, false 为解锁, 解锁需要bp多签
- memo: 备注

### 2.4 actfreezed 激活冻结列表

```bash
./cleos -u https://w3.eosforce.cn:443  push action eosio.freeze actfreezed '{"committer":"testd"}' -p eosio
```

- committer: 激活冻结列表的提交者

> 注意只有锁定的列表才能被确认激活

### 2.5 confirmact 在冻结过程中激活账户

```bash
./cleos -u https://w3.eosforce.cn:443  push action eosio.freeze confirmact '{"account":"aaaa1tj2zj4"}' -p aaaa1tj2zj4
```

- account: 激活的账户

> 只能在激活冻结列表之前操作才有效

### 2.6 actconfirmed 在冻结之后确认账户激活

```bash
./cleos -u https://w3.eosforce.cn:443  push action eosio.freeze actconfirmed '{"account":"aaaa1tj2zz4"}' -p aaaa1tj2zz4
```

- account: 激活的账户

> 只有激活冻结列表之前confirmact的操作之后的账户才能完成此操作.

## 3. TODOs

- 激活冻结之后应该允许未被接纳的冻结列表删去各个项以释放内存.
