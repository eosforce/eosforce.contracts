# EOSForce系统合约

version 1.0.0

## 1. 介绍

EOSForce系统合约是EOSForce的重要组成部分之一，
在EOSForce中, 系统合约部署在`eosio`账户及`eosio.*`账户，这些系统合约提供了大部分链的基础功能。

EOSForce的系统合约与EOSIO系统合约有很大不同，出于对于EOS不同的理解和思考，EOSForce重新规划和实现了系统合约，
主要提供以下的功能：

- 一票一投
- 投票分红
- 基于手续费的资源模型
- 平滑的更新机制
- 节点心跳机制
- 更多的合约层API

## 2. 编译

eosforce.contracts 依靠于 [eosforce.cdt](https://github.com/eosforce/eosforce.cdt)，需要安装：

```bash
git clone --recursive https://github.com/eosforce/eosforce.cdt
cd eosforce.cdt
./build.sh
sudo ./install.sh
```

之后可以编译eosforce.contracts：

```bash
git clone https://github.com/eosforce/eosforce.contracts.git
cd eosforce.contracts
./build.sh
```

## 3. License

[Apache license 2.0](https://github.com/eosforce/eosforce.contracts/blob/master/LICENSE)

## 4. 特别鸣谢

[Block.One](https://block.one/)
