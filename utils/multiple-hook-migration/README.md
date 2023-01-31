1. Generate faucet accounts for heartbeat and governor hooks.
2. Populate `accounts.json` file with current registry and newly generated heartbeat and governor hook account details.
3. Run `npm run migrate` to migrate hook states from registry to governor.
   1. This will create trustlines and reward EVR transfers as well.
4. Run `npm run clear` to clear hook states from registry.
5. Update `data/<registry_address>/accounts.json` file in registry index service directory with new heartbeat and governor account details.
6. Rename `data/<registry_address>` to `data/<governor_address>/`.
7. `npm run bundle` the `registry-index-service`.
8. For development,
   1. Replace the binaries exept the `data` directory in the registry-index from the generated bundle.
   2.  Run `./run-registry-index.sh <governor_address> set-hook` to upload and setup new hook binaries.
9. For deployments,
   1. Go to `test` directory and add generated governor address to the `deploy-service.sh`.
   2. Run `./deploy-service.sh <dev|beta>`.