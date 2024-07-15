#ifndef REPUTATION_INCLUDED
#define REPUTATION_INCLUDED 1

#include "../../lib/hookapi.h"
#include "../../headers/evernode.h"
#include "../../headers/statekeys.h"

#define FOREIGN_REF SBUF(NAMESPACE), state_hook_accid, ACCOUNT_ID_SIZE

// Minimum requirement of denominator to update score.
#define MIN_DENOM_REQUIREMENT(universe_size) \
    universe_size * 0.8

#endif
