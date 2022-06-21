#!/bin/bash
hook_data_dir="data"
config="accounts.json"
log="hook-emulator.log"
account_setup="account-setup"
emulator="index.js"
data_dir=$(pwd)
bin_dir=$(pwd)

# If emulator does not exist in current directory, this is a dev execution.
# Setup the dev repo paths.
if [ ! -f $emulator ]; then
    account_setup="dist/account-setup"
    emulator="hook-emulator.js"
    bin_dir="$bin_dir/dist"
fi

arg1=$1
arg2=$2
if ([ ! -z $arg1 ] && [[ ! "$arg1" =~ ^r[a-zA-Z0-9]{24,34}$ ]]) ||
    ([ ! -z $arg2 ] && [ "$arg2" != "service-start" ] && [ "$arg2" != "service-stop" ] &&
        [ "$arg2" != "service-rm" ] && [ "$arg2" != "rm" ]); then # Check for commandline params.
    echo "Invalid arguments: Usage \"./run-emulator [registry address] [command]\""
    echo "Optional registry address: Registry address for hook emulator"
    echo "                           If empty instantiate a new hook emulator instance"
    echo "Optional command: service-start - Initiate and start systemd service for hook emulator"
    echo "                  service-stop - Stop the systemd service"
    echo "                  service-rm - Remove the systemd service"
    echo "                  rm - Remove systemd service, emulator instance and its resources"
    echo "                  If empty starts emulator in terminal session"
    exit 1
fi

service="hook-emulator-$arg1"
systemd_file="/etc/systemd/system/$service.service"

if [ -z "$arg1" ]; then # If 1st param is empty, Create a new instance.
    ACCOUNT_DATA_DIR=$data_dir $(which node) $account_setup
    arg1=$(ls -t $hook_data_dir/ | head -1)
elif [ ! -d "$hook_data_dir/$arg1" ]; then # If 1st param is given check for data directory existance.
    echo "Data directory $hook_data_dir/$arg1 does not exist: Run \"./run-emulator\" to create a new instance."
    exit 1
elif [ ! -z "$arg2" ]; then     # If 2nd param is given.
    if [ $(id -u) -ne 0 ]; then # Check for root permissions.
        echo "\"./run-emulator $arg1 $arg2\" must run as root"
        exit 1
    elif [ "$arg2" == "service-start" ]; then
        if [ ! -f "$systemd_file" ]; then # Create a systemd service if not exist.
            echo "[Unit]
                Description=Hook emulator for registry address $arg1
                After=network.target
                StartLimitIntervalSec=0
                [Service]
                Type=simple
                User=root
                Group=root
                WorkingDirectory=$(pwd)
                Environment=\"DATA_DIR=$data_dir\"
                Environment=\"BIN_DIR=$bin_dir\"
                ExecStart=$(which node) $emulator $arg1
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
    fi
    exit 0
fi

DATA_DIR=$data_dir BIN_DIR=$bin_dir $(which node) $emulator $arg1

exit 0
