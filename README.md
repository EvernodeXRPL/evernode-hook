# evernode-hook
XRPL hook for Evernode.

# Preparation
- Use `sudo apt-get install gcc-multilib` to install 32 bit gcc header files which is needed to compile hook as a 32 bit binary.
- Run `./dev-setup.sh` to install prerequisites.
- Create hook account configuration file named `hook.json` in the evernode-hook directory with following format and populate the accounts and network field.
  ```
    {
        "governor": {
            "address": "",
            "secret": ""
        },
        "registry": {
            "address": "",
            "secret": ""
        },
        "heartbeat": {
            "address": "",
            "secret": ""
        },
        "network": ""
    }
  ```
  - _**NOTE:** If you intend to build and upload a specific hook instead of all three hooks, you can define the `hook.json` file within the directory of that specific hook. In this case, you can omit the account data related to the other two hooks from the file._

- XRPL.js npm library will not support the server definitions on `hooks-testnet-v3`, So you need to replace the server definitions in `xrpl-binary-codec`.

## Build
- Run `make build` to build the hook binaries.
- Run `make upload` to upload hook to the account.
- Run `make grant` grant the governance state access.
- Run `make clean` to clear the build directories.
- Running `make` will execute build upload and grant steps.


