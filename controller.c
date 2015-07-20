#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <inttypes.h>
#include <time.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <libconfig.h>
#include <hashset.h>

#include "CFGEval.h"
#include "valgrindEval.h"
#include "gstats.h"

typedef struct {
  char *sourceRootDir;
  char **source;
  int  testingFramework;
  char *CuTestLibSource;
  char *CheckTestLibSource;
  char *makeTestTarget;
  char *executablePath;
  
  int numberOfSource;
  
} Config;


typedef struct{
  char *mutation_code;
  char *mutant_source_file;
  //fragility is between 0->1
  double fragility;
  //set of tests which killed this mutant
  int *killed_by_tests;
  int killed_by_tests_count;
  ValgrindResult *valgrindResult;
  //CFGDeviation
  double cfgDeviation;
  double damage;
} Mutant;

typedef struct{
  char *HOMutant_source_file;
  Mutant *FOMutants;
  //fragility is between 0->1
  double fragility;
  //Discard HOM when fitness =0 or 1, fitness is between 0->1
  double fitness;
  int FOMutants_count;
  //CFGDeviation
  double cfgDeviation;
  double damage;
} HOMutant;


typedef struct{
  int total_mutants;
  int mutant_kill_count;
} HOMResult;

typedef struct{
  char *original_source_file;
  int non_trivial_FOM_count;
  Mutant *non_trivial_FOMS;
  Mutant *survived;
  int total_mutants;
  int total_tests;
  int killed_by_valgrind;
  int caused_CFG_deviation;
  int total_valgrind_errors;
  int survived_caused_CFG_deviation;
  int survived_killed_by_valgrind;
  
  int equivalent_mutants;
  //Survived
  int survived_count;
  int survived_SD;
  int survived_WD;
  int survived_WD_causedValgrindErrors;
  int survived_WD_caused_CFG_deviation;
  //Killed
  int mutant_kill_count;
  int killed_SD;
  int killed_WD;
  int killed_WD_causedValgrindErrors;
  int killed_WD_caused_CFG_deviation;
  int failed_injection;
  int dumb_mutants;
  
} FOMResult;


typedef struct{
  FOMResult *fomResult;
  HOMResult *homResult;
} MResult;

int PUTValgrindErrors=0;
int make_logs;
int verbose_mode=0;
int preset_project=0;
FILE* mutation_results=NULL;
FILE* temp_results=NULL;
char* mutation_results_path=NULL;
char* make_logs_dir=NULL;
char* temp_results_path=NULL;
char* GSTATS_PATH=NULL;

int startprogram(char*programparams[],char* stdoutfd,int redirect){
  int status = 0;
  int id = fork();
  
  if(id==0){
    if(redirect==1){
      FILE* file;
      if(stdoutfd!=NULL){
	file = fopen(stdoutfd, "a+");
      }else if(stdoutfd==NULL){
	file = fopen("/dev/null", "r");
      }
      dup2(fileno(file), fileno(stdout));
      dup2(fileno(file), fileno(stderr));
      fclose(file);
    }
    execvp (programparams[0], programparams);
  }else{
    waitpid(id,&status,0);
    if(WIFEXITED(status)) {
      return WEXITSTATUS(status);
    }
  }
  return -1;
}

void cleanUp(char*copyPUTDir){
  char**args_del = calloc(sizeof(char*),4);
  args_del[0]="rm";
  args_del[1]="-rf";
  args_del[2]=malloc(sizeof(char)*strlen(copyPUTDir)+1);
  strcpy(args_del[2],copyPUTDir);
  args_del[3]=NULL;
  startprogram(args_del,NULL,0);
}

Config* processConfigFile(char* filePath){
  config_t cfg;
  config_setting_t *setting;
  const char *str;
  
  config_init(&cfg);
  
  Config *user_config = malloc(sizeof(Config));
  
  /* Read the file. If there is an error, report it and exit. */
  if(! config_read_file(&cfg, filePath))
  {
    fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg),
	    config_error_line(&cfg), config_error_text(&cfg));
    config_destroy(&cfg);
    return(NULL);
  }
  
  /* Get the source root dir. */
  if(config_lookup_string(&cfg, "sourceRootDir", &str)){
    user_config->sourceRootDir=malloc(strlen(str)+1);
    user_config->sourceRootDir=strcpy(user_config->sourceRootDir,str);
  }else{
    fprintf(stderr, "No 'sourceRootDir' setting in configuration file.\n");
    return(NULL);
  }
  
  /* Get executable_path. */
  if(config_lookup_string(&cfg, "executablePath", &str)){
    user_config->executablePath=malloc(strlen(str)+1);
    user_config->executablePath=strcpy(user_config->executablePath,str);
  }else{
    fprintf(stderr, "No 'executablePath' setting in configuration file.\n");
    return(NULL);
  }
  
  /* Get source locations*/
  setting = config_lookup(&cfg, "source");
  
  if(setting != NULL)
  {
    int count = config_setting_length(setting);
    int i;
    
    user_config->source = malloc((count+1)*sizeof(char*));
    user_config->numberOfSource = count;
    for(i = 0; i < count; ++i)
    {
      const char *filename= config_setting_get_string_elem(setting, i);
      user_config->source[i] = malloc(sizeof(char)*(strlen(filename)+1));
      strcpy(user_config->source[i],filename);
    }
  }else{
    fprintf(stderr, "No 'source' setting in configuration file.\n");
    return(NULL);
  }
  
  /* Get testing framework */
  if(config_lookup_string(&cfg, "testingFramework", &str)){
    
    if(strcmp(str,"cutest")==0){
      user_config->testingFramework=1;
      
      printf("\n-->Assuming CuTest runner being run from make file<--\n");
      
      if(config_lookup_string(&cfg, "CuTestLibSource", &str)){
	user_config->CuTestLibSource=malloc(strlen(str)+1);
	user_config->CuTestLibSource=strcpy(user_config->CuTestLibSource,str);
      }else{
	fprintf(stderr, "No CuTest lib source found\n");
	return(NULL);
      }
      
    }else if(strcmp(str,"libcheck")==0){
      user_config->testingFramework=2;
      if(config_lookup_string(&cfg, "CheckTestLibSource", &str)){
	user_config->CheckTestLibSource=malloc(strlen(str)+1);
	user_config->CheckTestLibSource=strcpy(user_config->CheckTestLibSource,str);
      }else{
	printf("\n-->Assuming the project is using the modified libcheck package provided in Mutate/libcheck<--\n");
      }
    }else{
      fprintf(stderr, "testing framework provided in the configuration file is not supported\n");
      return(NULL);
    }
    /* get make test target */
    if(config_lookup_string(&cfg, "makeTestTarget", &str)){
      user_config->makeTestTarget=malloc(strlen(str)+1);
      user_config->makeTestTarget=strcpy(user_config->makeTestTarget,str);
    }
  }else{
    fprintf(stderr, "No 'testingFramework' setting in configuration file.\n");
    return(NULL);
  }
  config_destroy(&cfg);
  return user_config;
}

