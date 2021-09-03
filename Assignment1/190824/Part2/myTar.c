#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

int main(int argc,char **argv)
{
	char *str = (char*)malloc(1001*sizeof(char));
	int fd,fd_op,size,file_cnt=0,file_size;

	//create
	if(argv[1][1]=='c'){
		sprintf(str,"%s/%s",argv[2],argv[3]);
		fd = open(str,O_WRONLY | O_CREAT,0644);
		struct dirent *pDirent;
        DIR *pDir;

		//file count
		pDir = opendir (argv[2]);
		while ((pDirent = readdir(pDir)) != NULL) {
        	if(pDirent->d_name[0]!='.' && strcmp(pDirent->d_name,argv[3])!=0) file_cnt++;
		}
		closedir(pDir);
		sprintf(str,"<file_count>: %d\n",file_cnt);
		while(strlen(str)<25) strcat(str," ");
		write(fd,str,strlen(str));
		
		//file in tar
		pDir = opendir (argv[2]);
		while ((pDirent = readdir(pDir)) != NULL) {
        	if(pDirent->d_name[0]!='.' && strcmp(pDirent->d_name,argv[3])!=0){
            	file_cnt+=1;
				sprintf(str,"<ft>%s\n",pDirent->d_name);
				while(strlen(str)<25) strcat(str," ");
            	write(fd,str,strlen(str));
            	sprintf(str,"%s/%s",argv[2],pDirent->d_name);
            	fd_op = open(str,O_RDONLY);
				size=0;
            	while(read(fd_op,str,1000)==1000){
					write(fd,str,1000);
					size+=1000;
				}
				if(strlen(str)>0){
                	size+=strlen(str);
					while(strlen(str)<1000) strcat(str," ");
                	write(fd,str,strlen(str));
            	}
				sprintf(str,"<fs>%d\n",size);
				while(strlen(str)<1000) strcat(str," ");
				write(fd,str,strlen(str));
            	close(fd_op);
        	}
    	}
		closedir (pDir);
    	close(fd);
		return 0;
	}
	if(argv[1][1]=='d'){
		fd = open(argv[2],O_RDONLY);
		char file[256],name[300];
		strcpy(file,argv[2]);
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
			sprintf(name,"%s/%s",file,tok+4);
			fd_op = open(name,O_WRONLY | O_CREAT,0644);
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
	if(argv[1][1]=='e'){
		fd = open(argv[2],O_RDONLY);
		char file[256],name[300];
		strcpy(file,argv[2]);
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
		int cnt=0,mark=0;
		while(mark==0 && cnt<file_cnt){
			read(fd,str,25);
			tok = strtok(str,"\n");
			if(strcmp(tok+4,argv[3])==0){
				sprintf(name,"%s/%s",file,tok+4);
				fd_op = open(name,O_WRONLY | O_CREAT,0644);
				mark = 1;
			}
			while(read(fd,str,1000)){
				if(strncmp(str,"<fs>",4)==0) break;
				else if(mark==1) write(fd_op,str,1000);
			}
			cnt++;
			if(mark==1) close(fd_op);
		}
		close(fd);
		return 0;
	}

	//listing
	if(argv[1][1]=='l'){
		//get file size
		struct stat st;
		stat(argv[2], &st);
		file_size = st.st_size;
		
		fd = open(argv[2],O_RDONLY);
		
		//get path to file dir
		char s[] = "/";
		char path_dir[256];
		strcpy(path_dir,argv[2]);
		int j=strlen(path_dir)-1;
		while(path_dir[j]!='/') j--;
		strcpy(path_dir+j+1,"\0");
		//path to tarStructure
		strcat(path_dir,"tarStructure");
		fd_op = open(path_dir,O_WRONLY | O_CREAT,0644);
		sprintf(str,"%d\n",file_size);
		write(fd_op,str,strlen(str));

		read(fd,str,25);
		//num files in tar
		int i = 14;
		while(str[i]!='\n'){
			file_cnt = file_cnt*10+(str[i]-'0');
			write(fd_op,str+i,1);
			i++;
		}
		write(fd_op,"\n",strlen("\n"));
		char line[40],*tok;
		int cnt=0;
		while(cnt<file_cnt){
			read(fd,str,25);
			tok = strtok(str,"\n");
			sprintf(line,"%s ",tok+4);
			while(read(fd,str,1000)){
				if(strncmp(str,"<fs>",4)==0){
					tok = strtok(str,"\n");
					strcat(line,tok+4);
					strcat(line,"\n");
					write(fd_op,line,strlen(line));
					break;
				}
			}
			cnt++;
		}
		close(fd_op);
		close(fd);
		return 0;
	}
	return 0;
}
