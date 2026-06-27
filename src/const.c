#include "const.h"

const char CROSS_DOMAIN_XML[] =
    "<?xml version=\"1.0\"?>\n"
    "<cross-domain-policy>\n"
    "<allow-access-from domain=\"*\" to-ports=\"*\" />\n"
    "</cross-domain-policy>\n";

const size_t CROSS_DOMAIN_XML_LEN = sizeof(CROSS_DOMAIN_XML) - 1;
