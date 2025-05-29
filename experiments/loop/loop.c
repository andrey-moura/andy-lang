#include <stdio.h>

int main() {
    int acc = 0;
    for (int i = 0; i <= 100; i++) {
        acc += i;
    }
    printf("%d\n", acc);
    return 0;
}