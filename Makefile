.PHONY: all build

all: build upload

build:
	mkdir -p build
	wasmcc ./src/evernode.c -o ./build/evernode.wasm -O0 -Wl,--allow-undefined -I../
	wasm-opt -O2 ./build/evernode.wasm -o ./build/evernode.wasm
	hook-cleaner ./build/evernode.wasm

upload:
	node sethook.js

clean:
	rm -rf build/*