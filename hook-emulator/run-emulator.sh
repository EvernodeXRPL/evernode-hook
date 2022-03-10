#!/bin/bash
account_config="accounts.json"
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

# If there's no account config file. The account aren't created,
# So run account setup to create accounts and populate the data.
if [ ! -f $account_config ]; then
    EMULATOR_DATA_DIR=$data_dir node $account_setup
fi

# Run the emulator with working directory setting data and binary directories.
DATA_DIR=$data_dir BIN_DIR=$bin_dir node $emulator
