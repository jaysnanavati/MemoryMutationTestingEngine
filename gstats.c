#include <stdlib.h>
#include <string.h>

#include "gstats.h"
#include "libxml_commons.h"

#define MUT_QUERY_PREFIX "//mutation_operator[text()='"
#define MUT_QUERY_SUFFIX "']"

FILE* gstats;
xmlDocPtr doc;

//Store aggregate types in order to calculate derived results later
int total_mutants=0;
int mutant_kill_count=0;
int non_trivial_count=0;
int dumb_count=0;
int survived_count=0;
int killed_by_valgrind_count=0;
int total_tests_run=0;
int total_infection_count=0;
int total_valgrind_errors=0;


int clear_GStats(char* GSTATS_PATH){
  close_GStats(GSTATS_PATH);
  if(remove(GSTATS_PATH)==0){
    return 0;
  }else{
    return -1;
  }
}

void open_GStats(char* GSTATS_PATH){
  gstats = fopen(GSTATS_PATH, "ab+");
  doc = getdoc(GSTATS_PATH);
  if(doc ==NULL){
    doc = xmlNewDoc(BAD_CAST "1.0");
    xmlDocSetRootElement(doc,xmlNewNode(NULL,(xmlChar*) "gstats"));
    xmlNodePtr aggregate_node = xmlNewTextChild(xmlDocGetRootElement(doc),NULL,(xmlChar*)"aggregate_stats",(xmlChar*)"");
    
    //Extract aggregate stats from previous runs
    char *tic = (char*)xmlGetProp(aggregate_node, (xmlChar*)"total_infection_count");
    if(tic!=NULL){
      total_infection_count = atoi(tic);
      xmlFree(tic);
    }
    flush_GStats(GSTATS_PATH);
  }
}

void close_GStats(char* GSTATS_PATH){
  if(gstats!=NULL){
    fclose(gstats);
  }
  if(doc!=NULL){
    xmlFreeDoc(doc);
    xmlCleanupParser();
  }
}

void flush_GStats(char* GSTATS_PATH){
  if(xmlSaveFormatFileEnc(GSTATS_PATH,doc,NULL,1)==-1){
    //We should never get here, this is a malformed gstats file
    fprintf(stderr,"error: unable to create gstats.xml \n");
    exit(EXIT_FAILURE);
  }
}

char* int_to_char_heap(int i){
  char *c = malloc(snprintf(NULL, 0, "%d", i) + 1);
  sprintf(c, "%d", i);
  return c;
}

char* double_to_char_heap(double d){
  char *c = malloc(snprintf(NULL, 0, "%G", d) + 1);
  sprintf(c, "%G", d);
  return c;
}

xmlNodePtr find_mutation_by_code(char*mut_code){
  //Check to see if an entry for the mut_code exists, else create and fill with children key,value
  xmlNodeSetPtr nodeset;
  xmlXPathObjectPtr result; 
  char xPath_query[strlen(MUT_QUERY_PREFIX)+strlen(mut_code)+strlen(MUT_QUERY_SUFFIX)+1];
  
  //Create the XPath query to find the entry for the mutant with code = mut_code
  sprintf(xPath_query, "%s%s%s",MUT_QUERY_PREFIX , mut_code,MUT_QUERY_SUFFIX);
  xmlChar *xpath = (xmlChar*) xPath_query;
  //Execute
  result = getnodeset (doc, xpath);
  if(result==NULL){
    return NULL;
  }else{
    nodeset = result->nodesetval;
    if(nodeset->nodeNr==1){
      return nodeset->nodeTab[0];
    }else{
      //We should never get here, this is a malformed gstats file
      fprintf(stderr,"error: malformed gstats file \n");
      exit(EXIT_FAILURE);
    }
  }
}

int get_gstat_value_mutation(char*mut_code,char*key){
  xmlNodePtr mutation = find_mutation_by_code(mut_code);
  if(mutation==NULL){
    return 0;
  }
  xmlChar* val =xmlGetProp(mutation,(xmlChar*)key);
  if(val==NULL){
    return 0;
  }else{
    int i = atoi((char*)val);
    free(val);
    return i;
  }
}

