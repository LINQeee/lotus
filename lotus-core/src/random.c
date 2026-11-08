#include <stdlib.h>
#include <time.h>

#include "functions.h"

int randomNum(int min, int max) {
    srand(time(NULL));
    return rand() % (max - min + 1) + min;
}
