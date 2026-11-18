#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


int main( ) 
{
   printf("부모 프로세스 시작\n");
   if (fork( ) == 0) {
      execl("/bin/echo", "echo", "hello", NULL);
      fprintf(stderr,"첫 번째 실패"); 
      exit(1);
   }
   printf("부모 프로세스 끝\n");
}