void copy_file(char*src,char*target){
  char**args_cp_file = calloc(sizeof(char*),4);
  args_cp_file[0]="cp";
  args_cp_file[1]=malloc(sizeof(char)*strlen(src)+1);
  strcpy(args_cp_file[1],src);
  args_cp_file[2]=malloc(sizeof(char)*strlen(target)+1);
  strcpy(args_cp_file[2],target);
  args_cp_file[3]=NULL;
  startprogram(args_cp_file,NULL,0);
}

int startMake(char**args,char*currentMutation){
  int pid = fork();
  
  if(pid==0){
    //Disable runtime protection provided by glibc in order to ensure mutants are injected and run 
    putenv("MALLOC_CHECK_=0");
    make_logs=open(make_logs_dir, O_RDWR | O_APPEND);
    if(currentMutation!=NULL){
      char *title="--> MUTATION ::";
      char *mut_txt = malloc(snprintf(NULL, 0, "\n%s %s\n\n", title,currentMutation ) + 1);
      sprintf(mut_txt, "\n%s %s\n\n",title , currentMutation);
      write(make_logs,mut_txt,strlen(mut_txt));
    }
    dup2(make_logs, 1);
    dup2(make_logs, 2);
    close(make_logs);
    execvp (args[0],args);
  }else{
    int status;
    struct timespec sleepTime;
    struct timespec remainingTime;
    const int SLEEP_UNIT_IN_MS=100;
    const int WAIT_MAX_IN_MS=60*1000;
    sleepTime.tv_sec=0;
    sleepTime.tv_nsec=SLEEP_UNIT_IN_MS*1000*1000;
    int waitStatus, i;
    for(i=0; i<WAIT_MAX_IN_MS/SLEEP_UNIT_IN_MS; i++){
    	waitStatus=waitpid(pid, &status, WNOHANG);
    	if( waitStatus!=0 ) {
    		break;
    	}
    	nanosleep(&sleepTime, &remainingTime);
    }
    if( waitStatus < 0 ) {
      perror("wait");
    }
    else if(waitStatus==0){
    	// timeout, kill the process
    	kill(pid, SIGKILL);
    	printf("Timeout, killed child process.\n");
    	waitpid(pid, &status, 0);
    	return 2;
    }
    if(WIFEXITED(status)) {
      printf("make returns %d.\n", WEXITSTATUS(status));
      return WEXITSTATUS(status);
    }
  }
  return -1;
}

int runMake(char*projectDir,char*currentMutation,char*makeTestTarget){
  char**args_make = calloc(sizeof(char*),5);
  args_make[0]="make";
  args_make[1]="-C";
  args_make[2]=calloc(sizeof(char),strlen(projectDir)+1);
  strcpy(args_make[2],projectDir);
  //now run
  args_make[3]=makeTestTarget;
  args_make[4]=NULL;
  //Actually make now but use popen to access output of the child process
  if(startMake(args_make,currentMutation)==2){
    mutation_results = fopen(mutation_results_path,"a+");
    char *status = "status: (killed)\n";
    char *desc = "description: mutation caused a runtime error\n";
    fwrite(status,1,strlen(status),mutation_results);
    fwrite(desc,1,strlen(desc),mutation_results);
    fclose(mutation_results);
    return 2;
  }
  free(args_make[2]);
  free(args_make);
  
  return 0;
}

void extractCFGBranches(char*srcDir,char*outputDir,char*filename){
  char **args_genCFGBranches = malloc(sizeof(char*)*6);
  args_genCFGBranches[0]="bash";
  args_genCFGBranches[1]="GenCFGBranches.sh";
  args_genCFGBranches[2]=srcDir;
  args_genCFGBranches[3]=outputDir;
  if(filename==NULL){
    args_genCFGBranches[4]=NULL;
    args_genCFGBranches[5]=NULL;
  }else{
    args_genCFGBranches[4]=filename;
    args_genCFGBranches[5]=NULL;
  }
  startprogram(args_genCFGBranches,NULL,0);
  free(args_genCFGBranches);
}

//stats[0] = current mutant kill count
//stats[1] = number of tests that the  mutant failed
//stats[2] = total number of tests in the test suite
int* get_non_trivial_FOM_stats(){
  int *stats = calloc(sizeof(int),20);
  char killed_test_id[128];
  temp_results=fopen(temp_results_path,"r");
  fscanf (temp_results, "%d %d %d %s", stats, stats+1, stats+2,killed_test_id);
  fclose(temp_results);
  
  //Parse comma separated string
  int counter=3; 
  char *pt=NULL;
  pt = strtok (killed_test_id,",");
  while (pt != NULL) {
    if(counter>20){
      stats = realloc(stats,(counter*=2)*sizeof(int*));
    }
    stats[counter++]= atoi(pt);
    pt = strtok (NULL, ",");
  }
  return stats;	    
}

void printValgrindError(ValgrindError *valgrindError,int i){
  if(valgrindError==NULL){
    return;
  }
  if(valgrindError->location!=NULL){
    fprintf(mutation_results, "\t*** Error(%d) ***\n",i+1);
    fprintf(mutation_results, "\t> location: %s\n",valgrindError->location);
  }
  if(valgrindError->kind!=NULL){
    fprintf(mutation_results, "\t> kind: %s\n",valgrindError->kind);
  }
  if(valgrindError->what!=NULL){
    fprintf(mutation_results, "\t> what: %s\n",valgrindError->what);
  }
  if(valgrindError->auxwhat!=NULL){
    fprintf(mutation_results, "\t> auxwhat: %s\n\n",valgrindError->auxwhat);
  } 
}

void printValgrindResult(char*mutation_code,ValgrindResult*valgrindResult){
  
  int i;
  mutation_results = fopen(mutation_results_path,"a+");
  
  //Update gstats to record the number of valgrind errors, the mutant caused
  create_update_gstat_mutation(mutation_code,"valgrind_errors_count",get_gstat_value_mutation(mutation_code,"valgrind_errors_count")+valgrindResult->valgrind_error_count);
  fprintf(mutation_results, "New Valgrind errors: %d\n",valgrindResult->valgrind_error_count);
  
  if(verbose_mode==1){
    for(i=0;i<valgrindResult->unique_valgrind_error_count;i++){
      printValgrindError(&valgrindResult->valgrindErrors[i],i);
    }
  }
  
  fflush(mutation_results);
  fclose(mutation_results);
}

ValgrindResult * genValgrindResult(char *cwd, char*original_file_Name_dir,char*str, char*makeDir,Config*user_config){
   char **args_valgrind = calloc(sizeof(char*),8);
    char* xml_file_location = malloc(snprintf(NULL, 0, "%s/mutation_out/%s/valgrind_eval_%s.xml",cwd,original_file_Name_dir, str) + 1);
    sprintf(xml_file_location, "%s/mutation_out/%s/valgrind_eval_%s.xml",cwd,original_file_Name_dir, str);
    
    args_valgrind[0]="valgrind";
    args_valgrind[1]="-q";
    args_valgrind[2]="--trace-children=yes";
    args_valgrind[3]="--child-silent-after-fork=yes";
    args_valgrind[4]="--xml=yes";
    args_valgrind[5]= malloc(snprintf(NULL, 0, "--xml-file=%s",xml_file_location) + 1);
    sprintf(args_valgrind[5], "--xml-file=%s",xml_file_location);
    args_valgrind[6]=malloc(snprintf(NULL, 0, "%s", user_config->executablePath) + 1);
    sprintf(args_valgrind[6], "%s", user_config->executablePath);
    args_valgrind[7]=NULL;
    
    //Make sure we do not get duplicate output when lanching valgrind
    setenv("IS_MUTATE","false",1);
    
    startprogram(args_valgrind,NULL,1);
    
    //set it back so this process gets the right output
    setenv("IS_MUTATE","true",1);
    
    //evaluate the .xml output generated by Valgrind in order to generate statistics
    ValgrindResult* valgrindResult =valgrindXML_evaluate(xml_file_location,user_config->testingFramework);
    free(args_valgrind);
    return valgrindResult;
}

