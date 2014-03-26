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

#include "valgrindEval.h"
#include "gstats.h"

//With regards to the HOMs and the calculation of the fitness function, with regards to memory based operations, can we have a new parameter
//that we can optimize called: damage. As when looking at such mutations, although a mutant might be very fragile, but at the same time it might have
//greater damage to the memory model and vice versa, which we can uncover using memory tools such as valgrind. By adding this factor in our optimization 
//algorithm to identify mutants that do not fail a lot of tests AND have a great adverse affect to the memory modal rather than choosing only mutants
//that do not fail a lot of tests.

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
  char *mutant_source_file;
  //fragility is between 0->1
  double fragility;
  //set of tests which killed this mutant
  int *killed_by_tests;
  int killed_by_tests_count;
  ValgrindResult *valgrindResult;
} Mutant;

typedef struct{
  char *HOMutant_source_file;
  Mutant *FOMutants;
  //fragility is between 0->1
  double fragility;
  //Discard HOM when fitness =0 or 1, fitness is between 0->1
  double fitness;
  int FOMutants_count;
} HOMutant;


typedef struct{
  int total_mutants;
  int mutant_kill_count;
} HOMResult;

typedef struct{
  char *original_source_file;
  int mutant_kill_count;
  int non_trivial_FOM_count;
  int survived_count;
  Mutant *non_trivial_FOMS;
  Mutant *survived;
  int total_mutants;
  int total_tests;
  int killed_by_valgrind;
} FOMResult;


typedef struct{
  FOMResult *fomResult;
  HOMResult *homResult;
} MResult;


int make_logs;
int verbose_mode=0;
int preset_project=0;
FILE* mutation_results;
FILE* temp_results;
char* mutation_results_path;
char* make_logs_dir;
char* temp_results_path=NULL;

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
    char *title="--> MUTATION ::";
    char *mut_txt = malloc(snprintf(NULL, 0, "\n%s %s\n\n", title,currentMutation ) + 1);
    sprintf(mut_txt, "\n%s %s\n\n",title , currentMutation);
    write(make_logs,mut_txt,strlen(mut_txt));
    dup2(make_logs, 1);
    dup2(make_logs, 2);
    close(make_logs);
    execvp (args[0],args);
  }else{
    int status;
    if( waitpid(pid, &status, 0) < 0 ) {
      perror("wait");
    }
    if(WIFEXITED(status)) {
      return WEXITSTATUS(status);
    }
  }
  return -1;
}

int runMake(char*projectDir,char*currentMutation,char*makeTestTarget,MResult*mResult){
  char**args_make = calloc(sizeof(char*),5);
  args_make[0]="make";
  args_make[1]="-C";
  args_make[2]=malloc(sizeof(char)*strlen(projectDir)+1);
  strcpy(args_make[2],projectDir);
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
  char *pt = strtok (killed_test_id,",");
  while (pt != NULL) {
    if(counter>20){
      stats = realloc(stats,(counter*2)*sizeof(int*));
    }
    stats[counter++]= atoi(pt);
    pt = strtok (NULL, ",");
  }
  return stats;	    
}

