#!/bin/bash
# Usage ./dev-setup.sh
# Evernode hook build environment setup script.

# Installing puython and pip.
sudo apt install python3
sudo apt-get -y install python3-pip

tmp=$(mktemp -d)
cd $tmp

# Installing web assembly compiler.
curl https://raw.githubusercontent.com/wasienv/wasienv/master/install.sh | bash
source /home/chalith/.wasienv/wasienv.sh

# Installing binaryen.
! command -v wasm-opt /dev/null && sudo apt install binaryen

# Installing hook-cleaner.
git clone https://github.com/RichardAH/hook-cleaner-c.git
cd hook-cleaner-c
make
mv hook-cleaner /usr/bin/