#include <stdlib.h>
#include <string.h>

#include "gstats.h"
#include "libxml_commons.h"

#define MUT_QUERY_PREFIX "//mutation_operator[code='"
#define MUT_QUERY_SUFFIX "']"

FILE* gstats;
xmlDocPtr doc;

int clear_GStats(){
  close_GStats();
  if(remove(GSTATS_PATH)==0){
    return 0;
  }else{
    return -1;
  }
}

void open_GStats(){
  gstats = fopen(GSTATS_PATH, "ab+");
  doc = getdoc(GSTATS_PATH);
  if(doc ==NULL){
    doc = xmlNewDoc(BAD_CAST "1.0");
    xmlDocSetRootElement(doc,xmlNewNode(NULL,(xmlChar*) "gstats"));
    flush_GStats();
  }
}

void close_GStats(){
  if(gstats!=NULL){
    fclose(gstats);
  }
  if(doc!=NULL){
    xmlFreeDoc(doc);
    xmlCleanupParser();
  }
}

void flush_GStats(){
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
  nodeset = result->nodesetval;
  
  //Check if one or none entry exists
  if(nodeset->nodeNr==1){
    return nodeset->nodeTab[0];
  }else if(nodeset->nodeNr==0){
    return NULL;
  }else{
    //We should never get here, this is a malformed gstats file
    fprintf(stderr,"error: malformed gstats file \n");
    exit(EXIT_FAILURE);
  }
}

int get_gstat_value(char*mut_code,char*key){
  xmlNodePtr mutation = find_mutation_by_code(mut_code);
  xmlChar* val =xmlGetProp(mutation,(xmlChar*)key);
  if(val==NULL){
    return 0;
  }else{
    int i = atoi((char*)val);
    free(val);
    return i;
  }
}

void create_update_GStats(char*mut_code,char*key,int value){
  xmlNodePtr mutation = find_mutation_by_code(mut_code);
  xmlChar* char_value = (xmlChar*)int_to_char_heap(value);
  if(mutation==NULL){
    //Create a new mutation_operator record
    xmlNewTextChild(xmlDocGetRootElement(doc),NULL,(xmlChar*)"mutation_operator",(xmlChar*)mut_code);
  }else{
    //Get attribute with key=key
    xmlAttrPtr attr= xmlHasProp(mutation, (xmlChar*)key);
    if(attr==NULL){
      //Create attribute
      xmlSetProp(mutation,(xmlChar*)key,char_value);
    }else{
      //Remove the child
      xmlUnsetProp(mutation, (xmlChar*)key);
      //Set with new value
      xmlSetProp(mutation,(xmlChar*)key,char_value);
    }
  }
  free(char_value);
}