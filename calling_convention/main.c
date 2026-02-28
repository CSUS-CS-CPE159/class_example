#include "stdio.h"

int add(int a, int b, int c){
    return a + b + c;
}

int main(){
    int a = 1;
    int b = 2;
    int c = 3;
    int sum = add(a, b, c);
    printf("sum is %d\n", sum);
    return 1;
}
