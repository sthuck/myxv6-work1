#include "types.h"
#include "user.h"

void add_path_helper(char* str) {
	if (add_path(str)==0)
		;
	else  {
		printf(2,"error in adding %s. History List probably full.\n",str);
		exit();
	}
}

int
main(int argc, char *argv[])
{

  if(argc < 1){
    printf(2, "usage: export path1:path2:....\n");
    exit();
  }
  char str[129];
  strcpy(str,argv[1]);
  if (str[0]==':') {
  	printf(2, "usage: export path1:path2:....\n");
    exit();
  }

  int i;
  int lastPathElementIndex=0;

  for (i=0;str[i];i++){
  	if (str[i]==':') {
  		str[i]=0;
  		add_path_helper(&str[lastPathElementIndex]);
  		lastPathElementIndex=i+1;
  	}
  }
  if (lastPathElementIndex<i) 
  	add_path_helper(&str[lastPathElementIndex]);

  exit();
}
