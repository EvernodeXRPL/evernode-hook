.PHONY: all build

all: build upload

build:
	mkdir -p build
	wasmcc ./src/evernodezero.c -o ./build/evernodezero.wasm -O0 -Wl,--allow-undefined -I../
	wasm-opt -O2 ./build/evernodezero.wasm -o ./build/evernodezero.wasm
	hook-cleaner ./build/evernodezero.wasm

	wasmcc ./src/evernodeone.c -o ./build/evernodeone.wasm -O0 -Wl,--allow-undefined -I../
	wasm-opt -O2 ./build/evernodeone.wasm -o ./build/evernodeone.wasm
	hook-cleaner ./build/evernodeone.wasm

	wasmcc ./src/evernodetwo.c -o ./build/evernodetwo.wasm -O0 -Wl,--allow-undefined -I../
	wasm-opt -O2 ./build/evernodetwo.wasm -o ./build/evernodetwo.wasm
	hook-cleaner ./build/evernodetwo.wasm

upload:
	node sethook.js

clean:
	rm -rf build/*