#include "bitmap.h"

#define length 64

void setBit(int array[], int s){
	array[s/length] |= 1 << (s%length); 
}


void clearBit(int array[], int c){
	array[c/length] &= ~(1 << (c%length)); 
}

int testBit(int array[], int t){
	return ((array[t/length] & (1 << (t%length))) != 0); 
}
