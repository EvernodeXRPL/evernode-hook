#ifndef REPUTATION_INCLUDED
#define REPUTATION_INCLUDED 1

#include "../../lib/hookapi.h"
#include "../../headers/evernode.h"
#include "../../headers/statekeys.h"

#define FOREIGN_REF SBUF(NAMESPACE), state_hook_accid, ACCOUNT_ID_SIZE

// Minimum requirement of denominator to update score.
#define MIN_DENOM_REQUIREMENT(cluster_size) \
    cluster_size * 51 / 100

// Minimum requirement of cluster size to update score.
#define MIN_CLUSTER_REQUIREMENT(universe_size) \
    universe_size * 50 / 100

#endif
