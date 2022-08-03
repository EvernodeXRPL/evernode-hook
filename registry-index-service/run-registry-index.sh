#!/bin/bash
hook_data_dir="data"
config="accounts.json"
account_setup="account-setup"
hook_setup="hook-setup"
index="index.js"
data_dir=$(pwd)

# If index does not exist in current directory, this is a dev execution.
# Setup the dev repo paths.
if [ ! -f $index ]; then
    account_setup="dist/account-setup"
    hook_setup="dist/hook-setup"
    index="index-manager.js"
fi

wasm_path="$hook_setup/evernode.wasm"

arg1=$1
arg2=$2
if ([ ! -z $arg1 ] && [[ ! "$arg1" =~ ^r[a-zA-Z0-9]{24,34}$ ]]) ||
    ([ ! -z $arg2 ] && [ "$arg2" != "service-start" ] && [ "$arg2" != "service-stop" ] &&
        [ "$arg2" != "service-rm" ] && [ "$arg2" != "rm" ] && [ "$arg2" != "set-hook" ] && [ "$arg2" != "recover-start" ]); then # Check for commandline params.
    echo "Invalid arguments: Usage \"./run-registry-index [registry address] [command]\""
    echo "Optional registry address: Registry address for registry index"
    echo "                           If empty instantiate a new registry index instance"
    echo "Optional command: set-hook - Upload the hook and send init transaction"
    echo "                  recover-start - Starts registry index in terminal session and perform a bulk update for all the hook states"
    echo "                  service-start - Initiate and start systemd service for registry index"
    echo "                  service-stop - Stop the systemd service"
    echo "                  service-rm - Remove the systemd service"
    echo "                  rm - Remove systemd service, registry index instance and its resources"
    echo "                  If empty starts registry index in terminal session"
    exit 1
fi

service="registry-index-$arg1"
systemd_file="/etc/systemd/system/$service.service"

if [ -z "$arg1" ]; then # If 1st param is empty, Create a new instance.
    [ ! -d "$hook_data_dir" ] && mkdir "$hook_data_dir"
    ! ACCOUNT_DATA_DIR=$data_dir $(which node) $account_setup && echo "Account setup faild." && exit 1
    arg1=$(ls -t $hook_data_dir/ | head -1)
    ! CONFIG_PATH=$hook_data_dir/$arg1/$config WASM_PATH=$wasm_path $(which node) $hook_setup && echo "Hook setup faild." && exit 1
    sleep 2 # Sleep for 2 sec so all the pre required setup is completed before start the index service.
elif [ ! -d "$hook_data_dir/$arg1" ]; then # If 1st param is given check for data directory existance.
    echo "Data directory $hook_data_dir/$arg1 does not exist: Run \"./run-registry-index\" to create a new instance."
    exit 1
elif [ ! -z "$arg2" ]; then # If 2nd param is given.
    if ([ "$arg2" == "service-start" ] || [ "$arg2" == "service-stop" ] || [ "$arg2" == "service-rm" ] || [ "$arg2" == "rm" ]) &&
        [ $(id -u) -ne 0 ]; then # Check for root permissions.
        echo "\"./run-registry-index $arg1 $arg2\" must run as root"
        exit 1
    elif [ "$arg2" == "service-start" ]; then
        if [ ! -f "$systemd_file" ]; then # Create a systemd service if not exist.
            echo "[Unit]
                Description=Registry index service for address $arg1
                After=network.target
                StartLimitIntervalSec=0
                [Service]
                Type=simple
                User=root
                Group=root
                WorkingDirectory=$(pwd)
                Environment=\"DATA_DIR=$data_dir\"
                ExecStart=$(which node) $index $arg1
                Restart=on-failure
                RestartSec=5
                [Install]
                WantedBy=multi-user.target" >$systemd_file
            systemctl daemon-reload
            systemctl enable $service
            echo "Created systemd service $service"
        fi
        systemctl start $service
        echo "Started systemd service $service"
    elif [ "$arg2" == "service-stop" ]; then
        [ ! -f "$systemd_file" ] && echo "Systemd service not found, Create a service with 'service-start'." && exit 1
        systemctl stop $service
        echo "Stopped systemd service $service"
    elif [ "$arg2" == "service-rm" ] || [ "$arg2" == "rm" ]; then # If service-rm or rm given, Handle removes.
        if [ -f "$systemd_file" ]; then
            systemctl stop $service
            systemctl disable $service
            rm -r $systemd_file && systemctl daemon-reload
            echo "Removed systemd service $service"
        elif [ "$arg2" == "service-rm" ]; then
            echo "Systemd service not found." && exit 1
        fi
        [ "$arg2" == "rm" ] && rm -r $hook_data_dir/$arg1 && echo "Removed instance $arg1"
    elif [ "$arg2" == "set-hook" ]; then
        ! CONFIG_PATH=$hook_data_dir/$arg1/$config WASM_PATH=$wasm_path $(which node) $hook_setup && echo "Hook setup faild." && exit 1
        echo "Hook setup successful."
    elif [ "$arg2" == "recover-start" ]; then
        DATA_DIR=$data_dir ACTION=recover $(which node) $index $arg1 || echo "Recover starting registry index failed."
        echo "Recover starting registry index successful."
    fi
    exit 0
fi

DATA_DIR=$data_dir $(which node) $index $arg1

exit 0
