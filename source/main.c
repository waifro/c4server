#include <stdio.h>

int main(int argc, char *argv[]) {

    printf("c4server started\n");

    while(1) {
        if (getchar()) break;
    }

    printf("c4server stop\n");

    return 0;
}
