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

# If there's no at least on account config file. The account aren't created,
# So run account setup to create accounts and populate the data.
if [ ! -d $reg_data_dir ] || [ -z "$(ls -A $reg_data_dir)" ]; then
    EMULATOR_DATA_DIR=$data_dir $(which node) $account_setup
fi

# Run the emulator with data directories and registry address.
for reg_addr in $reg_data_dir/*; do
    # gnome-terminal -- bash -c "DATA_DIR=$data_dir BIN_DIR=$bin_dir $(which node) $emulator $(basename $reg_addr)" &
done
