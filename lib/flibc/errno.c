#include "errno.h"

int _errno_ = 0;
int* __errno(void) {
    return &_errno_;
}