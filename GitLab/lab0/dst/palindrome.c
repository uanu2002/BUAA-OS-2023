#include <stdio.h>
int main() {
    int n;
    scanf("%d", &n);
    int s = n, y = 0;
    while(s)
    {
        y = y * 10 + s % 10;
        s /= 10;
    }
    if (y == n) {
        printf("Y\n");
    } else {
        printf("N\n");
    }
    return 0;
}
