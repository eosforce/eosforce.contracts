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

## 3. 选举机制

## 4. BP监控机制
