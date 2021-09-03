#include "wc.h"


extern struct team teams[NUM_TEAMS];
extern int test;
extern int finalTeam1;
extern int finalTeam2;

int processType = HOST;
const char *team_names[] = {
  "India", "Australia", "New Zealand", "Sri Lanka",   // Group A
  "Pakistan", "South Africa", "England", "Bangladesh" // Group B
};


void teamPlay(void)
{
  char name[30];
  sprintf(name,"./test/%d/inp/%s",test,teams[processType].name);
  //printf("teamplay:%s\n",name);
  int fd = open(name,O_RDONLY);
  char *curr = (char *)calloc(1,sizeof(char));
  while(read(fd,curr,1)==1){
    write(teams[processType].matchpipe[WR],curr,1);
  }
  if(read(teams[processType].commpipe[RD],name,10)==10){
      free(curr);
      close(fd);
      exit(0);
  }
  return;
}

void endTeam(int teamID)
{
  write(teams[teamID].commpipe[WR],"terminate",10);
  return;
}

int match(int team1, int team2)
{
  int t1,t2,bat,ball,n_ball,n_wic,run,last,tot[2]={0,0},win_Y = 0;
  char *n1 = (char*) calloc(1,sizeof(char));
  char *n2 = (char*) calloc(1,sizeof(char));
  char str[100];
  sprintf(str,"./test/%d/out",test);
  int t = mkdir(str,0777);
  
  //TOSS
  read(teams[team1].matchpipe[RD],n1,1);
  read(teams[team2].matchpipe[RD],n2,1);
  t1 = atoi(n1);
  t2 = atoi(n2);
  bat = ((t1+t2)%2)?team1:team2;
  ball = ((t1+t2)%2)?team2:team1;

  sprintf(str,"./test/%d/out/%sv%s",test,teams[bat].name,teams[ball].name);
  if(team1/4!=team2/4) strcat(str,"-Final");
  int fd = open(str,O_RDWR | O_CREAT, 0644);
  
  //INN1
  sprintf(str,"Innings1: %s bats\n",teams[bat].name);
  write(fd,str,strlen(str));
  n_ball = 1, n_wic = 1,run = 0, last=0;
  while(n_ball<=120 && n_wic<=10){
    read(teams[bat].matchpipe[RD],n1,1);
    read(teams[ball].matchpipe[RD],n2,1);
    t1 = atoi(n1);
    t2 = atoi(n2);
    if(t1!=t2) run+=t1;
    else{
      sprintf(str,"%d:%d\n",n_wic,run);
      tot[0]+=run;
      n_wic+=1;
      run=0;
      if(n_ball==120) last=1;
      write(fd,str,strlen(str));
    }
    n_ball++;
  }
  if(!last && n_wic<=10){
    tot[0]+=run;
    sprintf(str,"%d:%d*\n",n_wic,run);
    write(fd,str,strlen(str));
  }
  sprintf(str,"%s TOTAL: %d\n",teams[bat].name,tot[0]);
  write(fd,str,strlen(str));

  //INN2
  int temp = bat;
  bat = ball;
  ball = temp;
  sprintf(str,"Innings2: %s bats\n",teams[bat].name);
  write(fd,str,strlen(str));
  n_ball = 1, n_wic = 1,run = 0, last=0;
  while(n_ball<=120 && n_wic<=10 && !win_Y){
    read(teams[bat].matchpipe[RD],n1,1);
    read(teams[ball].matchpipe[RD],n2,1);
    t1 = atoi(n1);
    t2 = atoi(n2);
    if(t1!=t2) run+=t1;
    else{
      sprintf(str,"%d:%d\n",n_wic,run);
      tot[1]+=run;
      n_wic+=1;
      run=0;
      if(n_ball==120) last=1;
      write(fd,str,strlen(str));
    }
    n_ball++;
    if(tot[1]+run>=tot[0]) win_Y = 1;
  }
  if(!last && n_wic<=10){
    tot[1]+=run;
    sprintf(str,"%d:%d*\n",n_wic,run);
    write(fd,str,strlen(str));
  }
  sprintf(str,"%s TOTAL: %d\n",teams[bat].name,tot[1]);
  write(fd,str,strlen(str));
  free(n1);
  free(n2);
  //WIN
  if(win_Y){
    sprintf(str,"%s beats %s by %d wickets",teams[bat].name,teams[ball].name,11-n_wic);
    write(fd,str,strlen(str));
    close(fd);
    return bat;
  }
  if(tot[0]==tot[1]){
    sprintf(str,"TIE: %s beats %s",teams[team1].name,teams[team2].name);
    write(fd,str,strlen(str));
    close(fd);
    return team1;
  }
  sprintf(str,"%s beats %s by %d runs\n",teams[ball].name,teams[bat].name,tot[0]-tot[1]);
  write(fd,str,strlen(str));
  close(fd);
  return ball;
}

void spawnTeams(void)
{
  for(int i=0;i<NUM_TEAMS;i++){
    strcpy(teams[i].name, team_names[i]);
    pipe(teams[i].matchpipe);
    pipe(teams[i].commpipe);
    int pid = fork();
    if(pid==0){
      //printf("spawnteams: %d\n",i);
      close(teams[i].commpipe[WR]);
      close(teams[i].matchpipe[RD]);
      close(RD);
      dup(teams[i].commpipe[RD]);
      close(WR);
      dup(teams[i].matchpipe[WR]);
      processType=i;
      teamPlay();
      return;
    }
  }
  return;
}

void conductGroupMatches(void)
{
  int groupPipe[2][2];
  char *str = (char*)calloc(1,sizeof(char));
  for(int grp=0;grp<2;grp++){
    pipe(groupPipe[grp]);
    int pid=fork();
    if(pid==0){
      int win_log[4]={0,0,0,0};
      for(int i=0;i<4;i++){
        for(int j=i+1;j<4;j++) win_log[match(grp*4+i,grp*4+j)-(grp*4)]++;
      }
      int max=win_log[0],max_idx=0;
      for(int i=1;i<4;i++){
        if(win_log[i]>max){
          max=win_log[i];
          max_idx=i;
        }
      }
      max_idx+=grp*4;
      for(int i=grp*4;i<grp*4+4;i++){
        if(i!=max_idx) endTeam(i);
      }
      sprintf(str,"%d",max_idx);
      write(groupPipe[grp][WR],str,1);
      exit(0);
    }
  }
  wait(NULL);
  read(groupPipe[0][RD],str,1);
  finalTeam1 = atoi(str);
  read(groupPipe[1][RD],str,1);
  finalTeam2 = atoi(str);
  free(str);
  return;
}
