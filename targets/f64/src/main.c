#include <stdint.h>

static volatile uint32_t dummy;

int main(void) {
    for(;;) {
        dummy++;
    }
}
