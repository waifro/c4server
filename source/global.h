#ifndef GLOBAL_H
#define GLOBAL_H

#include "net_utils.h"

#define PORT_MAINNET    62443
#define PORT_TESTNET    61338

cli_t   glo_client_list[MAX_CLIENTS];
net_lobby   glo_lobby[MAX_LOBBY];

char *fen_standard = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 0";

#endif