void genResultsFOM(char *str,char* makeDir,char* filename_qfd,char*mv_dir,Config *user_config,MResult*mResult,int non_trivial_FOM_buffer,char*txl_original,char *original_file_Name_dir,char*cwd){
  
  char**args_diff;
  Mutant *non_trivial_FOMS_ptr = mResult->fomResult->non_trivial_FOMS;
  Mutant *survived_ptr = mResult->fomResult->survived;
  //Run make on the mutated project in order to build it
  printf("--> Evaluating FOM: %s\n",str);
  
  //Extract the mutation code
  char* mutation_code=NULL;
  char *chptr = strrchr(str, '_');
  
  if(chptr==NULL){
    mutation_code=strndup(str,strlen(str)-2);
  }else{
    long dif = chptr - str;
    mutation_code=strndup(str,dif);
  }
  
  //Start Updating gstats
  open_GStats(GSTATS_PATH);
  
  //Open log file for recording mutation results
  mutation_results = fopen(mutation_results_path,"a+");
  fprintf(mutation_results, "\n**** Mutant: %s ****\n",str);
  fflush(mutation_results);
  fclose(mutation_results);
  mResult->fomResult->total_mutants++;
  
  
  //Get mutants killed by tests before new evaluation
  int prev_killed_by_tests=get_non_trivial_FOM_stats()[0];
  
  //Evaluate mutant
  int make_result = runMake(makeDir,str,user_config->makeTestTarget);
  double cfgDeviation =0.0;
  
  create_update_gstat_mutation(mutation_code,"generated_mutants",get_gstat_value_mutation(mutation_code,"generated_mutants")+1);
  
  if(make_result==2){
    mResult->fomResult->mutant_kill_count++;
    mResult->fomResult->failed_injection++;
    //Update gstats to record mutant did not execute
    create_update_gstat_mutation(mutation_code,"failed_injection",get_gstat_value_mutation(mutation_code,"failed_injection")+1);
  }else{

    char* cfg_out = malloc(snprintf(NULL, 0, "%s/mutation_out/%s",cwd,original_file_Name_dir) + 1);
    sprintf(cfg_out, "%s/mutation_out/%s",cwd,original_file_Name_dir);
    extractCFGBranches(makeDir,cfg_out,original_file_Name_dir);
    free(cfg_out);
    //compare CFG branches with PUT
    char* cfg_out_file = malloc(snprintf(NULL, 0, "%s/mutation_out/%s/%s.c.gcov.branches",cwd,original_file_Name_dir,original_file_Name_dir) + 1);
    sprintf(cfg_out_file, "%s/mutation_out/%s/%s.c.gcov.branches",cwd,original_file_Name_dir,original_file_Name_dir);
    //original path
    char* cfg_original_file = malloc(snprintf(NULL, 0, "%s/mutation_out/PUT/%s.c.gcov.branches",cwd,original_file_Name_dir) + 1);
    sprintf(cfg_original_file, "%s/mutation_out/PUT/%s.c.gcov.branches",cwd,original_file_Name_dir);
    
    //calculate the deviation
    cfgDeviation =calculateCFGBranchDeviation(cfg_original_file,cfg_out_file);
    if(cfgDeviation>0){
      create_update_gstat_mutation(mutation_code,"cfg_deviation_raw",get_gstat_value_mutation(mutation_code,"cfg_deviation_raw")+(cfgDeviation*100));
      mResult->fomResult->caused_CFG_deviation++;
    }
     
    mutation_results = fopen(mutation_results_path,"a+");
    fprintf(mutation_results, "CFG Deviation : %.2f%%\n",cfgDeviation*100.00);
    fflush(mutation_results);
    fclose(mutation_results);
    free(cfg_original_file);
    free(cfg_out_file);
  }
  
  //Get mutants killed by tests after evaluation
  int *stats = get_non_trivial_FOM_stats();
  
  if(stats[0]-prev_killed_by_tests==1){
    //Update gstats to record mutant was killed
    create_update_gstat_mutation(mutation_code,"killed_by_tests",get_gstat_value_mutation(mutation_code,"killed_by_tests")+1);
    
    
    mResult->fomResult->mutant_kill_count++;
    
    if(stats[0] >non_trivial_FOM_buffer){
      mResult->fomResult->non_trivial_FOMS = realloc(mResult->fomResult->non_trivial_FOMS,(non_trivial_FOM_buffer*=2)*sizeof(Mutant));
    }
    
    int NTFC = mResult->fomResult->non_trivial_FOM_count;
    
    ValgrindResult* valgrindResult = genValgrindResult(cwd, original_file_Name_dir,str, makeDir,user_config);
    if(valgrindResult!=NULL){
      valgrindResult->valgrind_error_count=valgrindResult->valgrind_error_count>PUTValgrindErrors?valgrindResult->valgrind_error_count-PUTValgrindErrors:0;
      mResult->fomResult->total_valgrind_errors+=valgrindResult->valgrind_error_count;
      printValgrindResult(mutation_code,valgrindResult);
    }
    
    if(stats[1]>0 && stats[1]<stats[2]){
      //Was a non_trivial_mutant ie: only a subset of the tests killed it
      
      //Update gstats to record mutant was a  non_trivial_mutant
      
      non_trivial_FOMS_ptr[NTFC].mutant_source_file = calloc(sizeof(char),strlen(filename_qfd)+1);
      non_trivial_FOMS_ptr[NTFC].mutation_code = calloc(sizeof(char),strlen(mutation_code)+1);
      strncpy(non_trivial_FOMS_ptr[NTFC].mutant_source_file,filename_qfd,strlen(filename_qfd));
      strncpy(non_trivial_FOMS_ptr[NTFC].mutation_code,mutation_code,strlen(mutation_code));
      
      non_trivial_FOMS_ptr[NTFC].fragility=((double)stats[1]/(double)stats[2]);
      non_trivial_FOMS_ptr[NTFC].cfgDeviation=cfgDeviation;
      non_trivial_FOMS_ptr[NTFC].damage=(((1-non_trivial_FOMS_ptr[NTFC].fragility)/3.0)+(non_trivial_FOMS_ptr[NTFC].cfgDeviation/3.0));
      
      non_trivial_FOMS_ptr[NTFC].killed_by_tests = calloc(sizeof(int),stats[1]);
      non_trivial_FOMS_ptr[NTFC].killed_by_tests_count=stats[1];
      memcpy(non_trivial_FOMS_ptr[NTFC].killed_by_tests,stats+3,stats[1]*sizeof(int));
     
      non_trivial_FOMS_ptr[NTFC].valgrindResult = valgrindResult;
      mResult->fomResult->non_trivial_FOM_count++;
    }else if(stats[1]>0 && stats[1]==stats[2]){
      mResult->fomResult->dumb_mutants++;
    }
    mResult->fomResult->total_tests=stats[2];
    
    if((valgrindResult!=NULL && valgrindResult->valgrind_error_count!=PUTValgrindErrors)&& cfgDeviation>0){
       mResult->fomResult->killed_SD++;
       create_update_gstat_mutation(mutation_code,"killed_SD",get_gstat_value_mutation(mutation_code,"killed_SD")+1);
       
    }else if((valgrindResult!=NULL && valgrindResult->valgrind_error_count!=PUTValgrindErrors)){
      mResult->fomResult->killed_WD++;
      mResult->fomResult->killed_WD_causedValgrindErrors++;
       create_update_gstat_mutation(mutation_code,"killed_WD",get_gstat_value_mutation(mutation_code,"killed_WD")+1);
      create_update_gstat_mutation(mutation_code,"killed_WD_VE",get_gstat_value_mutation(mutation_code,"killed_WD_VE")+1);
   
    }else if(cfgDeviation>0){
      mResult->fomResult->killed_WD++;
      mResult->fomResult->killed_WD_caused_CFG_deviation++;
       create_update_gstat_mutation(mutation_code,"killed_WD",get_gstat_value_mutation(mutation_code,"killed_WD")+1);
      create_update_gstat_mutation(mutation_code,"killed_WD_CFD",get_gstat_value_mutation(mutation_code,"killed_WD_CFD")+1);
   
    }
    
  }else if(make_result!=2){
    //Update gstats to record the mutant survived
    //create_update_gstat_mutation(mutation_code,"survived_tests_count",get_gstat_value_mutation(mutation_code,"survived_tests_count")+1);
    
    if(cfgDeviation!=0){
	mResult->fomResult->survived_caused_CFG_deviation++;
	//create_update_gstat_mutation(mutation_code,"survived_CFG_deviation",get_gstat_value_mutation(mutation_code,"survived_CFG_deviation")+1);
    }
    
    int SMC = mResult->fomResult->survived_count;
    if(SMC>non_trivial_FOM_buffer){
      mResult->fomResult->survived = realloc(mResult->fomResult->survived,(non_trivial_FOM_buffer*=2)*sizeof(Mutant));
    }
    
    //GEt valgrind results
    ValgrindResult* valgrindResult = genValgrindResult(cwd, original_file_Name_dir,str, makeDir,user_config);
    
    if((valgrindResult==NULL||(valgrindResult!=NULL && valgrindResult->valgrind_error_count==PUTValgrindErrors)) && cfgDeviation==0){
      mResult->fomResult->equivalent_mutants++;
      create_update_gstat_mutation(mutation_code,"equivalent_mutants",get_gstat_value_mutation(mutation_code,"equivalent_mutants")+1);
   
    }else if((valgrindResult!=NULL && valgrindResult->valgrind_error_count!=PUTValgrindErrors)&& cfgDeviation>0){
       mResult->fomResult->survived_SD++;
       create_update_gstat_mutation(mutation_code,"survived_SD",get_gstat_value_mutation(mutation_code,"survived_SD")+1);
   
    }else if((valgrindResult!=NULL && valgrindResult->valgrind_error_count!=PUTValgrindErrors)){
      mResult->fomResult->survived_WD++;
      mResult->fomResult->survived_WD_causedValgrindErrors++;
      create_update_gstat_mutation(mutation_code,"survived_WD",get_gstat_value_mutation(mutation_code,"survived_WD")+1);
      create_update_gstat_mutation(mutation_code,"survived_WD_VE",get_gstat_value_mutation(mutation_code,"survived_WD_VE")+1);
   
    }else if(cfgDeviation>0){
      mResult->fomResult->survived_WD++;
      mResult->fomResult->survived_WD_caused_CFG_deviation++;
      create_update_gstat_mutation(mutation_code,"survived_WD",get_gstat_value_mutation(mutation_code,"survived_WD")+1);
      create_update_gstat_mutation(mutation_code,"survived_WD_CFD",get_gstat_value_mutation(mutation_code,"survived_WD_CFD")+1);
    }
    
    if(valgrindResult!=NULL){
       mResult->fomResult->total_valgrind_errors+=valgrindResult->valgrind_error_count;
       printValgrindResult(mutation_code,valgrindResult);
    }
    
    //Store results in the Mutant
    survived_ptr[SMC].mutant_source_file = calloc(sizeof(char),strlen(filename_qfd)+1);
    survived_ptr[SMC].mutation_code = calloc(sizeof(char),strlen(mutation_code)+1);
    strncpy(survived_ptr[SMC].mutant_source_file,filename_qfd,strlen(filename_qfd));
    strncpy(survived_ptr[SMC].mutation_code,mutation_code,strlen(mutation_code));
    
    survived_ptr[SMC].valgrindResult=valgrindResult;
    survived_ptr[SMC].cfgDeviation=cfgDeviation;
    survived_ptr[SMC].damage=((2/3.0)+(survived_ptr[SMC].cfgDeviation/3.0));
    mResult->fomResult->survived_count++;
  }
  mResult->fomResult->original_source_file=mv_dir;   
  
  //If verbose_mode is selected, print the mutation itself, diff
  if(verbose_mode==1){
    args_diff = calloc(sizeof(char*),5);
    mutation_results = fopen(mutation_results_path,"a+");
    fprintf(mutation_results, "PUT vs. Mutant : ");
    fflush(mutation_results);
    fclose(mutation_results);
    
    args_diff[0]="diff";
    args_diff[1]="-wb";
    args_diff[2]=txl_original;
    args_diff[3]=filename_qfd;
    args_diff[4]=NULL;
    startprogram(args_diff,mutation_results_path,1);
    free(args_diff);
  }
  flush_GStats(GSTATS_PATH);
  close_GStats(GSTATS_PATH);
}

