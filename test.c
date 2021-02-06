#include <stdio.h>
#include "test.h"

#define LENGTH(a) (sizeof(a)/sizeof(a[0]))

int main() {
	for(int i = 0; i < LENGTH(text); i++){
		printf("%s", text[i]);
	}
	return 0;
}
