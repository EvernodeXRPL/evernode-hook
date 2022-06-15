.PHONY: all build

all: build upload

build:
	mkdir -p build
	wasmcc ./src/evernode.c -o ./build/evernode.wasm -O2 -Wl,--allow-undefined -I../
	hook-cleaner ./build/evernode.wasm

upload:
	node sethook.js

clean:
	rm -rf build/*