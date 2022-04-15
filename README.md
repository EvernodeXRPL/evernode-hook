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
* Note: `make upload` will create a hook.cfg file with default hook address and secret. This can be updated to change your hook account.

## Hook emulator
- Go to `hook-emulator` directory.
- Run `npm install` to install necessary dependencies.
- Run `npm run build` to build the hook binary and emulator bundle.
- Steps to run the emulator.
  - Run `./run-emulator.sh` to generate and instantiate a new hook emulator instance, The generated account config will be placed in the `data` directory.
  - Run `./run-emulator [registry address]` to start the instance with generated "registry address".
  - Run `sudo ./run-emulator [registry address] service-start` with root privileged mode to initiate and start systemd service for hook emulator.
  - Run `sudo ./run-emulator [registry address] service-stop` with root privileged mode to stop the systemd service.
  - Run `sudo ./run-emulator [registry address] service-rm` with root privileged mode to remove the systemd service.
  - Run `sudo ./run-emulator [registry address] rm` with root privileged mode to remove systemd service, emulator instance and its resources.
  - Note: the systemd service name will be `hook-emulator-[registry address]`.
- Running emulator on dev mode.
  - Follow the "Steps to run the emulator." from the repository's `hook-emulator` directory.
- Running emulator on portable mode.
  - Run `npm run bundle` to build the hook binary and generate the portable hook-emulator compressed bundle.
  - Extract the `dist/emulator.tar.gz` bundle to any location.
  - Go to the extracted location and follow the "Steps to run the emulator.".
- Run `node hook-emulator.js`.
  - First run will fail since there's no config file.
  - This will create `hook-emulator.cfg` in first run.
  - Update config with hook address and secret.
  - Then re run `node hook-emulator.js`.

### Generating the firebase key file for state index
- Go to [firebase console](https://console.firebase.google.com).
- Choose your project.
- Go to `Project Settings`.
- Go to `Service Accounts` tab.
- Download a key json file with `Generate new private key` button.
- Place it under `sec` directory as `./hook-emulator/sec/firebase-sa-key.json`.
- If your system is 64-bit, You need to have 32-bit support in you system to run emulator wrappers.
- 32-bit support can be installed with `apt-get install libc6-i386`.
