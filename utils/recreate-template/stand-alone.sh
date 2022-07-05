#!/bin/sh
if [ "$1" = "create" ]; then
    echo "Creating hooks docker"
    docker create -p 6005:6005 --name xrpld-hooks xrpllabsofficial/xrpld-hooks-testnet bash -c "./rippled -a --start >> log 2>> log"
elif [ "$1" = "start" ]; then
    echo "Starting hooks docker"
    address='rBtpWXY7e7GHYR4YceF4dG4jcUYiemFzci'
    docker start xrpld-hooks
    docker exec -it xrpld-hooks tail -f log | grep -a $address
elif [ "$1" = "stop" ]; then
    echo "Stopping hooks docker"
    docker stop xrpld-hooks
elif [ "$1" = "remove" ]; then
    echo "Removing hooks docker"
    docker rm -f xrpld-hooks
fi
