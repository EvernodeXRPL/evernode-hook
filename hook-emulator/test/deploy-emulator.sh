#!/bin/bash

dev_path="/root/dev-test"
beta_path="/root/beta1"
build_path="../dist"
bundle="emulator.tar.gz"

mode=$1

# Resolve dev and beta paths.
to_path=""
emulator_path=""
registry_address=""
if [[ $mode = "dev" ]]; then
    to_path="$dev_path"
    emulator_path="$to_path/emulator"
    registry_address="rDsg8R6MYfEB7Da861ThTRzVUWBa3xJgWL"
elif [[ $mode = "beta" ]]; then
    to_path="$beta_path"
    emulator_path="$to_path/hook-emulator"
    registry_address="rHQQq5aJ5kxFyNJXE36rAmuhxpDvpLHcWq"
else
    echo "Invalid mode"
    exit 1
fi
emulator_data_path="$emulator_path/data"
emulator_bk_path="$emulator_path-bk"
emulator_bk_data_path="$emulator_bk_path/data"
bundle_path="$to_path/$bundle"

echo "Enter the machine ip address:"
read ip </dev/tty
echo "Enter the $ip root password:"
read pword </dev/tty

# Copy the bundle from local machine.
sshpass -p $pword scp "$build_path/$bundle" root@$ip:"$to_path/"

code="
echo 'Stopping the emulator service.'
systemctl stop hook-emulator-$registry_address.service

echo 'Creating the backup.'
rm -rf emulator_bk_path
mv $emulator_path $emulator_bk_path

echo 'Extracting the bundle.'
mkdir $emulator_path
tar -xf $bundle_path --strip-components=1 -C $emulator_path

echo 'Moving the data from backup to new emulator.'
cp -r $emulator_bk_data_path $emulator_data_path

echo 'Starting the service.'
systemctl start hook-emulator-$registry_address.service
"

sshpass -p $pword ssh root@$ip "$code"

exit 0
