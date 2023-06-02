#!/bin/bash

dev_path="/root/dev-test"
beta_path="/root/beta1"
v3_dev_path="/root/v3-dev-test"
v3_beta_path="/root/v3-beta"
build_path="../dist"
bundle="registry-index.tar.gz"

mode=$1
set_hook=$2

# Resolve dev and beta paths.
to_path=""
index_path=""
governor_address=""
if [[ $mode = "dev" ]]; then
    to_path="$dev_path"
    index_path="$to_path/registry-index"
    governor_address=""
elif [[ $mode = "v3-dev" ]]; then
    to_path="$v3_dev_path"
    index_path="$to_path/registry-index"
    governor_address="raVhw4Q8FQr296jdaDLDfZ4JDhh7tFG7SF"
# elif [[ $mode = "beta" ]]; then
#     to_path="$beta_path"
#     index_path="$to_path/registry-index"
#     governor_address=""
# elif [[ $mode = "v3-beta" ]]; then
#     to_path="$v3_beta_path"
#     index_path="$to_path/registry-index"
#     governor_address=""
else
    echo "Invalid mode"
    echo "Usage: deploy-service.sh <mode (dev|beta)> [arguments (--set-hook)]"
    exit 1
fi
index_data_path="$index_path/data"
index_bk_path="$index_path-bk"
index_bk_data_path="$index_bk_path/data"
bundle_path="$to_path/$bundle"

read -p "Enter the machine ip address: " ip </dev/tty
read -s -p "Enter the $ip root password (your input will be hidden on screen): " pword </dev/tty && echo ""

# Copy the bundle from local machine.
sshpass -p $pword scp "$build_path/$bundle" root@$ip:"$to_path/"

code="
echo 'Creating the backup.'
rm -rf $index_bk_path
cp -r $index_path $index_bk_path

echo 'Overriding the registry index the binaries.'
tar -xf $bundle_path --strip-components=1 -C $index_path

pushd $index_path

if [[ '$set_hook' == '--set-hook' ]]; then
    echo 'Performing set-hook.'
    ./run-registry-index.sh $governor_address set-hook
fi

echo 'Re-configuring the service'
./run-registry-index.sh $governor_address service-reconfig

popd
"

sshpass -p $pword ssh root@$ip "$code"

exit 0
