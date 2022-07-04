# Hooks error re-creating template

- This template is intented to reduce the time taken to create reproduction steps of a hooks issue.
- This script will assume there is a standalone node is running locally.
- It will create all the accounts and trustlines required from the genesis ledger.
- Then it will upload the hook given in hook folder to the created registry account.
- Finally the error creating transaction can be added so it will re-create the issue locally.

## Accounts
    issuer: {
        address: "rP9bHYzzH91wW51L4VMCZgMUxWfMxzMyBK"
    },
    foundation: {
        address: "rwEVcdR7HMdJtCpcwHFHWe4AnzaPjAaph4",
        secret: "ssma2d2ySK1RbtPoGhNC48KamFRaY"
    },
    registry: {
        address: "rBtpWXY7e7GHYR4YceF4dG4jcUYiemFzci",
        secret: "shrvS4mJAFFeiZmhkrGGRmwQaaLmR"
    },
    host: {
        address: "r3PKGU5Uzbck3nSXzjoUiK4wiJT7eCwUxC",
        secret: "snPyvMBLESA3hoAb6RCpwDEMDkrUr"
    },
    tenant: {
        address: "reL4CVVPZFSkr6SXdHRxjEXDt5crhwbcE",
        secret: "snBKhh1hkpNaWLLNmtqk1QZs92nZh"
    }

# Standalone mode
- For the ease of use, Scripts are added to run the hooks docker image in standalone mode and a script to accept ledgers every 5 seconds.