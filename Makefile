mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
current_dir := $(patsubst %/,%,$(dir $(mkfile_path)))

.PHONY: all build

all: registry-build governor-build registry-upload governor-upload grant

build: registry-build governor-build

upload: registry-upload governor-upload

grant:
	node setgrant.js

governor-build:
	make build -C ./evernode-governor-hook CONFIG_PATH=${current_dir}/hook.json

registry-build:
	make build -C ./evernode-registry-hook CONFIG_PATH=${current_dir}/hook.json

registry-upload:
	make upload -C ./evernode-governor-hook CONFIG_PATH=${current_dir}/hook.json

governor-upload:
	make upload -C ./evernode-registry-hook CONFIG_PATH=${current_dir}/hook.json

clean:
	make clean -C ./evernode-governor-hook
	make clean -C ./evernode-registry-hook