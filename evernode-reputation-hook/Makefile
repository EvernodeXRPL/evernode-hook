.PHONY: all build

all: build upload

build:
	mkdir -p build
	wasmcc ./src/reputation.c -o ./build/reputation.wasm -O0 -Wl,--allow-undefined -I../
	wasm-opt -O2 ./build/reputation.wasm -o ./build/reputation.wasm
	hook-cleaner ./build/reputation.wasm

upload:
	node sethook.js

clean:
	rm -rf build/*