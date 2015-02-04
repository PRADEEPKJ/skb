#include <stdio.h>
#include <numa.h>
#include <stdlib.h>


void main (int argc, char *argv[]) {

  char *ptr;
  
  if(atoi(argv[1]) == 0 )
	 ptr = malloc (atoi(argv[2]));
  else
	ptr = numa_alloc_onnode(atoi(argv[2]), 0);
	
   if(ptr == NULL)
	printf("failed to allocate the memory\n");
   memset (ptr,0,atoi(argv[2]));

}