MResult* inject_mutations(char* srcDir,char*target,char*makeDir,char* original_file_Name,Config *user_config,char* txl_original,char*original_file_Name_dir,char*cwd){
  
  int non_trivial_FOM_buffer = 200;
  
  MResult *mResult = malloc(sizeof(MResult));
  mResult->fomResult= malloc(sizeof(FOMResult));
  mResult->homResult = malloc(sizeof(HOMResult));
  
  //Initialization 
  mResult->fomResult->original_source_file=NULL;
  mResult->fomResult->mutant_kill_count=0;
  mResult->fomResult->total_mutants=0;
  mResult->fomResult->non_trivial_FOM_count=0;
  mResult->fomResult->survived=0;
  mResult->fomResult->non_trivial_FOMS = malloc(non_trivial_FOM_buffer*sizeof(Mutant));
  mResult->fomResult->survived=malloc(non_trivial_FOM_buffer*sizeof(Mutant));
  mResult->fomResult->killed_by_valgrind=0;
  mResult->fomResult->caused_CFG_deviation=0;
  mResult->fomResult->survived_count=0;
  mResult->fomResult->total_valgrind_errors=0;
  mResult->fomResult->survived_caused_CFG_deviation=0;
  mResult->fomResult->survived_killed_by_valgrind=0;
  mResult->fomResult->total_tests=0;
  mResult->homResult->mutant_kill_count=0;
  mResult->homResult->total_mutants=0;

  mResult->fomResult->equivalent_mutants=0;
  //Survived
  mResult->fomResult->survived_count=0;
  mResult->fomResult->survived_SD=0;
  mResult->fomResult->survived_WD=0;
  mResult->fomResult->survived_WD_causedValgrindErrors=0;
  mResult->fomResult->survived_WD_caused_CFG_deviation=0;
  //Killed
  mResult->fomResult->mutant_kill_count=0;
  mResult->fomResult->killed_SD=0;
  mResult->fomResult->killed_WD=0;
  mResult->fomResult->killed_WD_causedValgrindErrors=0;
  mResult->fomResult->killed_WD_caused_CFG_deviation=0;
  mResult->fomResult->failed_injection=0;
  mResult->fomResult->dumb_mutants=0;
  
  //Backup original file to be replaced after every injection of mutations
  char *tmp = malloc(snprintf(NULL, 0, "%s/%s", srcDir,original_file_Name ) + 1);
  sprintf(tmp, "%s/%s",srcDir , original_file_Name);
  char *mv_dir = malloc(snprintf(NULL, 0, "%s.original", tmp) + 1);
  sprintf(mv_dir, "%s.original",tmp);
  
  int ret = rename(target, mv_dir);
  if(ret != 0)
  {
    fprintf(stderr, "Can't move %s\n", target);
    exit(EXIT_FAILURE);
  }
  
  struct dirent *dp;
  DIR *dfd;
  
  if ((dfd = opendir(srcDir)) == NULL)
  {
    fprintf(stderr, "Can't open %s\n", srcDir);
    exit(EXIT_FAILURE);
  }
  
  char filename_qfd[1024] ;
  
  while ((dp = readdir(dfd)) != NULL)
  {
    struct stat stbuf ;
    sprintf( filename_qfd , "%s/%s",srcDir,dp->d_name) ;
    if( stat(filename_qfd,&stbuf ) == -1 )
    {
      printf("Unable to stat file: %s\n",filename_qfd) ;
      continue ;
    }
    
    if ( ( stbuf.st_mode & S_IFMT ) == S_IFDIR )
    {
      continue;
      // Skip directories
    }else if(strcmp(original_file_Name, dp->d_name)!=0 && strcmp("txl_original.c", dp->d_name)!=0) {
      char *dot = strrchr(dp->d_name, '.');
      if (dot && !strcmp(dot, ".c")){
	char*str = malloc((sizeof(char)*strlen(dp->d_name))+1);
	strcpy(str, dp->d_name);
	copy_file(filename_qfd, target);
	
	genResultsFOM(str,makeDir,filename_qfd,mv_dir,user_config,mResult,non_trivial_FOM_buffer,txl_original,original_file_Name_dir,cwd);
	free(str);
      }
    }
  }
  return mResult;
}


