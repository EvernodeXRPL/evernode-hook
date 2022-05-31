# registry-index-service
## Evernode Registry account index management service.

### Generating the firebase key file for state index
- Go to [firebase console](https://console.firebase.google.com).
- Choose your project.
- Go to `Project Settings`.
- Go to `Service Accounts` tab.
- Download a key json file with `Generate new private key` button.
- Place it under `service-acc` directory as `./registry-index-service/service-acc/firebase-sa-key.json`.
- If your system is 64-bit, You need to have 32-bit support in you system to run emulator wrappers.
- 32-bit support can be installed with `apt-get install libc6-i386`.