int get_gstat_value_aggr_results(char*key){
 
  xmlNodePtr aggregate_node = xmlDocGetRootElement(doc)->xmlChildrenNode;
    while(xmlStrcmp(aggregate_node->name, (const xmlChar *)"aggregate_stats")!=0){
      aggregate_node = xmlNextElementSibling(aggregate_node);
    }
  
  xmlChar* val =xmlGetProp(aggregate_node,(xmlChar*)key);
  if(val==NULL){
    return 0;
  }else{
    int i = atoi((char*)val);
    free(val);
    return i;
  }
}

void create_update_aggr_results(char*key,int value){
  
  if(strcmp(key,"total_tests_run")==0){
    total_tests_run=value;
  }else if(strcmp(key,"total_mutants")==0){
    total_mutants=value;
  }else if(strcmp(key,"mutant_kill_count")==0){
    mutant_kill_count=value;
  }else if(strcmp(key,"non_trivial_count")==0){
    non_trivial_count=value;
  }else if(strcmp(key,"dumb_count")==0){
    dumb_count=value;
  }else if(strcmp(key,"survived_count")==0){
    survived_count=value;
  }else if(strcmp(key,"killed_by_valgrind_count")==0){
    killed_by_valgrind_count=value;
  }else if(strcmp(key,"total_infection_count")==0){
    total_infection_count=value;
  }else if(strcmp(key,"total_valgrind_errors")==0){
    total_valgrind_errors=value;
  }
  
  xmlNodePtr aggregate_node = xmlDocGetRootElement(doc)->xmlChildrenNode;
    while(xmlStrcmp(aggregate_node->name, (const xmlChar *)"aggregate_stats")!=0){
      aggregate_node = xmlNextElementSibling(aggregate_node);
    }
  
  
  xmlChar* char_value = (xmlChar*)int_to_char_heap(value);
  
  xmlSetProp(aggregate_node,(xmlChar*)key,char_value);
  free(char_value);
}

void create_update_gstat_mutation(char*mut_code,char*key,int value){
  xmlNodePtr mutation = find_mutation_by_code(mut_code);
  xmlChar* char_value = (xmlChar*)int_to_char_heap(value);
  if(mutation==NULL){
    //Create a new mutation_operator record
    mutation =xmlNewTextChild(xmlDocGetRootElement(doc),NULL,(xmlChar*)"mutation_operator",(xmlChar*)mut_code);
  }
  xmlSetProp(mutation,(xmlChar*)key,char_value);
  free(char_value);
  
  //Update aggregate values
  if(strcmp(key,"infection_count")==0 && value!=0){
    total_infection_count++;
    create_update_aggr_results("total_infection_count",total_infection_count);
  }
}

