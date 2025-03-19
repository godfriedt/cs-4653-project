#include "password.h"

int authenticate(char password[]) {
    int len = strlen(password);
    int key = 0x10;
    key *= 0x10;
    key -= 0xe0;
    key += 0x5;
    key *=0x5;
    for (int i = 0; i < len; i++) {
        password[i] = password[i] ^ key;
    }
    return strncmp(password, GLOBAL_PASS, 16);
}