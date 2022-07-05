#!/bin/bash
while true; do
    docker exec -it xrpld-hooks bash -c "/opt/xrpld-hooks/rippled ledger_accept --conf /opt/xrpld-hooks/testnet.cfg"
    sleep 5
done