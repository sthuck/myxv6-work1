#include "types.h"
#include "user.h"
#include "stat.h"
void pCP(void)
{
    int i;
    int pid;
    int Pchild[10][4];
    for (i = 0; i < 10; ++i) {
        if (fork() == 0) {
            int j;
            pid= getpid();
            for(j=0; j<1000;++j)
            	printf(1,"child %d prints for the %d time\n", pid,j);

            exit();
        }
    }
    // wait all child processes
    int rtime,wtime,iotime = 0;
    for (i = 0; i < 10; ++i)
        if((pid=wait2(&wtime,&rtime,&iotime))>0){
        	Pchild[i][0]=pid;
        	Pchild[i][1]=wtime;
        	Pchild[i][2]=rtime;
        	Pchild[i][3]=wtime+rtime+iotime;
        }
    for(i=0;i<10; ++i)
    	printf(1,"child %d : wait time %d run time %d trnaround time %d \n", i,Pchild[i][1],Pchild[i][2],Pchild[i][3]);
}

int main(void){
pCP();
exit();
} 