void generate_derived_stats(){
  //For each mutation_operator entry generate derived results
  xmlNodeSetPtr nodeset;
  xmlXPathObjectPtr result; 
  xmlChar *xPath_query= (xmlChar*)"//mutation_operator";
  int i;
  
  //Execute
  result = getnodeset (doc, xPath_query);
  if(result==NULL){
    return;
  }else{
    nodeset = result->nodesetval;
    for (i=0; i < nodeset->nodeNr; i++) {
      xmlNodePtr mutation_operator = nodeset->nodeTab[i];
      char* gm= (char*)xmlGetProp(mutation_operator, (xmlChar*)"generated_mutants");
      int generated_mutants = gm==NULL?-1:atoi(gm);
      xmlFree(gm);
      
      //Average damage
      char* damage_count= (char*)xmlGetProp(mutation_operator, (xmlChar*)"damage_raw");
      if(damage_count!=NULL && generated_mutants!=-1){
	int rdc = atoi(damage_count);
	char*val = double_to_char_heap((double)(rdc/100.0)/(double)generated_mutants);
	xmlSetProp(mutation_operator,(xmlChar*)"damage",BAD_CAST val);
	xmlFree(damage_count);
	free(val);
      }
      
      //Fragilty:total number of tests that killed it vs total number of tests that were run
      char* killed_by_test_count= (char*)xmlGetProp(mutation_operator, (xmlChar*)"killed_by_tests");
      if(killed_by_test_count!=NULL && generated_mutants!=-1){
	int kbtc = atoi(killed_by_test_count);
	char*val = double_to_char_heap((double)kbtc/(double)generated_mutants);
	xmlSetProp(mutation_operator,(xmlChar*)"fragilty",BAD_CAST val);
	xmlFree(killed_by_test_count);
	free(val);
      }
      
      //Average CFG deviation
      char* cfd= (char*)xmlGetProp(mutation_operator, (xmlChar*)"cfg_deviation_raw");
      if(cfd!=NULL && generated_mutants!=-1){
	int icfd = atoi(cfd);
	char*val = double_to_char_heap((double)(icfd/100.0)/(double)generated_mutants);
	xmlSetProp(mutation_operator,(xmlChar*)"average_CFD",BAD_CAST val);
	xmlFree(cfd);
	free(val);
      }
      
      //Infectiveness: as a ratio of all other mutants, how many times was it able to infect the programs run
      char* infection_count= (char*)xmlGetProp(mutation_operator, (xmlChar*)"failed_injection");
      if(infection_count!=NULL && generated_mutants!=-1){
	int ic = atoi(infection_count);
	char*val = double_to_char_heap(1.0-((double)ic/(double)generated_mutants));
	xmlSetProp(mutation_operator,(xmlChar*)"infectiveness",BAD_CAST val);
	xmlFree(infection_count);
	free(val);
      }
      
      //Valgrind_immunity
      char* valgrind_survive_count= (char*)xmlGetProp(mutation_operator, (xmlChar*)"survived_WD_VE");
      char* valgrind_kill_count= (char*)xmlGetProp(mutation_operator, (xmlChar*)"killed_WD_VE");
      if(valgrind_survive_count==NULL && valgrind_kill_count==NULL){
	xmlSetProp(mutation_operator,(xmlChar*)"valgrind_immunity",BAD_CAST "0.0");
      }else if(valgrind_survive_count==NULL && valgrind_kill_count!=NULL){
	xmlSetProp(mutation_operator,(xmlChar*)"valgrind_immunity",BAD_CAST "0.0");
      }else if(valgrind_survive_count!=NULL && valgrind_kill_count==NULL){
	xmlSetProp(mutation_operator,(xmlChar*)"valgrind_immunity",BAD_CAST "1.0");
      }else if(valgrind_survive_count!=NULL && valgrind_kill_count!=NULL){
	int sv = atoi(valgrind_survive_count);
	int kv = atoi(valgrind_kill_count);
	char*val = double_to_char_heap((double)sv/(double)(sv+kv));
	xmlSetProp(mutation_operator,(xmlChar*)"valgrind_immunity",BAD_CAST val);
	xmlFree(valgrind_survive_count);
	xmlFree(valgrind_kill_count);
	free(val);
      }
      
      //CFG_immunity
      char* cfd_survive_count= (char*)xmlGetProp(mutation_operator, (xmlChar*)"survived_WD_CFD");
      char* cfd_kill_count= (char*)xmlGetProp(mutation_operator, (xmlChar*)"killed_WD_CFD");
      if(cfd_survive_count==NULL && cfd_kill_count==NULL){
	xmlSetProp(mutation_operator,(xmlChar*)"CFD_immunity",BAD_CAST "0.0");
      }else if(cfd_survive_count==NULL && cfd_kill_count!=NULL){
	xmlSetProp(mutation_operator,(xmlChar*)"CFD_immunity",BAD_CAST "0.0");
      }else if(cfd_survive_count!=NULL && cfd_kill_count==NULL){
	xmlSetProp(mutation_operator,(xmlChar*)"CFD_immunity",BAD_CAST "1.0");
      }else if(cfd_survive_count!=NULL && cfd_kill_count!=NULL){
	int scfd = atoi(cfd_survive_count);
	int kcfd = atoi(cfd_kill_count);
	char*val = double_to_char_heap((double)scfd/(double)(scfd+kcfd));
	xmlSetProp(mutation_operator,(xmlChar*)"CFD_immunity",BAD_CAST val);
	xmlFree(cfd_survive_count);
	xmlFree(cfd_kill_count);
	free(val);
      }
    }
  }
}
