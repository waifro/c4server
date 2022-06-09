SOURCE := source/main.c
DEP_PP4M := source/pp4m/pp4m_io.c source/pp4m/pp4m_net.c #source/pp4m/pp4m.c  source/pp4m/pp4m_draw.c source/pp4m/pp4m_image.c source/pp4m/pp4m_ttf.c source/pp4m/pp4m_net.c source/pp4m/pp4m_input.c


OUTPUT := $(notdir $(CURDIR))

all :	$(SOURCE) $(DEP_PP4M)
	gcc $(SOURCE) $(DEP_PP4M) -lws2_32 -Wall -Wextra -o $(OUTPUT)
