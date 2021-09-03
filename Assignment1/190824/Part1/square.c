#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
int main(int argc, char *argv[])
{
	int t = atoi(argv[argc-1]);
	t = t*t;
    char str[10];
	sprintf(str,"%d",t);
	strcpy(argv[argc-1],str);
	if(argc>2){
		char file[10];
		strcpy(file,"./");
		strcat(file,argv[1]);
		char *argv1[argc];
		for(int i=0;i<argc;i++) argv1[i]=argv[i+1];
		if(execv(file,argv1)<0){
			printf("ERROR");
		}
	}
	printf("%s",argv[argc-1]);
	return 0;
}
