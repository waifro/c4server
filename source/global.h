#ifndef GLOBAL_H
#define GLOBAL_H

#include "c4network/net_utils.h"

#define PORT_MAINNET 62443
#define PORT_TESTNET 61338

extern cli_t glo_client_list[MAX_CLIENTS];
extern net_lobby glo_lobby[MAX_LOBBY];

#endif