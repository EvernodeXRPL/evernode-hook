# evernode-hook
XRPL hook for Evernode.

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
* Note: `make upload` will create a hook.cfg file with default hook address and secret. This can be updated to change your hook account.
