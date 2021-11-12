# evernode-hook
Evernode brings "layer 2" smart contracts to the [XRP Ledger](https://github.com/ripple/rippled#readme), It is powered by a network of peer-to-peer servers that run HotPocket, HotPocket is a UNL consensus protocol that allows nodes to act as a mini-blockchain reserved to run and compute scalable smart contracts at an affordable cost.

## Evers
Evers are the proposed native currency of Evernode. Evers would be a currency that live on the XRP Ledger, Ever's distribution would be governed by a new form of mini-smart contract on the XRP Ledger called a [Hook](https://hooks-testnet.xrpl-labs.com/).

## Evernode 
Evernode consist of four main components that make it possible to execute flexible smart contracts: 

 - [**Consensus Protocol**](https://evernode.files.wordpress.com/2021/08/hotpocket-white-paper.pdf): HotPocket is a Unique Node List (UNL) based consensus protocol that allows multiple machines to become a mini-blockchain by enforcing consensus rules on inputs and outputs and maintaining a shared, canonical state.
 - [**Native Currency**](https://evernode.files.wordpress.com/2021/08/evernode-whitepaper-1.0.pdf): Evers is the native digital currency of Evernode, Evers is an XRP Ledger currency distributed by a Hook to reward Nodes for participating in the network and used by dApps to pay hosting fees to Nodes
 - [**Evernode Hook**](https://github.com/HotPocketDev/evernode-hook): A Hook on the XRP Ledger that controls the emission of Evers and confirms each Node’s membership in Evernode, their willingness to host dApps, and their agreement to be paid in Evers.
 - [**DEX**](https://xrpl.org/decentralized-exchange.html#main-page-header): The XRP Ledger’s native decentralised exchange which Evernode utilizes to automate the buying and selling of hosting services with Evers. 

In concert, these 4 components create Evernode: a reliable, secure, and decentralised “network-of-networks” of HotPocket nodes piggybacking off the XRP Ledger that anyone can join, capable of running a wide variety of cheap, scalable and fast dApps that is mainted 24/7.


## Build
Run `make build` to build the hook binary and upload it to the hook account.

```bash
make build
```

Run `sethook.js` to upload the binary to the hook account.
```bash
node sethook.js
```

* Note: `make` will build and upload the binary at the same time.
* Note: `make upload` will create a hook.cfg file with default hook address and secret. This can be updated to change your hook account.