double genHOMFitness(char* srcDir,char*target,char*makeDir,Config *user_config,char*original_source,MResult *mResult,HOMutant *hom){
  
  char **args_combineFOM = malloc(sizeof(char*)*(hom->FOMutants_count+4));
  args_combineFOM[0]="bash";
  args_combineFOM[1]="combineFOM.sh";
  args_combineFOM[2]=original_source;
  args_combineFOM[hom->FOMutants_count+2]=NULL;
  
  //Merge all the FOM source files to generate the HOM
  int i;
  for (i=0;i<hom->FOMutants_count;i++){
    args_combineFOM[i+3]=hom->FOMutants[i].mutant_source_file;
  }
  startprogram(args_combineFOM,NULL,0);
  free(args_combineFOM);
  
  char hom_dir[strlen(srcDir)+strlen("hom_dir.log")+2];
  sprintf(hom_dir,"%s/%s", srcDir, "hom_dir.log");
  
  char hom_dir_loc[strlen(srcDir)+20];
  char hom_file_name[strlen(srcDir)+20];
  
  FILE* hom_dir_file =fopen(hom_dir,"r");
  if(hom_dir_file==NULL){
    return -1;
  }
  fscanf (hom_dir_file, "%s %s",hom_file_name , hom_dir_loc);
  fclose(hom_dir_file);
  
  copy_file(hom_dir_loc, target);
  
  //Evaluate mutant
  //Run make on the mutated project in order to build it
  printf("--> Evaluating HOM: %s\n",hom_file_name);
  
  //Open log file for recording mutation results
  mutation_results = fopen(mutation_results_path,"a+");
  fprintf(mutation_results, "\n**** Mutant: %s ****\n",hom_file_name);
  fflush(mutation_results);
  fclose(mutation_results);
  mResult->homResult->total_mutants++;
  
  //Get mutants killed by tests before new evaluation
  int prev_killed_by_tests=get_non_trivial_FOM_stats()[0];
  
  if(runMake(makeDir,hom_file_name,user_config->makeTestTarget)==2){
    mResult->homResult->mutant_kill_count++;
  }
  
  //Get mutants killed by tests after evaluation
  int *stats = get_non_trivial_FOM_stats();
  
  if(stats[0]-prev_killed_by_tests==1){
    mResult->homResult->mutant_kill_count++;
  }
  hom->fragility=((double)stats[1]/(double)stats[2]);
  
  //Generate the fragility for all the set of FOMs that makeup this HOM
  hashset_t test_killed_set = hashset_create();
  if (test_killed_set == NULL) {
    fprintf(stderr, "failed to create hashset instance\n");
    abort();
  }
  
  int mut,c;
  for(mut=0;mut<hom->FOMutants_count;mut++){
    Mutant mutant = hom->FOMutants[mut];
    for(c=0;c<mutant.killed_by_tests_count;c++){
      hashset_add(test_killed_set, &mutant.killed_by_tests[c]);
    }
  }
  hom->fitness=hom->fragility/(((double)hashset_num_items(test_killed_set)/(double)stats[2]));
  hashset_destroy(test_killed_set);
  return hom->fitness;
}

