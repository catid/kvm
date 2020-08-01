// Copyright 2020 Christopher A. Taylor

#include "sha1.h"

#include <iostream>
using namespace std;

#include <string.h>

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    cout << "sha1_test" << endl;

    sha1_state state{};

    sha1_neon_init(&state);

    uint8_t data[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    sha1_neon_update(&state, data, (unsigned)sizeof(data));

    uint8_t hash[SHA1_DIGEST_SIZE];

    sha1_neon_final(&state, hash);

    return 0;
}
