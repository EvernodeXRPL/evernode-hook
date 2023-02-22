# evernode-hook
XRPL hook for Evernode.

# Preparation
- Use `sudo apt-get install gcc-multilib` to install 32 bit gcc header files which is needed to compile hook as a 32 bit binary.
- Run `./dev-setup.sh` to install prerequisites.
- Create hook account configuration file with following format and populate the accounts.
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
    }
    }
```

## Build
- Run `make build` to build the hook binaries.
- Run `make upload` to upload hook to the account.
- Run `make grant` grant the governance state access.
- Run `make clean` to clear the build directories.
- Running `make` will execute build upload and grant steps.