void printValgrindError(ValgrindError *valgrindError,int i){
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

void printValgrindResult(ValgrindResult*valgrindResult){
  int i;
  mutation_results = fopen(mutation_results_path,"a+");
  fprintf(mutation_results, "Valgrind errors: %d\n",valgrindResult->valgrind_error_count);
  
  if(verbose_mode==1){
    for(i=0;i<valgrindResult->unique_valgrind_error_count;i++){
      int j=0;
      for(j=0;j<valgrindResult->valgrindErrors[i].occurrences;j++){
	printValgrindError(&(valgrindResult->valgrindErrors[i].siblings[j]),j+1);
      }
      printValgrindError(&valgrindResult->valgrindErrors[i],i+j);
    }
  }
  
  fflush(mutation_results);
  fclose(mutation_results);
}

void genResultsFOM(char *str,char* makeDir,char* filename_qfd,char*mv_dir,Config *user_config,MResult*mResult,int non_trivial_FOM_buffer,char*txl_original,char *original_file_Name_dir){
  
  char**args_diff;
  Mutant *non_trivial_FOMS_ptr = mResult->fomResult->non_trivial_FOMS;
  Mutant *survived_ptr = mResult->fomResult->survived;
  //Run make on the mutated project in order to build it
  printf("--> Evaluating FOM: %s\n",str);
  
  //Open log file for recording mutation results
  mutation_results = fopen(mutation_results_path,"a+");
  fprintf(mutation_results, "\n**** Mutant: %s ****\n",str);
  fflush(mutation_results);
  fclose(mutation_results);
  mResult->fomResult->total_mutants++;
  
  //Get mutants killed by tests before new evaluation
  int prev_killed_by_tests=get_non_trivial_FOM_stats()[0];
  
  //Evaluate mutant
  int make_result = runMake(makeDir,str,user_config->makeTestTarget,mResult);
  if(make_result==2){
    mResult->fomResult->mutant_kill_count++;
  }
  
  //Get mutants killed by tests after evaluation
  int *stats = get_non_trivial_FOM_stats();
  
  if(stats[0]-prev_killed_by_tests==1){
    mResult->fomResult->mutant_kill_count++;
    
    if(stats[0] >non_trivial_FOM_buffer){
      mResult->fomResult->non_trivial_FOMS = realloc(mResult->fomResult->non_trivial_FOMS,(non_trivial_FOM_buffer*=2)*sizeof(Mutant));
    }
    
    int NTFC = mResult->fomResult->non_trivial_FOM_count;
    
    if(stats[1]>0 && stats[1]<stats[2]){
      non_trivial_FOMS_ptr[NTFC].mutant_source_file = calloc(sizeof(char),strlen(filename_qfd)+1);
      strncpy(non_trivial_FOMS_ptr[NTFC].mutant_source_file,filename_qfd,strlen(filename_qfd));
      non_trivial_FOMS_ptr[NTFC].fragility=((double)stats[1]/(double)stats[2]);
      non_trivial_FOMS_ptr[NTFC].killed_by_tests = calloc(sizeof(int),stats[1]);
      non_trivial_FOMS_ptr[NTFC].killed_by_tests_count=stats[1];
      memcpy(non_trivial_FOMS_ptr[NTFC].killed_by_tests,stats+3,stats[1]*sizeof(int));
      mResult->fomResult->non_trivial_FOM_count++;
    }
  }else if(make_result!=2){
    int SMC = mResult->fomResult->survived_count;
    if(SMC>non_trivial_FOM_buffer){
      mResult->fomResult->survived = realloc(mResult->fomResult->survived,(non_trivial_FOM_buffer*=2)*sizeof(Mutant));
    }
    
    /*Call Valgrind(memcheck),memcheck to find out if it is an equivilent mutant as it has survived*/
    char **args_valgrind = calloc(sizeof(char*),8);
    char* xml_file_location = malloc(snprintf(NULL, 0, "mutation_out/%s/valgrind_eval_%s.xml",original_file_Name_dir, str) + 1);
    sprintf(xml_file_location, "mutation_out/%s/valgrind_eval_%s.xml",original_file_Name_dir, str);
    
    args_valgrind[0]="valgrind";
    args_valgrind[1]="-q";
    args_valgrind[2]="--trace-children=yes";
    args_valgrind[3]="--child-silent-after-fork=yes";
    args_valgrind[4]="--xml=yes";
    args_valgrind[5]= malloc(snprintf(NULL, 0, "--xml-file=%s",xml_file_location) + 1);
    sprintf(args_valgrind[5], "--xml-file=%s",xml_file_location);
    args_valgrind[6]=malloc(snprintf(NULL, 0, "%s/%s",makeDir, user_config->executablePath) + 1);
    sprintf(args_valgrind[6], "%s/%s",makeDir, user_config->executablePath);
    args_valgrind[7]=NULL;
    
    //Make sure we do not get duplicate output when lanching valgrind
    setenv("IS_MUTATE","false",1);
    
    startprogram(args_valgrind,NULL,1);
    
    //set it back so this process gets the right output
    setenv("IS_MUTATE","true",1);
    
    //evaluate the .xml output generated by Valgrind in order to generate statistics
    ValgrindResult* valgrindResult =valgrindXML_evaluate(xml_file_location,user_config->testingFramework);
    
    if(valgrindResult!=NULL){
      //Print results to the console
      printValgrindResult(valgrindResult);
    }
    //Store results in the Mutant
    survived_ptr[SMC].mutant_source_file = calloc(sizeof(char),strlen(filename_qfd)+1);
    strncpy(survived_ptr[SMC].mutant_source_file,filename_qfd,strlen(filename_qfd));
    
    if(valgrindResult!=NULL){
      mResult->fomResult->killed_by_valgrind++;
    }
    survived_ptr[SMC].valgrindResult=valgrindResult;
    
    mResult->fomResult->survived_count++;
    free(args_valgrind);
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
}

MResult* inject_mutations_CuTest(char* srcDir,char*target,char*makeDir,char* original_file_Name,Config *user_config,char* txl_original,char*original_file_Name_dir){
  
  int non_trivial_FOM_buffer = 20;
  
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
  mResult->fomResult->survived_count=0;
  mResult->homResult->mutant_kill_count=0;
  mResult->homResult->total_mutants=0;
  
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
	
	genResultsFOM(str,makeDir,filename_qfd,mv_dir,user_config,mResult,non_trivial_FOM_buffer,txl_original,original_file_Name_dir);
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
  
  if(runMake(makeDir,hom_file_name,user_config->makeTestTarget,mResult)==2){
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
      
      if(optimum_hom==NULL || (((tmp_hom_fitness >0) && (tmp_hom_fitness <1))&& tmp_hom_fitness >optimum_hom->fitness)){
	optimum_hom = tmp_hom;
      }else{
	no_new_optimum++;
      }
      
      //Archive tmp_hom
      if(hom_list_counter==hom_list_size){
	hom_list_size=hom_list_size*2;
	hom_list = realloc(hom_list,hom_list_size*sizeof(HOMutant));
      }
      hom_list[hom_list_counter++]=*tmp_hom;
    }
  }
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
  
  char * args3 = malloc(snprintf(NULL, 0, "%s/%s/%s", "mutation_out", original_file_Name_dir,"txl_original.c") + 1);
  sprintf(args3,"%s/%s/%s", "mutation_out", original_file_Name_dir,"txl_original.c");
  args_txl[3]=args3;
  
  args_txl[7]=original_file_Name_dir;
  
  printf("\n");
  
  if(startprogram(args_txl,NULL,0)!=0){
    return;
  }
  
  printf("\n*-------------------------------------------------------------------------*\n* Mutating %s \n*-------------------------------------------------------------------------*\n\n",*source);
  
  char *mut_out_dir = malloc(snprintf(NULL, 0, "%s/%s", "mutation_out", original_file_Name_dir) + 1);
  sprintf(mut_out_dir,"%s/%s", "mutation_out", original_file_Name_dir);
  
  make_logs_dir = malloc(snprintf(NULL, 0, "%s/%s", mut_out_dir , "make_logs.log") + 1);
  sprintf(make_logs_dir,"%s/%s", mut_out_dir , "make_logs.log");
  
  //Start log file for recording mutation builds
  make_logs = open(make_logs_dir, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
  close(make_logs);
  
  printf("\n--> Successfully generated mutations (FOMs) for: %s in %s, now performing injections <---\n\n",*source,mut_out_dir);
  MResult* mResult = inject_mutations_CuTest(mut_out_dir,args_txl[4],copy_put,original_file_Name,user_config,args3,original_file_Name_dir);
  
  char *mut_out_dir_hom = malloc(snprintf(NULL, 0, "%s/%s", mut_out_dir, "HOM") + 1);
  sprintf(mut_out_dir_hom,"%s/%s", mut_out_dir, "HOM");
  
  
  if(mResult->fomResult->total_mutants>0){
    //Generate HOMs from FOMs
    //generateSubsumingHOMs(mut_out_dir_hom,args_txl[4],copy_put,user_config,args3,mResult);
    //Re-inject the original source file into the project
    copy_file(mResult->fomResult->original_source_file, args_txl[4]);
    
    printf("\n*-------------------------------------------------------------------------*\n* Results for %s:\n*-------------------------------------------------------------------------*\n* Total Mutants: %d (FOM) %d (HOM)\n* Mutants killed: %d (FOM) %d (HOM)\n*  > Non-trivial mutants: %d (FOM)\n*  > Dumb mutants: %d (FOM)\n* Mutants survived: %d (FOM) %d (HOM)\n*  > Live/Equivalent mutants: %d (FOM)\n*  > Failed Valgrind checks: %d (FOM)\n*  > Caused inconsistent program flow: %d (FOM)\n* Mutation score: %G (FOM) %G (HOM)\n* Test coverage increased by: %G%%\n*-------------------------------------------------------------------------*\n",*source,
	   mResult->fomResult->total_mutants, 
	   mResult->homResult->total_mutants, 
	   mResult->fomResult->mutant_kill_count,
	   mResult->homResult->mutant_kill_count,
	   mResult->fomResult->non_trivial_FOM_count,
	   mResult->fomResult->mutant_kill_count-mResult->fomResult->non_trivial_FOM_count,
	   mResult->fomResult->survived_count,
	   0,
	   0,
	   mResult->fomResult->killed_by_valgrind,
	   mResult->homResult->total_mutants- mResult->homResult->mutant_kill_count,
	   mResult->fomResult->total_mutants!=0?(double)mResult->fomResult->mutant_kill_count/(double)mResult->fomResult->total_mutants:0,
	   mResult->homResult->total_mutants!=0?(double)mResult->homResult->mutant_kill_count/(double)mResult->homResult->total_mutants:0
	   ,0.0
    );
    
    //Show results
    char*args_cat[]={"cat",mutation_results_path,NULL};
    startprogram(args_cat,NULL,0);   
  }else{
    printf("\n*-------------------------------------------------------------------------*\n* Results for %s:\n* Total Mutants: 0 (FOM) 0 (HOM)\n*-------------------------------------------------------------------------*\n",*source);
  }
  
  //Free resources
  free(original_file_Name);
  free(original_file_Name_dir);
  free(s);
  free(args3);
  free(mut_out_dir);
  free(mut_out_dir_hom);
  free(mResult->fomResult);
  free(mResult->homResult);
  free(mResult);
  free(args_txl[4]);
  free(cwd);
}

void process_source_directory(char*srcDir,char * cwd,char*copy_put,char**args_txl,char**source,Config *user_config){
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
      //Recurse
      char fullpath[1024];
      snprintf(fullpath, sizeof(fullpath), "%s/%s", srcDir, dp->d_name);
      process_source_directory(fullpath,cwd,copy_put,args_txl,source,user_config);
      
    }else{
      char *dot = strrchr(dp->d_name, '.');
      if (dot && !strcmp(dot, ".c")){
	process_source_file(srcDir,cwd,copy_put,args_txl,source,user_config);
      }
    }
  }
}

