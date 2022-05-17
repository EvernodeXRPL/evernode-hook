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
    echo "Usage: deploy-emulator.sh <Mode (dev|beta)>"
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
echo 'Creating the backup.'
rm -rf $emulator_bk_path
cp -r $emulator_path $emulator_bk_path

echo 'Overriding the emulator the binaries.'
tar -xf $bundle_path --strip-components=1 -C $emulator_path

echo 'Restarting the service.'
systemctl restart hook-emulator-$registry_address.service
"

sshpass -p $pword ssh root@$ip "$code"

exit 0
