#!/bin/sh
if [ "$1" = "start" ]; then
    echo "Starting hooks docker"
    docker run -d --name xrpld-hooks xrpllabsofficial/xrpld-hooks-testnet
    docker exec -it xrpld-hooks tail -f log | grep -a rB9gfgPcZ9aS1WF2D1MPS8Gdg8bs2y8aEK
elif [ "$1" = "stop" ]; then
    echo "stopping hooks docker"
    docker rm -f xrpld-hooks
fi