int main(int argc, char**argv) {
  
  Config *user_config;
  char * cwd = getcwd(NULL, 0);
  int args_index=1;
  
  if(argc==2){
    open_GStats();
    if(strcmp(argv[args_index],"--gstats-clear")==0){
      if(clear_GStats()==0){
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
    }else{
      fprintf(stderr,"error: incorrect arguments\n");
      exit(EXIT_FAILURE);
    }
    close_GStats();
    
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
    
    char *args_cp[5];
    args_cp[0]="cp";
    args_cp[1]="-r";
    args_cp[2]=user_config->sourceRootDir;
    args_cp[3]=copy_put;
    args_cp[4]=NULL;
    startprogram(args_cp,NULL,0);
    
    if(user_config->testingFramework==1){
      //Replace the CuTest library in the project with our modified library
      char * cutest_source = malloc(snprintf(NULL, 0, "%s/%s", copy_put,user_config->CuTestLibSource) + 1);
      sprintf(cutest_source, "%s/%s",copy_put , user_config->CuTestLibSource);
      copy_file("CuTest/CuTest.forked.c",cutest_source);
      free(cutest_source);
    }else if(user_config->testingFramework==2){
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
      char *path = malloc(sizeof(char)*(snprintf(NULL, 0, "%s/%s", copy_put, *source) + 1));
      sprintf(path, "%s/%s", copy_put, *source);
      args_txl[4]=path;
      
      //Process the path depending on whether it is a direcotry or a file
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
	  process_source_file(path,cwd,copy_put,args_txl,source,user_config);
	}
      }
      else
      {
	//error
	printf("error: parsing source files\n");
	exit(EXIT_FAILURE);
      }
      source++;
    }
    
    //Remove the copy of the program under test we made earlier
    cleanUp(copy_put);
    
    printf("\n");
  }
  return 0;
}