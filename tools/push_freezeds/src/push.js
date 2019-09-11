const { Api, JsonRpc, RpcError } = require("eosforcejs2");
const { JsSignatureProvider } = require("eosforcejs2/dist/eosjs-jssig"); // development only
const fetch = require("node-fetch"); // node only; not needed in browsers
const { TextEncoder, TextDecoder } = require("util"); // node only; native TextEncoder/Decoder

const defaultPrivateKeys = [
  "5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3",
  "5KYQvUwt6vMpLJxqg4jSQNkuRfktDHtYDp8LPoBpYo8emvS1GfG"
];
const signatureProvider = new JsSignatureProvider(defaultPrivateKeys);

const rpc = new JsonRpc("http://127.0.0.1:8001", { fetch });
const api = new Api({
  rpc,
  signatureProvider,
  textDecoder: new TextDecoder(),
  textEncoder: new TextEncoder()
});

const get_freezed_accounts_action = (committer, accounts, memo) => {
  console.log("push %d from %s - %s", accounts.length, accounts[0], accounts[accounts.length - 1]);
  return {
    account: "eosio.freeze",
    name: "addfreezed",
    authorization: [
      {
        actor: committer,
        permission: "active"
      }
    ],
    data: {
      committer: committer,
      freezeds: accounts,
      memo: memo
    }
  };
};

const sleep = (milliseconds) => {
   return new Promise(resolve => setTimeout(resolve, milliseconds))
 }

const push_freezed_accounts_trx = async actions => {
  const result = await api.transact(
    {
      actions: actions
    },
    {
      blocksBehind: 3,
      expireSeconds: 30
    }
  );
  await sleep(300)
};

const freezed_accounts_action = async (committer, accounts, memo) => {
  let actions = [];
  let accs = [];
  for (let account in accounts) {
    accs.push(accounts[account]);

    if (accs.length >= 64) {
      actions.push(get_freezed_accounts_action(committer, accs, memo));
      accs = [];
    }

    if (actions.length >= 6) {
      await push_freezed_accounts_trx(actions);
      actions = [];
    }
  }

  if (accs.length > 0) {
    actions.push(get_freezed_accounts_action(committer, accs, memo));
  }

  if (actions.length > 0) {
   await push_freezed_accounts_trx(actions);
  }
};

const accs = [
   "aaa1t21", "aaa1t22", "aaa1t23", "aaa1t24", "aaa1t25",
   "aaa1t11", "aaa1t12", "aaa1t13", "aaa1t14", "aaa1t15",
   "aaa1ta1", "aaa1ta2", "aaa1ta3", "aaa1ta4", "aaa1ta5",
   "aaa1tb1", "aaa1tb2", "aaa1tb3", "aaa1tb4", "aaa1tb5",
   "aaa1tc1", "aaa1tc2", "aaa1tc3", "aaa1tc4", "aaa1tc5",
   "aaa1td1", "aaa1td2", "aaa1td3", "aaa1td4", "aaa1td5",
   "aaa1te1", "aaa1te2", "aaa1te3", "aaa1te4", "aaa1te5",
   "aaa1tf1", "aaa1tf2", "aaa1tf3", "aaa1tf4", "aaa1tf5",
   "aaa1tg1", "aaa1tg2", "aaa1tg3", "aaa1tg4", "aaa1tg5",
   "aaa1th1", "aaa1th2", "aaa1th3", "aaa1th4", "aaa1th5",
   "aaa1ti1", "aaa1ti2", "aaa1ti3", "aaa1ti4", "aaa1ti5",
   "aaa1tj1", "aaa1tj2", "aaa1tj3", "aaa1tj4", "aaa1tj5",
   "aaa1tk1", "aaa1tk2", "aaa1tk3", "aaa1tk4", "aaa1tk5",
   "aaa1tl1", "aaa1tl2", "aaa1tl3", "aaa1tl4", "aaa1tl5",
   "aaa1tm1", "aaa1tm2", "aaa1tm3", "aaa1tm4", "aaa1tm5",
   "aaa1tn1", "aaa1tn2", "aaa1tn3", "aaa1tn4", "aaa1tn5",
   "aaa1to1", "aaa1to2", "aaa1to3", "aaa1to4", "aaa1to5",
   "aaa1tp1", "aaa1tp2", "aaa1tp3", "aaa1tp4", "aaa1tp5",
   "aaa1tq1", "aaa1tq2", "aaa1tq3", "aaa1tq4", "aaa1tq5",
   "aaa1tr1", "aaa1tr2", "aaa1tr3", "aaa1tr4", "aaa1tr5",
];

const back = [ "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z"]
const back2 = [ "1", "2", "3", "4", "5"]

const account_to_ban = []

add = 'a'

for (let i1 = 0; i1 < back.length; i1++) {
   for (let i2 = 0; i2 < back.length; i2++) {
      for (let i3 = 0; i3 < back2.length; i3++) {
         for (let ii in accs){
            back_str = back[i1] + back[i2] + back2[i3]
            account_to_ban.push(add + accs[ii] + back_str)
         }
      }
   }
}


freezed_accounts_action('testd', account_to_ban, 'freezed');
