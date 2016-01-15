#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CFGEval.h"

double calculateCFGBranchDeviation(char*source1, char*source2){
  FILE *f1 =fopen(source1,"r");
  char *line_f1 = NULL;
  size_t len_f1 = 0;
  ssize_t read_f1;
  
  FILE *f2 = fopen(source2,"r");
  char *line_f2 = NULL;
  size_t len_f2 = 0;
  ssize_t read_f2;
  
  if (f1 == NULL){
    return -1;
  }
  if (f2==NULL){
    return -2;
  }
  
  int deviated_branches=0;
  int total_branches=0;
  
  while (((read_f1 = getline(&line_f1, &len_f1, f1)) != -1) && (read_f2 = getline(&line_f2, &len_f2, f2))!=-1) {
    
    int branch_num_f1=-1;
    int taken_count_f1=-1;
    int branch_num_f2=-1;
    int taken_count_f2=-1;
    
    int result_f1 = sscanf(line_f1, "branch  %d taken %d", &branch_num_f1,&taken_count_f1);
    int result_f2 = sscanf(line_f2, "branch  %d taken %d", &branch_num_f2,&taken_count_f2);
    
    if(result_f1==1){
      result_f1 = sscanf(line_f1, "branch  %d never executed", &branch_num_f1);
    }
    if(result_f2==1){
      result_f2 = sscanf(line_f2, "branch  %d never executed", &branch_num_f2);
    }
    
    if((branch_num_f1==branch_num_f2)){
      if(taken_count_f1!=taken_count_f2){
	deviated_branches++;
      }
      total_branches++;
    }
  }
  if (line_f1){
    free(line_f1);
  }
  if (line_f2){
    free(line_f2);
  }
  if(total_branches==0) return 0;
  return (double)deviated_branches/(double)total_branches;
}
