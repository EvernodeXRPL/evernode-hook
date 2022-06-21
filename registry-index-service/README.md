# registry-index-service
## Evernode Registry account index management service.

### Generating the firebase key file for state index
- Go to [firebase console](https://console.firebase.google.com).
- Choose your project.
- Go to `Project Settings`.
- Go to `Service Accounts` tab.
- Download a key json file with `Generate new private key` button.
- Place it under `service-acc` directory as `./registry-index-service/service-acc/firebase-sa-key.json`.

### How to setup and run
- Go to `registry-index-service` directory.
- Run `npm install` to install necessary dependencies.
- Run `npm run build` to build the hook binary and emulator bundle.
- Steps to run the emulator.
  - Run `./run-registry-index.sh` to generate and instantiate a new registry index instance, The generated account config will be placed in the `data` directory.
  - Run `./run-registry-index [registry address]` to start the instance with generated "registry address".
  - Run `sudo ./run-registry-index [registry address] set-hook` to upload the latest hook to the registry xrpl account.
  - Run `sudo ./run-registry-index [registry address] recover-start` to start registry index in terminal session and perform a bulk update for all the hook states.
  - Run `sudo ./run-registry-index [registry address] service-start` with root privileged mode to initiate and start systemd service for registry index.
  - Run `sudo ./run-registry-index [registry address] service-stop` with root privileged mode to stop the systemd service.
  - Run `sudo ./run-registry-index [registry address] service-rm` with root privileged mode to remove the systemd service.
  - Run `sudo ./run-registry-index [registry address] rm` with root privileged mode to remove systemd service, registry index instance and its resources.
  - Note: the systemd service name will be `registry-index-[registry address]`.
- Running registry-index on dev mode.
  - Follow the "Steps to run the emulator." from the repository's `registry-index-service` directory.
- Running registry-index on portable mode.
  - Run `npm run bundle` to build the hook binary and generate the portable registry-index compressed bundle.
  - Extract the `dist/registry-index.tar.gz` bundle to any location.
  - Go to the extracted location and follow the "Steps to run the emulator.".
- In addtion to that, you can pass environmental variables such as,
  - `MODE=recover` - To perform a bulk update.
