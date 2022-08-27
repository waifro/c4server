SOURCE := source/main.c
DEP_PP4M := source/pp4m/pp4m.c source/pp4m/pp4m_io.c source/pp4m/pp4m_net.c
DEP_C4NETWORK := source/c4network/net.c source/c4network/net_utils.c source/c4network/send.c source/c4network/recv.c source/c4network/client.c source/c4network/server.c source/c4network/lobby.c

OUTPUT := $(notdir $(CURDIR))

all :	$(SOURCE) $(DEP_PP4M) $(DEP_C4NETWORK)
	gcc $(SOURCE) $(DEP_PP4M) $(DEP_C4NETWORK) -O0 -g -gdwarf-2 -Wall -Wextra -o $(OUTPUT)
