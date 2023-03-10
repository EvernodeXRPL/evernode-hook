.PHONY: all build

all: build upload

build:
	mkdir -p build
	wasmcc ./src/registry.c -o ./build/registry.wasm -O0 -Wl,--allow-undefined -I../
	wasm-opt -O2 ./build/registry.wasm -o ./build/registry.wasm
	hook-cleaner ./build/registry.wasm

upload:
	node sethook.js

clean:
	rm -rf build/*