void generateSubsumingHOMs(char* srcDir,char*target,char*makeDir,Config *user_config,char*original_source,MResult *mResult){
  
  if(!((mResult->fomResult->mutant_kill_count!=mResult->fomResult->total_mutants)&&(mResult->fomResult->non_trivial_FOM_count!=0))){
    return;
  }
  
  printf("\n--> Successfully generated mutations (HOMs) for: %s in %s, now performing injections <---\n\n",target,srcDir);  
  
  srand(time(NULL));
  int i=0;
  int hom_list_counter=0;
  int no_new_optimum=0;
  int hom_list_size=1<<mResult->fomResult->non_trivial_FOM_count;
  
  HOMutant *hom_list = malloc(hom_list_size *sizeof(HOMutant));
  
  
  while(no_new_optimum<mResult->fomResult->non_trivial_FOM_count*2){
    //Select a random mutant from non-trivial FOMs
    int r = rand()%(mResult->fomResult->non_trivial_FOM_count);
    Mutant *rand_fom = &mResult->fomResult->non_trivial_FOMS[r];
    HOMutant *optimum_hom=NULL;
    
    
    for(i=0;i<mResult->fomResult->non_trivial_FOM_count;i++){
      //Make sure that we do not combine the same FOMs
      if(i==r){
	continue;
      }
      
      HOMutant *tmp_hom = malloc(sizeof(HOMutant));
      //Any HOM can be made up of atmost the maximum number of non-trivial FOMs
      tmp_hom->FOMutants = malloc(mResult->fomResult->non_trivial_FOM_count*sizeof(Mutant));
      tmp_hom->FOMutants_count=0;
      tmp_hom->fragility=0.0;
      tmp_hom->cfgDeviation=0.0;
      tmp_hom->fitness=0.0;
      //Combine the FOMs in the optimum_hom with another fom in the population
      if(optimum_hom!=NULL){
	memcpy(tmp_hom->FOMutants,optimum_hom->FOMutants,mResult->fomResult->non_trivial_FOM_count*sizeof(Mutant));
	tmp_hom->FOMutants[optimum_hom->FOMutants_count]=mResult->fomResult->non_trivial_FOMS[i];
	tmp_hom->FOMutants_count=optimum_hom->FOMutants_count+1;
      }else{
	//Combine the randomly selected FOM with another FOM from the population
	tmp_hom->FOMutants[0]=*rand_fom;
	tmp_hom->FOMutants[1]=mResult->fomResult->non_trivial_FOMS[i];
	tmp_hom->FOMutants_count=2;	
      }
      
      //Evaluate fitness of the new HOM to determine the optimum_hom
      double tmp_hom_fitness = genHOMFitness(srcDir,target,makeDir,user_config,original_source,mResult,tmp_hom);
      
      if(tmp_hom_fitness!=-1){
	if(optimum_hom==NULL){
	  optimum_hom = tmp_hom;
	}else if(tmp_hom_fitness >0 && tmp_hom_fitness <1){
	  if(tmp_hom_fitness >optimum_hom->fitness){
	    optimum_hom = tmp_hom;
	  }
	}
      }
      if(optimum_hom==NULL){
	no_new_optimum++;
      }
      
      //Archive tmp_hom
      if(hom_list_counter==hom_list_size){
	hom_list_size=hom_list_size*2;
	hom_list = realloc(hom_list,hom_list_size*sizeof(HOMutant));
      }
      if(tmp_hom_fitness!=-1){
	hom_list[hom_list_counter++]=*tmp_hom;
      }
    }
  }
}

double calculate_average_damage(FOMResult * fomResult){
  open_GStats(GSTATS_PATH);
  //update and calculate damage
  double average_damage =0.0;
  int total_valgrind_errors = fomResult->total_valgrind_errors;
  Mutant *survived_ptr = fomResult->survived;
  Mutant *non_trivial_FOMS_ptr = fomResult->non_trivial_FOMS;
  
  int s;
  for(s=0;s<fomResult->survived_count;s++){
    Mutant m = survived_ptr[s];
    if(m.valgrindResult!=NULL){
      m.damage+=(((m.valgrindResult->valgrind_error_count)/total_valgrind_errors)/3.0);
      create_update_gstat_mutation(m.mutation_code,"damage_raw",get_gstat_value_mutation(m.mutation_code,"damage_raw")+(m.damage*100));
    }
    average_damage+=m.damage;
  }
  for(s=0;s<fomResult->non_trivial_FOM_count;s++){
    Mutant m = non_trivial_FOMS_ptr[s];
    if(m.valgrindResult!=NULL){
      m.damage+=(((m.valgrindResult->valgrind_error_count)/total_valgrind_errors)/3.0);
      create_update_gstat_mutation(m.mutation_code,"damage_raw",get_gstat_value_mutation(m.mutation_code,"damage_raw")+(m.damage*100));
    }
    average_damage+=m.damage;
  }
  flush_GStats(GSTATS_PATH);
  close_GStats(GSTATS_PATH);
  return average_damage==0.0?0:average_damage/(double)(fomResult->total_mutants-fomResult->failed_injection);
}


