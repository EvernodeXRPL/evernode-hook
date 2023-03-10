.PHONY: all build

all: build upload

build:
	mkdir -p build
	wasmcc ./src/heartbeat.c -o ./build/heartbeat.wasm -O0 -Wl,--allow-undefined -I../
	wasm-opt -O2 ./build/heartbeat.wasm -o ./build/heartbeat.wasm
	hook-cleaner ./build/heartbeat.wasm

upload:
	node sethook.js

clean:
	rm -rf build/*