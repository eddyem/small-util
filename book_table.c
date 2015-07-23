#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//use: tofile [-f] <total pages> [pages-in-pamphlet]
FILE* f;
int i,total,b,ii,ost,tp;
int tostd=1;//if prints to std -- adds comments to it

void first(int N, int i)
{
        int tp4 = (int)N/4;
	if (tostd)
	    printf("\tside # 1\n");
	fprintf(f,"\n");
	for (ii=1; ii<tp4; ii++){
	    fprintf(f, "%d,%d,", tp*i+N-2*ii+2, tp*i+2*ii-1);
	}
        fprintf(f, "%d,%d", tp*i+N-2*tp4+2, tp*i+2*tp4-1);	
	fprintf(f,"\n");
}

void second(int N, int i)
{
	int tp4 = (int)N/4;
	if (tostd)
	    printf("\tside # 2\n");
	for (ii=1; ii<tp4; ii++){
	    fprintf(f, "%d,%d,", tp*i+2*tp4-2*ii+2, tp*i+2*tp4-1+2*ii);
	}
        fprintf(f, "%d,%d", tp*i+2, tp*i+4*tp4-1);
	fprintf(f,"\n");
}

int main(int argc, char** argv)
{
//    printf("1:%s", argv[1]);
    if ( strcmp(argv[1], "-f") == 0 ){
        f = fopen("table", "w");
	--argc;
	++argv;
	tostd=0;
        }
    else f = stdout;	
//    printf("\n2:%s", argv[1]);
    total=atoi(argv[1]);
    tp=32;
    if ( argc > 2)
	tp=atoi(argv[2]);
    b = (int)total/tp;
    ost=total%tp;
    if (ost%4 > 0 || tp%4 >0){
		fprintf(stderr, "Число страниц не делится на 4");
		return(1);
	}
    for (i=0; i < b; i++){
	if (tostd)
	    printf("\nTetrad' # %d\n", i+1);
	first(tp,i);
	second(tp,i);
    }
    if ( ost>0 ){
        if (tostd)
	    printf("\nLast tetrad'\n");
	first(ost,b);
        second(ost,b);
    }
    fclose(f);
    return 0;
}
