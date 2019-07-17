
# EOSForce System Contracts

Version 1.0.0

## 1. Introduction

EOSForce System Contracts is an important part of EOSForce.
In EOSForce, system contracts are set code and abi to `eosio` account or `eosio.*` accounts, which provide the basic functionality of most chains.

The EOSForce system contract is very different from the EOSIO system contract. EOSForce re-planned and implemented the system contract for different understanding and thinking about EOS.
EOSForce mainly provides the following features:

- One token one vote
- Reward to voter
- Fee-based resource model
- Smooth update mechanism
- Node heartbeat mechanism
- More APIs in contract layer

## 2. Compile

Eosforce.contracts relies on [eosforce.cdt] (https://github.com/eosforce/eosforce.cdt).
To build eosforce.contracts needs to install first:

```bash
Git clone --recursive https://github.com/eosforce/eosforce.cdt
Cd eosforce.cdt
./build.sh
Sudo ./install.sh
```

Then can then compile eosforce.contracts:

```bash
Git clone https://github.com/eosforce/eosforce.contracts.git
Cd eosforce.contracts
./build.sh
```

## 3. License

[Apache license 2.0] (https://github.com/eosforce/eosforce.contracts/blob/master/LICENSE)

## 4. Special thanks

[Block.One] (https://block.one/)