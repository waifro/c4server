SOURCE := source/main.c source/net_utils.c source/lobby.c source/client.c source/server.c
DEP_PP4M := source/pp4m/pp4m_io.c source/pp4m/pp4m_net.


OUTPUT := $(notdir $(CURDIR))

all :	$(SOURCE) $(DEP_PP4M)
	gcc $(SOURCE) $(DEP_PP4M) -Wall -Wextra -o $(OUTPUT)
