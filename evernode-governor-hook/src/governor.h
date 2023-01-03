#ifndef GOVERNOR_INCLUDED
#define GOVERNOR_INCLUDED 1

#include "../../lib/hookapi.h"
#include "../../headers/evernode.h"
#include "../../headers/statekeys.h"
#include "../../headers/transactions.h"

// Parameters
// Hook address which contains the states
const uint8_t PARAM_STATE_HOOK[32] = {'E', 'V', 'R', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};

#define FOREIGN_REF 0, 0, 0, 0

#endif
