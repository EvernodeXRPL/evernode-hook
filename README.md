# evernode-hook
XRPL hook for Evernode.

# Dev
Use `sudo apt-get install gcc-multilib` to install 32 bit gcc header files which is needed to compile hook as a 32 bit binary.

## Build
Run `make build` to build the hook binary and upload it to the hook account.

```bash
make build
```

Run `sethook.js` to upload the binary to the hook account.
```bash
node sethook.js
```

* Note: `make` will build and upload the binary at the same time.
* Note: `make upload` will create a hook.json file with default hook address and secret. This can be updated to change your hook account.