void process_source_file(char*s,char * cwd,char*copy_put,char**args_txl,char**source,Config *user_config){
  remove(mutation_results_path);
  
  temp_results =fopen(temp_results_path,"w+");
  fwrite("0",1,1,temp_results);
  fclose(temp_results);
  
  //Extract dir from target
  char *chptr = strrchr(args_txl[4], '/');
  long dif = chptr - args_txl[4];
  
  //Extract file name in order to rename mutants
  char *original_file_Name =strndup(chptr+1,strlen(args_txl[4])-dif);
  char *original_file_Name_dir =strndup(chptr+1,strlen(args_txl[4])-(dif+3));
  char *makeDir =strndup(args_txl[4],dif);
  
  char * args3 = malloc(snprintf(NULL, 0, "%s/%s/%s/%s", cwd,"mutation_out", original_file_Name_dir,"txl_original.c") + 1);
  sprintf(args3,"%s/%s/%s/%s", cwd,"mutation_out", original_file_Name_dir,"txl_original.c");
  args_txl[3]=args3;
  
  //Create the folder that stores the mutations
  char m_dir[strlen(cwd)+strlen("mutation_out")+strlen(original_file_Name_dir)+3];
  sprintf(m_dir,"%s/mutation_out/%s",cwd,original_file_Name_dir);
  mkdir(m_dir,S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  args_txl[7]=m_dir;
  
  printf("\n");
  
  startprogram(args_txl,NULL,0);
  
  printf("\n*-------------------------------------------------------------------------*\n* Mutating %s \n*-------------------------------------------------------------------------*\n\n",*source);
  
  char *mut_out_dir = malloc(snprintf(NULL, 0, "%s/%s/%s", cwd,"mutation_out", original_file_Name_dir) + 1);
  sprintf(mut_out_dir,"%s/%s/%s", cwd,"mutation_out", original_file_Name_dir);
  
  make_logs_dir = malloc(snprintf(NULL, 0, "%s/%s", mut_out_dir , "make_logs.log") + 1);
  sprintf(make_logs_dir,"%s/%s", mut_out_dir , "make_logs.log");
  
  //Start log file for recording mutation builds
  make_logs = open(make_logs_dir, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
  close(make_logs);
  
  printf("\n--> Successfully generated mutations (FOMs) for: %s in mutation_out/%s, now performing injections <---\n\n",*source,original_file_Name_dir);
  MResult* mResult = inject_mutations(mut_out_dir,args_txl[4],makeDir,original_file_Name,user_config,args3,original_file_Name_dir,cwd);
  
  char *mut_out_dir_hom = malloc(snprintf(NULL, 0, "%s/%s", mut_out_dir, "HOM") + 1);
  sprintf(mut_out_dir_hom,"%s/%s", mut_out_dir, "HOM");
  
  
  if(mResult->fomResult->total_mutants>0){
    //Generate HOMs from FOMs
    //generateSubsumingHOMs(mut_out_dir_hom,args_txl[4],copy_put,user_config,args3,mResult);
    //Re-inject the original source file into the project
    copy_file(mResult->fomResult->original_source_file, args_txl[4]);
    
    double average_damage = calculate_average_damage(mResult->fomResult);
    
    printf("\n*-------------------------------------------------------------------------*\n* Results for %s:\n*-------------------------------------------------------------------------*\n* Generated Mutants: %d \n* Mutants survived: %d (%.2f%%)\n*  > Equivalent: %d (%.2f%%)\n*  > Strongly damaging: %d (%.2f%%)\n*  > Weakly damaging: %d (%.2f%%)\n*\t> Caused Valgrind errors: %d (%.2f%%)\n*\t> Caused CFG deviation: %d (%.2f%%)\n* Mutants killed: %d (%.2f%%)\n*  > Strongly damaging: %d (%.2f%%)\n*  > Weakly damaging: %d (%.2f%%)\n*\t> Caused Valgrind errors: %d (%.2f%%)\n*\t> Caused CFG deviation: %d (%.2f%%)\n*  > Failed Injection: %d (%.2f%%)\n* Dumb mutants: %d (%.2f%%)\n*-------------------------------------------------------------------------*\n* Total Valgrind memory errors generated: %d\n* Average mutant damage score: %.2f\n* Mutation score: %f\n*-------------------------------------------------------------------------*\n",*source,
	   mResult->fomResult->total_mutants, 
	   mResult->fomResult->survived_count,
	   ((double)mResult->fomResult->survived_count/(double)mResult->fomResult->total_mutants)*100,
	   mResult->fomResult->equivalent_mutants,
	    ((double)mResult->fomResult->equivalent_mutants/(double)mResult->fomResult->survived_count)*100,
	   mResult->fomResult->survived_SD,
	   ((double)mResult->fomResult->survived_SD/(double)mResult->fomResult->survived_count)*100,
	   mResult->fomResult->survived_WD,
	   ((double)mResult->fomResult->survived_WD/(double)mResult->fomResult->survived_count)*100,
	   mResult->fomResult->survived_WD_causedValgrindErrors,
	   ((double)mResult->fomResult->survived_WD_causedValgrindErrors/(double)mResult->fomResult->survived_WD)*100,
	   mResult->fomResult->survived_WD_caused_CFG_deviation,
	   ((double)mResult->fomResult->survived_WD_caused_CFG_deviation/(double)mResult->fomResult->survived_WD)*100,
	   mResult->fomResult->mutant_kill_count,
	   ((double)mResult->fomResult->mutant_kill_count/(double)mResult->fomResult->total_mutants)*100,
	   mResult->fomResult->killed_SD,
	   ((double)mResult->fomResult->killed_SD/(double)mResult->fomResult->mutant_kill_count)*100,
	   mResult->fomResult->killed_WD,
	    ((double)mResult->fomResult->killed_WD/(double)mResult->fomResult->mutant_kill_count)*100,
	   mResult->fomResult->killed_WD_causedValgrindErrors,
	   ((double)mResult->fomResult->killed_WD_causedValgrindErrors/(double)mResult->fomResult->killed_WD)*100,
	   mResult->fomResult->killed_WD_caused_CFG_deviation,
	   ((double)mResult->fomResult->killed_WD_caused_CFG_deviation/(double)mResult->fomResult->killed_WD)*100,
	   mResult->fomResult->failed_injection,
	   ((double)mResult->fomResult->failed_injection/(double)mResult->fomResult->mutant_kill_count)*100,
	   mResult->fomResult->dumb_mutants,
	   ((double)mResult->fomResult->dumb_mutants/(double)mResult->fomResult->total_mutants)*100,
	   mResult->fomResult->total_valgrind_errors,
	   average_damage,
	   ((double)mResult->fomResult->mutant_kill_count/(double)mResult->fomResult->total_mutants)	
    );
    
    //Update gstats with the aggregated resuts of this run
    open_GStats(GSTATS_PATH);
    create_update_aggr_results("total_tests_run",get_gstat_value_aggr_results("total_tests_run")+mResult->fomResult->total_tests);
    //create_update_aggr_results("total_valgrind_errors",get_gstat_value_aggr_results("total_valgrind_errors")+mResult->fomResult->total_valgrind_errors);
    
    //create_update_aggr_results("total_mutants",get_gstat_value_aggr_results("total_mutants")+mResult->fomResult->total_mutants);
    //create_update_aggr_results("mutant_kill_count",get_gstat_value_aggr_results("mutant_kill_count")+mResult->fomResult->mutant_kill_count);
    //create_update_aggr_results("non_trivial_count",get_gstat_value_aggr_results("non_trivial_count")+mResult->fomResult->non_trivial_FOM_count);
    //create_update_aggr_results("dumb_count",get_gstat_value_aggr_results("dumb_count")+mResult->fomResult->dumb_mutants);
    //create_update_aggr_results("survived_count",get_gstat_value_aggr_results("survived_count")+mResult->fomResult->survived_count);
    //create_update_aggr_results("killed_by_valgrind_count",get_gstat_value_aggr_results("killed_by_valgrind_count")+mResult->fomResult->killed_by_valgrind);
    flush_GStats(GSTATS_PATH);
    generate_derived_stats();
    flush_GStats(GSTATS_PATH);
    close_GStats(GSTATS_PATH);
    
    //Show results
    char*args_cat[]={"cat",mutation_results_path,NULL};
    startprogram(args_cat,NULL,0);   
  }else{
    printf("\n*-------------------------------------------------------------------------*\n* Results for %s:\n* Total Mutants: 0 (FOM) 0 (HOM)\n*-------------------------------------------------------------------------*\n",*source);
  }
  
  //Free resources
  //free(original_file_Name);
  //free(original_file_Name_dir);
  //free(s);
  //free(args3);
  //free(mut_out_dir);
  //free(mut_out_dir_hom);
  //free(mResult->fomResult);
  //free(mResult->homResult);
  //free(mResult);
  //free(args_txl[4]);
  //free(cwd);
}


void process_source_directory(char *dir,char *cwd,char*copy_put,char**args_txl,char**source,Config *user_config)
{
  DIR *dp;
  struct dirent *entry;
  struct stat statbuf;
  
  if((dp = opendir(dir)) == NULL) {
    fprintf(stderr,"cannot open directory: %s\n", dir);
    return;
  }
  chdir(dir);
  while((entry = readdir(dp)) != NULL) {
    lstat(entry->d_name,&statbuf);
    if(S_ISDIR(statbuf.st_mode)) {
      /* Found a directory, but ignore . and .. */
      if(strcmp(".",entry->d_name) == 0 ||
	strcmp("..",entry->d_name) == 0){
	continue;
	}
	/* New dir path */
	char *newDir = malloc(snprintf(NULL, 0, "%s/%s",dir,entry->d_name ) + 1);
      sprintf(newDir, "%s/%s",dir,entry->d_name);
      /* Recurse at a new indent level */
      process_source_directory(newDir,cwd,copy_put,args_txl,source,user_config);
    }
    else{
      char *dot = strrchr(entry->d_name, '.');
      if (dot && !strcmp(dot, ".c")){
	char *file = malloc(snprintf(NULL, 0, "%s/%s", dir,entry->d_name ) + 1);
	sprintf(file, "%s/%s",dir,entry->d_name);
	args_txl[4]=file;
	process_source_file(file,cwd,copy_put,args_txl,source,user_config);
	free(file);
      }
    }
  }
  chdir("..");
  closedir(dp);
}


int main(int argc, char**argv) {
  
  Config *user_config;
  char * cwd = getcwd(NULL, 0);
  int args_index=1;
  
  //Set "gstats.xml" location
  GSTATS_PATH=malloc(snprintf(NULL, 0, "%s/gstats.xml", cwd) + 1);
  sprintf(GSTATS_PATH, "%s/gstats.xml", cwd);
  
  if(argc==2){
    open_GStats(GSTATS_PATH);
    if(strcmp(argv[args_index],"--gstats-clear")==0){
      if(clear_GStats(GSTATS_PATH)==0){
	printf("gstat-status: File %s  deleted.\n", GSTATS_PATH);
	exit(EXIT_SUCCESS);
      }else{
	fprintf(stderr, "gstat-status: Error deleting the file %s.\n", GSTATS_PATH);
	exit(EXIT_FAILURE);
      }
    }else if(strcmp(argv[args_index],"--gstats-collect")==0){
      //Show stats
      char*args_cat[]={"cat",GSTATS_PATH,NULL};
      startprogram(args_cat,NULL,0);
      exit(EXIT_SUCCESS);
    }
    close_GStats(GSTATS_PATH);
  }
  
  if(strcmp(argv[args_index],"--gstreamer")==0){
    args_index++;
    preset_project=1;
  }
  if(strcmp(argv[args_index],"-v")==0){
    user_config = processConfigFile(argv[2]);
    verbose_mode=1;
    args_index++;
  }
  
  user_config = processConfigFile(argv[args_index]);
  
  if(user_config==NULL){
    fprintf(stderr,"error: incorrect arguments\n");
    exit(EXIT_FAILURE);
  }
  
  if (cwd== NULL){
    perror("getcwd() error");
    exit(EXIT_FAILURE);
  }
  if (user_config) {
    char**source=user_config->source;
    
    //Duplicate the project in order to revert back to after mutations
    char *chptr = strrchr(user_config->sourceRootDir, '/');
    long dif = chptr - user_config->sourceRootDir;
    char *copy_put =strndup(chptr+1, strlen(user_config->sourceRootDir)-dif);
    
    //Remove the copy of the program under test if it exists due to an execution terminated by the user
    cleanUp(copy_put);
    cleanUp("mutation_out");
    
    //Create mutation_out
    mkdir("mutation_out",S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    //Create gcov_out
    mkdir("mutation_out/PUT",S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    
    //copy PUT
    char *args_cp[5];
    args_cp[0]="cp";
    args_cp[1]="-r";
    args_cp[2]=user_config->sourceRootDir;
    args_cp[3]=copy_put;
    args_cp[4]=NULL;
    startprogram(args_cp,NULL,0);
    
    //Generate initial stats for PUT
    setenv("IS_MUTATE","false",1);
    runMake(copy_put,NULL,user_config->makeTestTarget);
    // CFG data for the PUT
    extractCFGBranches(copy_put,"mutation_out/PUT",NULL);
    // Valgrind data for the PUT
    ValgrindResult* valgrindResultPUT = genValgrindResult(cwd,"PUT","PUT",copy_put,user_config);
    if(valgrindResultPUT!=NULL){
      PUTValgrindErrors = valgrindResultPUT->valgrind_error_count;
    }
    setenv("IS_MUTATE","true",1);
    
    if(user_config->testingFramework==1){
      //Replace the CuTest library in the project with our modified library
      char * cutest_source = malloc(snprintf(NULL, 0, "%s/%s", copy_put,user_config->CuTestLibSource) + 1);
      sprintf(cutest_source, "%s/%s",copy_put , user_config->CuTestLibSource);
      copy_file("CuTest/CuTest.forked.c",cutest_source);
      free(cutest_source);
    }else if(user_config->testingFramework==2 && user_config->CheckTestLibSource!=NULL){
      //Replace the Check library in the project with our modified library
      
      char * check_source = malloc(snprintf(NULL, 0, "%s/%s", copy_put,user_config->CheckTestLibSource) + 1);
      sprintf(check_source, "%s/%s",copy_put , user_config->CheckTestLibSource);
      
      //Check if a preset project is being run
      if(preset_project==1){
	copy_file("libcheck-presets/check.c.gstreamer",check_source);
      }else{
	copy_file("libcheck/src/check.c",check_source);
      }
      free(check_source);
    }
    
    //Initialize txl arguments in order to perform mutations on each source file
    char**args_txl =  malloc(9 * sizeof(char*));;
    args_txl[0]="txl";
    args_txl[1]="mutator.Txl";
    args_txl[2]="-o";
    args_txl[5]="-";
    args_txl[6]="-mut_out";
    args_txl[8]=NULL;        
    
    //mutator.txl path
    args_txl[1]=malloc(snprintf(NULL, 0, "%s/mutator.Txl",cwd) + 1);
    sprintf(args_txl[1],"%s/mutator.Txl",cwd);
    
    //Store mutation_results_path
    mutation_results_path = malloc(snprintf(NULL, 0, "%s/%s/%s",cwd, copy_put,"mutation_results.log") + 1);
    sprintf(mutation_results_path, "%s/%s/%s",cwd, copy_put, "mutation_results.log");
    
    //Create a temporary log file in order to keep track of mutation results
    temp_results_path = malloc(snprintf(NULL, 0, "%s/%s/%s",cwd, copy_put,"temp_results.log") + 1);
    sprintf(temp_results_path, "%s/%s/%s",cwd, copy_put,"temp_results.log");
    
    //Create temparary environment variables to store temp_results_path and mutation_results_path to be accessed by child processes
    setenv("MUTATION_RESULTS_PATH",mutation_results_path,1);
    setenv("TEMP_RESULTS_PATH",temp_results_path,1);
    
    //Because we run the test suite again from Valgrind, in order to prevent duplicate output, we uniquely identify this program
    setenv("IS_MUTATE","true",1);
    
    int sc;
    struct stat s;
    
    for(sc=0;sc < user_config->numberOfSource;sc++){
      //get local path
      char *path = malloc(sizeof(char)*(snprintf(NULL, 0, "%s/%s/%s",cwd, copy_put, *source) + 1));
      sprintf(path, "%s/%s/%s",cwd, copy_put, *source);
      
      //Check if the path is a directory or a file
      if( stat(path,&s) == 0 )
      {
	if( s.st_mode & S_IFDIR )
	{
	  //it's a directory
	  process_source_directory(path,cwd,copy_put,args_txl,source,user_config);
	}
	else if( s.st_mode & S_IFREG )
	{
	  //it's a file
	  args_txl[4]=path;
	  process_source_file(path,cwd,copy_put,args_txl,source,user_config);
	}
      }
      source++;
    }
    
    //Remove the copy of the program under test we made earlier
    cleanUp(copy_put);
    printf("\n");
  }
  return 0;
}
