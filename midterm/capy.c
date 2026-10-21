#include <stdio.h>

int main(int argc, char *argv[])
{
   char c;
   int op;
   long strtol( );
   op = (int) strtol(argv[1], (char **) NULL, 8);
   FILE *fp1, *fp2;
   if (argc !=4) {
      fprintf(stderr,  "How to use: %s num File1 File2\n", argv[0]);
      return 1;
   }
   fp1 = fopen(argv[2], "r");
   if (fp1 == NULL) {
      fprintf(stderr, "File %s Open Error\n", argv[1]);
      return 2;
   }

   fp2 = fopen(argv[3], "w");

	
   if(op == 0) {
      while ((c = fgetc(fp1)) != EOF){
      fputc(c, fp2);
      fputc(c, stdout);
}}
   else if(op == 1) {
      while ((c = fgetc(fp1)) != EOF){
      if(c >= 65 && c <= 95){
          c = c+32;	
          fputc(c, fp2);
          fputc(c, stdout);
}
      else{
	   fputc(c, fp2);
          fputc(c, stdout);
}
}}
   else if(op == 2) {
      while ((c = fgetc(fp1)) != EOF){
      if(c >= 97 && c <= 122){
      c = c - 32;
      fputc(c, fp2);
      fputc(c, stdout);
}
      else{
      fputc(c, fp2);
      fputc(c, stdout);
}
}}
   fclose(fp1);
   fclose(fp2);
   return 0;
}
