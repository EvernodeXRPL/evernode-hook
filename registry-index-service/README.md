# registry-index-service
## Evernode Registry account index management service.

### Generating the firebase key file for state index
- Go to [firebase console](https://console.firebase.google.com).
- Choose your project.
- Go to `Project Settings`.
- Go to `Service Accounts` tab.
- Download a key json file with `Generate new private key` button.
- Place it under `service-acc` directory as `./registry-index-service/service-acc/firebase-sa-key.json`.

### To run the Node.js script
- Run `node index-manager.js <evernode Registry address>`
- In addtion to that, you can pass environmental variables such as,
    - `ACTION=recover` - To perform a bulk update
