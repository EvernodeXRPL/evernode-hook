#!/bin/bash
reg_data_dir="reg-data"
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

param=$1
if [ "$param" == "new" ]; then
    EMULATOR_DATA_DIR=$data_dir $(which node) $account_setup
    param=$(ls -t $reg_data_dir/ | head -1)
elif [[ ! "$param" =~ ^r[a-zA-Z0-9]{24,34}$ ]]; then
    echo 'Invalid arguments: use "./run-emulator new|<registry address>"'
    exit 1
fi

DATA_DIR=$data_dir BIN_DIR=$bin_dir $(which node) $emulator $param

exit 0
