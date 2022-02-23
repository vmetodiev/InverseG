#include <stdio.h>

void print_ptr_int_value_short(void *p)
{
    printf("Short cast, value = %d\r\n", (int)p);
    printf("Short cast, address = %p\r\n", (int)&p);
    return;
}

void print_ptr_int_value_explicit(void *p)
{
    printf("Explicit cast, value = %d\r\n", *((int *)p));
    printf("Explicit cast, address = %p\r\n", (int)&p);
    return;
}


int main()
{
    int a = 15;
   
    print_ptr_int_value_short( (void *)a );
    print_ptr_int_value_explicit( (void *)&a );
   
    return 0;
}