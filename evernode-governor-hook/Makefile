.PHONY: all build

all: build upload

build:
	mkdir -p build
	wasmcc ./src/governor.c -o ./build/governor.wasm -O0 -Wl,--allow-undefined -I../
	wasm-opt -O2 ./build/governor.wasm -o ./build/governor.wasm
	hook-cleaner ./build/governor.wasm

upload:
	node sethook.js

clean:
	rm -rf build/*