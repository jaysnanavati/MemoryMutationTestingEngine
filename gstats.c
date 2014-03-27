#include <stdlib.h>
#include <string.h>

#include "gstats.h"
#include "libxml_commons.h"

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

int create_update_GStats(char*mut_code,char*key,char*value){
  //Check to see if an entry for the mut_code exists, else create and fill with children key,value
  xmlNodeSetPtr nodeset;
  xmlXPathObjectPtr result;
  
  char *xPath_prefix = "//mutation_operator[code='";
  char *xPath_suffix = "']";
  char xPath_query[strlen(xPath_prefix)+strlen(mut_code)+strlen(xPath_suffix)+1];
  
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



