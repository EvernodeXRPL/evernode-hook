#!/bin/bash

dev_path="/root/dev-test"
beta_path="/root/beta2"
build_path="../dist"
bundle="registry-index.tar.gz"

mode=$1

# Resolve dev and beta paths.
to_path=""
index_path=""
registry_address=""
if [[ $mode = "dev" ]]; then
    to_path="$dev_path"
    index_path="$to_path/registry-index"
    registry_address="raaFre81618XegCrzTzVotAmarBcqNSAvK"
elif [[ $mode = "beta" ]]; then
    to_path="$beta_path"
    index_path="$to_path/registry-index"
    registry_address="r3cNR2bdao1NyvQ5ZuQvCUgqkoWGmgF34E"
else
    echo "Invalid mode"
    echo "Usage: deploy-service.sh <Mode (dev|beta)>"
    exit 1
fi
index_data_path="$index_path/data"
index_bk_path="$index_path-bk"
index_bk_data_path="$index_bk_path/data"
bundle_path="$to_path/$bundle"

echo "Enter the machine ip address:"
read ip </dev/tty
echo "Enter the $ip root password:"
read pword </dev/tty

# Copy the bundle from local machine.
sshpass -p $pword scp "$build_path/$bundle" root@$ip:"$to_path/"

code="
echo 'Creating the backup.'
rm -rf $index_bk_path
cp -r $index_path $index_bk_path

echo 'Overriding the registry index the binaries.'
tar -xf $bundle_path --strip-components=1 -C $index_path

pushd $index_path

echo 'Performing set-hook.'
./run-registry-index.sh $registry_address set-hook

echo 'Re-configuring the service'
./run-registry-index.sh $registry_address service-reconfig

popd

echo 'Restarting the service.'
systemctl restart registry-index-$registry_address.service
"

sshpass -p $pword ssh root@$ip "$code"

exit 0
