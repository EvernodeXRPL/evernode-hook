#ifndef GOVERNOR_INCLUDED
#define GOVERNOR_INCLUDED 1

#include "../../headers/evernode.h"
#include "../../headers/statekeys.h"

#define FOREIGN_REF 0, 0, 0, 0

#define EQUAL_HOST_PROPOSE_CANDIDATE(buf, len)      \
    (sizeof(HOST_PROPOSE_CANDIDATE) == (len + 1) && \
     BUFFER_EQUAL_20(buf, HOST_PROPOSE_CANDIDATE))

#define EQUAL_HOST_VOTE_CANDIDATE(buf, len)              \
    (sizeof(HOST_VOTE_CANDIDATE) == (len + 1) &&         \
     BUFFER_EQUAL_8(buf, HOST_VOTE_CANDIDATE) &&         \
     BUFFER_EQUAL_8(buf + 8, HOST_VOTE_CANDIDATE + 8) && \
     BUFFER_EQUAL_1((buf + 16), (HOST_VOTE_CANDIDATE + 16)))
#endif
