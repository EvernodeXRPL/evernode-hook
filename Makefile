.PHONY: all build

all: build upload

build:
	mkdir -p build
	docker run --rm -v "$(shell pwd)":"$(shell pwd)" --entrypoint /root/.wasienv/bin/wasmcc xrpllabsofficial/xrpld-hooks-testnet "$(shell pwd)"/src/test.c -o "$(shell pwd)"/build/evernode.wasm -O0 -Wl,--allow-undefined -I../

upload:
	node sethook.js

clean:
	rm -rf build/*