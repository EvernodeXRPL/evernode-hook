1. Generate faucet accounts for heartbeat and governor hooks.
2. Populate `accounts.json` file with current registry and newly generated heartbeat and governor hook account details.
3. Run `npm run migrate` to migrate hook states from registry to governor.
   1. This will create trustlines and reward EVR transfers as well.
4. Run `npm run clear` to clear hook states from registry.
5. Update `data/<registry_address>/accounts.json` file in registry index service directory with new heartbeat and governor account details.
6. Rename `data/<registry_address>` to `data/<governor_address>/`.
7. Run `./run-registry-index.sh <governor_address> set-hook` to upload and setup new hook binaries.
   1. For deployments,
   2. `npm run bundle` the `registry-index-service`
   3. Go to `test` directory and run `./deploy-service.sh <dev|beta>`