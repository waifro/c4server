SOURCE := source/main.c

OUTPUT := $(notdir $(CURDIR))

all :	$(SOURCE)
	gcc $(SOURCE) -Wall -Wextra -o $(OUTPUT)
