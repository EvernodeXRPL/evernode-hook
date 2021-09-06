#!/bin/sh
if [ "$1" = "start" ]; then
    echo "Starting hooks docker"
    docker run -d --name xrpld-hooks xrpllabsofficial/xrpld-hooks-testnet
    docker exec -it xrpld-hooks tail -f log | grep -a rLj1Ea4eeTdgAGoAXSCEk8dvqHewFeoeb7
elif [ "$1" = "stop" ]; then
    echo "stopping hooks docker"
    docker rm -f xrpld-hooks
fi