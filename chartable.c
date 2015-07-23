#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define UC unsigned char
int main(int argc, char* argv []){
UC code;
int ii,i;
printf("\n");
    if(argc > 1){
	if(isdigit(*argv[1])){
	    code=(UC)atoi(argv[1]);
	    printf("Code %3d is letter %c\n", code, code);
	    exit (0);}
	else{
	    code=(UC)*argv[1];
	    printf("Char %c is %3d\n", code, code);}}
    else{
	for(i=0; i<32; i++){
	    for(ii=1; ii<8; ii++){
		code=(UC)(i+ii*32);
		printf("%3d [0x%x] - %c\t", code, code, code);}
	    printf("\n");}}
return 0;
}
