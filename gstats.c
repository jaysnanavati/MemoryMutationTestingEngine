#include <stdlib.h>
#include <string.h>

#include "gstats.h"
#include "libxml_commons.h"

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
  //Check to see if an entry for the mut_code exists, else create and fill with children key,value
  xmlDocPtr doc;
  xmlNodeSetPtr nodeset;
  xmlXPathObjectPtr result;
  xmlNodePtr root_node;
  doc = getdoc(GSTATS_PATH);
  char *xPath_prefix = "//mutation_operator[code='";
  char *xPath_suffix = "']";
  char xPath_query[strlen(xPath_prefix)+strlen(mut_code)+strlen(xPath_suffix)+1];
  
  //Check if a file is empty, if it is, then create a new root element
  root_node=xmlDocGetRootElement(doc);
  if (!(root_node!=NULL && (!xmlStrcmp(root_node->name, (const xmlChar *)"gstats")))){
    root_node=xmlNewNode(NULL,(xmlChar*) "gstats");
    root_node=xmlDocSetRootElement(doc,root_node);
  }
  
  //Create the XPath query to find the entry for the mutant with code = mut_code
  sprintf(xPath_query, "%s%s%s",xPath_prefix , mut_code,xPath_suffix);
  xmlChar *xpath = (xmlChar*) xPath_query;
  //Execute
  result = getnodeset (doc, xpath);
  nodeset = result->nodesetval;
  
  //Check if one or none entry exists
  if(nodeset->nodeNr==1){
    
  }else if(nodeset->nodeNr==0){
    //Create and populate a new entry
    
  }else{
    //We should never get here, this is a malformed gstats file
    fprintf(stderr,"error: malformed gstats file \n");
    exit(EXIT_FAILURE);
  }
  return 0;
}



