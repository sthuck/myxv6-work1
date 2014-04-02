#include "types.h"
#include "user.h"

int cid;

int findPidLocation(int pid, int* pids) {
  if (pid==-1) {
    printf(2,"error in fork\\wait\n");
    exit();
  }
  int i=0;
  for (i=0;i<20;i++) {
    if (pids[i]==pid)
      return i;
  }
  printf(2,"error in finding pid\n");
    exit();
  return -1;
}

int doFork() {
  int pid;
  pid=fork();
  if (pid==0) {
    if (cid%2==0) {
      int j;
      for (j=0;j<90000000;j++)  //time consuming, should take more than one cycle
        j++;

      int i;
      for (i=0;i<200;i++)
        printf(1,"cid: %d\n",cid);
    }
    else {
      int up = uptime();  //this should casue sleep and prio UP
      up=uptime();
      up=uptime();
      up++;
      int i;
      for (i=0;i<200;i++)
        printf(1,"cid: %d\n",cid);
    }
    exit();
  }
  else
    return pid;

}

int
main(int argc, char *argv[])
{
  int procStat[20][3];
  int pids[20];
  cid=-1;
  int i;
  for (i=0;i<20;i++) {
    cid++; 
    pids[i]=doFork();
  }
  for (i=0;i<20;i++) {
    int wTime=0,ioTime=0,rTime=0;
    int pid=wait2(&wTime,&rTime,&ioTime);
    int loc=findPidLocation(pid,pids);
    procStat[loc][0]=wTime;
    procStat[loc][1]=rTime;
    procStat[loc][2]=ioTime;
  }

  int wTime1=0,rTime1=0,ioTime1=0,turnTime1=0;
  int wTime2=0,rTime2=0,ioTime2=0,turnTime2=0;
  for (i=0;i<20;i=i+2){
    wTime1+=procStat[i][0];
    rTime1+=procStat[i][1];
    ioTime1+=procStat[i][2];
    turnTime1+=wTime1+rTime1+ioTime1;
  }
  for (i=1;i<20;i=i+2){
    wTime2+=procStat[i][0];
    rTime2+=procStat[i][1];
    ioTime2+=procStat[i][2];
    turnTime2+=wTime2+rTime2+ioTime2;
  }
  int wTimeAvg1=0,rTimeAvg1=0,ioTimeAvg1=0,turnTimeAvg1=0;
  int wTimeAvg2=0,rTimeAvg2=0,ioTimeAvg2=0,turnTimeAvg2=0;
  wTimeAvg1=wTime1/10;
  rTimeAvg1=rTime1/10;
  ioTimeAvg1=ioTime1/10;
  turnTimeAvg1=turnTime1/10;

  wTimeAvg2=wTime2/10;
  rTimeAvg2=rTime2/10;
  ioTimeAvg2=ioTime2/10;
  turnTimeAvg2=turnTime2/10;

  printf(1,"\n========================================\n\n");
  printf(1,"avarage stats for group 1 - low prio:\n");
  printf(1,"rtime:%d  wtime:%d  iotime:%d  Turnaround:%d\n\n",rTimeAvg1,wTimeAvg1,ioTimeAvg1,turnTimeAvg1);
  for (i=0;i<20;i=i+2)
    printf(1,"cid:%d  rtime:%d  wtime:%d  iotime:%d Turnaround:%d\n",i,procStat[i][1],procStat[i][0],procStat[i][2],procStat[i][0]+procStat[i][1]+procStat[i][2]);
  printf(1,"\n========================================\n\n");
  printf(1,"avarage stats for group 2 - high prio:\n");
  printf(1,"rtime:%d  wtime:%d  iotime:%d  Turnaround:%d\n\n",rTimeAvg2,wTimeAvg2,ioTimeAvg2,turnTimeAvg2);
  for (i=1;i<20;i=i+2)
    printf(1,"cid:%d  rtime:%d  wtime:%d  iotime:%d  Turnaround:%d\n",i,procStat[i][1],procStat[i][0],procStat[i][2],procStat[i][0]+procStat[i][1]+procStat[i][2]);
  
  exit();
}



