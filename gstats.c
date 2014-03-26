#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gstats.h"

FILE* gstats;

int clear_GStats(){
  if(remove(GSTATS_PATH)==0){
    return 0;
  }else{
    return -1;
  }
}

void open_GStats(){
  gstats = fopen(GSTATS_PATH, "ab+");
}

void close_GStats(){
  fclose(gstats);
}

int create_update_GStats(char*mut_code,char*key,char*value){
  return 0;
}



