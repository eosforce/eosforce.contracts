#!/usr/bin/env python3

import argparse
import json
import os
import re
import subprocess
import sys
import time

eosforce_path = "~/Projects/eosforce/eosforce"

enable_push = False # True to push on chain
cleos = eosforce_path + '/build/programs/cleos/cleos --wallet-url http://127.0.0.1:6666 --url http://127.0.0.1:8001 '
wallet_password = ''
wallet_name = 'testc'
active_account = 'testc'

# eosio system contract code in wasm
wasm_path = '../build/contracts/eosio.system/eosio.system.wasm'
abi_path = '../build/contracts/eosio.system/eosio.system.abi'

tx_expire_hours = 120   # 5days

def getAbiData(path):
    run("rm -rf ./system_contract_abi.data")
    run("./abi2hex -abi " + path + " > ./system_contract_abi.data")

    time.sleep(.5)

    f = open("./system_contract_abi.data")
    line = f.readline()
    run("rm -rf ./system_contract_abi.data")
    return line

def jsonArg(a):
    return " '" + json.dumps(a) + "' "


def run(args):
    print('', args)
    if subprocess.call(args, shell=True):
        print(' exiting because of error')
        sys.exit(1)


def runone(args):
    print('', args)
    subprocess.call(args, shell=True)


def getOutput(args):
    print('', args)
    proc = subprocess.Popen(args, shell=True, stdout=subprocess.PIPE)
    return proc.communicate()[0].decode('utf-8')


def getJsonOutput(args):
    return json.loads(getOutput(args))


def getbps():
    bpsa = []
    bpsj = getJsonOutput(cleos + " get schedule -j ")
    for bp in bpsj["active"]["producers"]:
        bpsa.append(bp["producer_name"])
    return bpsa

# msig to update system contract code
def mkUpdateSystemActionDatas( wasmpath ):
    with open(wasmpath, mode='rb') as f:
        setcode = {'account': 'eosio', 'vmtype': 0, 'vmversion': 0, 'code': f.read().hex()}
    return setcode


# msig to update system contract abi
def mkUpdateSystemAbiActionDatas( abi_path ):
    return {
        "account": "eosio",
        "abi": getAbiData(abi_path)
    }

def dumpActionData( contract, action_name, datas ):
    flie_name = './%s.%s.json' % (contract, action_name)
    run(('rm -rf %s' % (flie_name)))
    with open(flie_name, mode='w') as f:
        f.write(json.dumps(datas))

def mkAction( wasm_path, abi_path ):
    dumpActionData( 'eosio', 'setcode', mkUpdateSystemActionDatas(wasm_path) )
    dumpActionData( 'eosio', 'setabi', mkUpdateSystemAbiActionDatas(abi_path) )

# msig to update system contract abi
def execMultisig(proposer, bps):
    expirehours = tx_expire_hours
    requestedPermissions = []
    for i in range(0, len(bps)):
        requestedPermissions.append({'actor': bps[i], 'permission': 'active'})
    trxPermissions = [{'actor': 'eosio', 'permission': 'active'}]

    run(cleos + 'multisig propose p.upsysabi' + jsonArg(requestedPermissions) +
        jsonArg(trxPermissions) + ' eosio setabi ./eosio.setabi.json ' + proposer + ' ' + str(expirehours) + ' -p ' + proposer)

    run(cleos + 'multisig propose p.upsyscode' + jsonArg(requestedPermissions) +
        jsonArg(trxPermissions) + ' eosio setcode ./eosio.setcode.json ' + proposer + ' ' + str(expirehours) + ' -p ' + proposer)

# ---------------------------------------------------------------------------------------------------
# msig to update system contract

# unlock wallet
unlockwallet_str = 'cleos wallet unlock -n ' + wallet_name + ' --password ' + wallet_password
runone(unlockwallet_str)

run(cleos + ('transfer %s eosio "1000.0000 EOS" ""' % (active_account)))
run(cleos + ('push action eosio vote4ram \'{"voter":"%s","bpname":"biosbpa","stake":"5000.0000 EOS"}\' -p %s' % (active_account, active_account)))

mkAction( wasm_path, abi_path )
execMultisig(active_account, getbps())
