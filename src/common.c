#include "common.h"
#include <string.h>

const char *common_get_prefix(const char *room) {
    if (!room) return "o";
    if (strncmp(room, "house_", 6) == 0) return "h";
    if (strncmp(room, "work_", 5) == 0) return "w";
    if (strncmp(room, "clan_", 5) == 0) return "c";
    return "o";
}
