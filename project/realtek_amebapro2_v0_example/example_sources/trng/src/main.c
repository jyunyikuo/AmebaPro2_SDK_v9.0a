#include "device.h"
#include "trng_api.h"
#include "main.h"


void print_array(int arr[],  int n)
{
	int i;
	for (i = 0; i < n; i++) {
		printf("%08x \r\n", arr[i]);
	}
}

int main()
{
	volatile int lenght;
	lenght = 10;
	volatile int arr[10] = {0};

	trng_init_128k();
	trng_run_128k(lenght, &arr[0]);
	print_array(arr, lenght);
	trng_deinit();
	while (1) {;}
}





