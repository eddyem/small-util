#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

unsigned char c[3][128]={
"€‚ƒ„…†‡‰‹‘’“”•–—™› ΅Ά£¤¥¦§¨©ª«¬­®―°±²³΄µ¶·ΈΉΊ»Ό½ΎΏΐΑΒΓΔΕΖΗΘΙΚΛΜΝΞΟΠΡÒΣΤΥΦΧΨΩΪΫάέήίΰαβγδεζηθικλμνξοπρςστυφχψωϊϋόύώÿ",
"???????????????????????????????? ???????³Ώ????????????£???????αβχηδεφϊικλμνξοπςστυζθγώϋύÿωψόΰρΑΒΧΗΔΕΦΪΙΚΛΜΝΞΟΠÒΣΤΥΖΘΓήΫέίΩΨάΐΡ",
"αβχηδεφϊικλμνξοπςστυζθγώϋύÿωψόΰρΑΒΧΗΔΕΦΪΙΚΛΜΝΞΟΠ‘’‡²΄§¦µ΅¨®­¬ƒ„‰†€―°«¥»Έ± ΎΉΊ¶·ª©Ά¤½Ό…‚‹ÒΣΤΥΖΘΓήΫέίΩΨάΐΡ³£??????•–??” "};

char encode(const int ch, const int i){ //returns encoding letter
    char ret;
    if (ch == 13) ret = 10;
    else
	if (ch < 128) ret = ch;
    else
	ret = c[i][ch%256 - 128];
    return ret;
}

int recognize(FILE* f){ //asks to recognize encoding
    char* ans=(char*) malloc(128);
    int i,ch,ii;
    if (f==NULL){
	printf("Cannot open file\n");
	exit(4);
    }
    printf("\n");
    for (i=0; i<3; i++){
	ch=fgetc(f);
	ii=0;
	while(ch != EOF){
	    if (ch>127){
		++ii;
		if (ii<256) putchar(encode(ch,i));
		else
		    break;
	    }
	    ch=fgetc(f);
	}
	rewind(f);
	printf("\n\nIs it right?[n]\n");
	scanf("%s",ans);
	if(strcmp(ans,"y")==0 || strcmp(ans,"yes")==0) return i;
    }
    printf("\n\t\t\tUndefined codepage!!!\n\n");
    return 0;
}

int main(int argc, char** argv){
    int x,i,l;
    char name[128];
    FILE *f, *f1;
    if(argc<2){
	printf("\nError! Need at least 1 argument!!!\n");
	exit(1);
    }
    int global=0; //asks to recognize only once
    if(strcmp(argv[1],"cp1251")==0 || strcmp(argv[1],"CP1251")==0){
	--argc; ++argv;
	global=1;
	x=1;
    }
    if(strcmp(argv[1],"-r")==0 && global != 1){
	printf("Recognize first file\n");
	--argc;	++argv;			//analog of shell's "shift"
	global=1;
	f=fopen(argv[1],"r");
	x=recognize(f);
	fclose(f);
    }
    for (i=1; i<argc; i++){
	f=fopen(argv[i],"r");
	if (global==0) x=recognize(f);
	sprintf(name, "enc.%d", i);
	f1=fopen("tmp_encode","w");
	l=fgetc(f);
	while(l!=EOF){
	    fputc(encode(l,x),f1); //this is encoding itself
	    l=fgetc(f);
	}
	fclose(f1);
	fclose(f);
	unlink(argv[i]);//rename tmp->argv[i]
	if(link("tmp_encode",argv[i])<0)
	    printf("error moving file %s\n",argv[i]);
	unlink("tmp_encode");
	printf("File %s is done\n",argv[i]);
    }
    printf("\nAll files are processed!!!\n");
    return 0;
}
