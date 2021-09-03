#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
int main(int argc,char **argv){
        int fd = open(argv[1],O_RDONLY);
        int file_cnt=0;
        char *str = (char*)malloc(1001*sizeof(char));
		char file[256],name[300];
		strcpy(file,argv[1]);
		char *tok;
		strcpy(file+strlen(file)-4,"\0");
		strcat(file,"Dump");
		mkdir(file,0777);
		read(fd,str,25);
		int i = 14;
		while(str[i]!='\n'){
			file_cnt = file_cnt*10+(str[i]-'0');
			i++;
		}
		int cnt=0;
		while(cnt<file_cnt){
			read(fd,str,25);
			tok = strtok(str,"\n");
			sprintf(name,"%s/",file);
            strcat(name,&tok[4]);
			int fd_op = open(name,O_WRONLY | O_CREAT,0644);
			while(read(fd,str,1000)){
				if(strncmp(str,"<fs>",4)==0) break;
				else write(fd_op,str,1000);
			}
			cnt++;
			close(fd_op);
		}
		close(fd);
		return 0;
}