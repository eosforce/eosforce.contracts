# Eosio.budget contract documentation

## 0. Contract introduction

Eosio.budget is a budget proposal contract that provides users with the ability to apply for proposals and apply for tokens based on the proposal.

## 1. Function

### handover

Handover Provides the function of updating the members of the motion. The contract needs to update the members of the new motion to the contract form through this function.

参数：

+ committeers Member of the motion，Vector<account_name> type
+ memo  Remarks, string type

```bash
./cleos push action eosio.budget handover '{"committeers":["eosforce","testa","testb","testc","biosbpa"],"memo":"first handover"}' -p eosio.budget
```

### propose

Proposed is the function that the user needs to call when making a proposal.

参数：

+ proposer  Proposer
+ title     Proposal title
+ content   Proposal content
+ quantity  Total tokens required for the entire proposal
+ end_num   Proposal end block height

```bash
./cleos push action eosio.budget propose '{"proposer":"eosforce","title":"test title","content":"test content","quantity":"500.0000 EOS","end_num":10000}' -p eosforce
```

### approve

Approve the ability of committee members to pass proposals

参数：

+ approver  Account name of the committee member who passed the proposal
+ id        Proposal number
+ memo      Reason for passing

```bash
./cleos  push action eosio.budget approve '{"approver":"eosforce","id":0,"memo":"test approve"}' -p eosforce	
```

### unapprove

Unapprove gives committee members the ability to reject proposals

参数：

+ approver  Account name of the committee member who rejected the proposal
+ id        Proposal number
+ memo      Reason for rejection

```bash
./cleos  push action eosio.budget unapprove '{"approver":"testa","id":0,"memo":"test unapprove"}' -p testa	
```

**The passage of the proposal requires the consent of more than two-thirds of the members of the committee, and the rejection of more than one-third of the members is rejected.**

### takecoin

After the proposal is approved, the proposer can initiate a proposal to collect coins through takecoin.

参数：

+ proposer          Proposer
+ montion_id        Proposal number
+ content           Content of the takecoin proposal
+ quantity          The number of tokens received by the proposal

```bash
./cleos  push action eosio.budget takecoin '{"proposer":"eosforce","montion_id":0,"content":"test takecoin","quantity":"10.0000 EOS"}' -p eosforce	
```

### agreecoin

Agreecoin gives the committee members a function to pass the currency bill

参数：

+ approver         Account name of the committee member who passed the bill
+ proposer         Account name for the takecoin proposal
+ id               Takecoin motion number
+ memo             Reason for passing

```bash
./cleos  push action eosio.budget agreecoin '{"approver":"eosforce","proposer":"eosforce","id":0,"memo":"test agree"}' -p eosforce	
```

### unagreecoin

Agreecoin gives committee members a function to pass the takecoin motion

参数：

+ approver         The account name of the committee member who rejected the takecoin bill
+ proposer         Account name for the takecoin proposal
+ id               Takecoin motion number
+ memo             Reason for rejection

```bash
./cleos  push action eosio.budget unagreecoin '{"approver":"testa","proposer":"eosforce","id":0,"memo":"test agree"}' -p testa	
```

**The passage of the proposal requires the approval of more than two-thirds of the members of the committee. After the adoption, the currency will be directly sent to the proposal account, and the rejection of more than one-third of the members will be rejected.**

### turndown

turndown Close proposal

参数：

+ id               Proposal ID 
+ memo             Reason for closure

```bash
./cleos  push action eosio.budget turndown '{"id":0,"memo":"test turn down"}' -p eosio.budget	
```

**After the proposal is closed, the sponsor cannot apply for the currency from the proposal.**
