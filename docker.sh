#!/bin/sh
config='config.json'
if [ "$1" = "start" ]; then
    echo "Starting hooks docker"
    [ ! -f $config ] && echo "$config does not exist, Run 'make' first." && exit 1
    address=$(jq -r '.address' $config)
    [ -z $address ] && echo "Populate the hook address in the $config and try again." && exit 1
    docker run -d --name xrpld-hooks xrpllabsofficial/xrpld-hooks-testnet
    docker exec -it xrpld-hooks tail -f log | grep -a $address
elif [ "$1" = "stop" ]; then
    echo "stopping hooks docker"
    docker rm -f xrpld-hooks
fi
