.PHONY: all

all:
	mkdir -p build
	wasmcc ./hook.c -o ./build/hook.wasm -O0 -Wl,--allow-undefined -I../
	wasm-opt -O2 ./build/hook.wasm -o ./build/hook.wasm
	hook-cleaner ./build/hook.wasm

clean:
	rm -rf build/*