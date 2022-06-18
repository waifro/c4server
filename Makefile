SOURCE := source/main.c source/net_utils.c source/lobby.c source/client.c source/server.c
<<<<<<< HEAD
DEP_PP4M := source/pp4m/pp4m_io.c source/pp4m/pp4m_net.c
=======
DEP_PP4M := source/pp4m/pp4m_io.c source/pp4m/pp4m_net.

>>>>>>> 67100ae353fc62942cb556d5ee4e92cecdab56bf

OUTPUT := $(notdir $(CURDIR))

all :	$(SOURCE) $(DEP_PP4M)
	gcc $(SOURCE) $(DEP_PP4M) -Wall -Wextra -o $(OUTPUT)
