#include <inttypes.h>
#include <string.h>

#include "valgrindEval.h"
#include "libxml_commons.h"

void evaluate_error_nodes(ValgrindError*valgrindError,xmlDocPtr doc,int error_count,xmlChar *unique_value,int testing_framework){
  xmlNodeSetPtr nodeset;
  char*xPath_prefix = "//error[unique='";
  char*xPath_suffix = "']";
  char xPath_query[strlen(xPath_prefix)+strlen((char*)unique_value)+strlen(xPath_suffix)+1];
  xmlXPathObjectPtr result;
  int i;
  
  //Initialization of valgrinderror
  valgrindError->siblings=NULL;
  valgrindError->occurrences=0;
  valgrindError->kind=NULL;
  valgrindError->what=NULL;
  valgrindError->auxwhat=NULL;
  valgrindError->location=NULL;
  
  sprintf(xPath_query, "%s%s%s",xPath_prefix , (char*)unique_value,xPath_suffix);
  xmlChar *xpath = (xmlChar*) xPath_query;
  result = getnodeset (doc, xpath);
  
  valgrindError->occurrences=error_count;
  if(error_count>1){
    valgrindError->siblings = malloc(error_count*sizeof(ValgrindError));
  }
  
  //We have a guarentee of getting a non-empty nodeset due to the format of the valgrind output
  nodeset = result->nodesetval;
  //Itterate through the <error><error> nodes that have the provided <unique></unique> value
  ValgrindError* current_error=valgrindError;
  for (i=0; i < nodeset->nodeNr; i++) {
    if(i>0){
      current_error = &valgrindError->siblings[i-1];
      current_error->siblings=NULL;
      current_error->occurrences=1;
      current_error->kind=NULL;
      current_error->auxwhat=NULL;
      current_error->location=NULL;
    }
    xmlNodePtr errorNode = nodeset->nodeTab[i];
    xmlNodePtr currentNode = errorNode->xmlChildrenNode;
    
    while(currentNode!=NULL){
      //Get Kind if present
      if ((!xmlStrcmp(currentNode->name, (const xmlChar *)"kind"))){
	xmlChar *kind_value = xmlNodeListGetString(doc, currentNode->xmlChildrenNode, 1);
	current_error->kind=(char*)kind_value;
	currentNode=xmlNextElementSibling(currentNode);  
      }else if((!xmlStrcmp(currentNode->name, (const xmlChar *)"what"))){
	xmlChar *what_value = xmlNodeListGetString(doc, currentNode->xmlChildrenNode, 1);
	current_error->what=(char*)what_value;
	currentNode=xmlNextElementSibling(currentNode);  
      }else if((!xmlStrcmp(currentNode->name, (const xmlChar *)"stack"))){
	int max_location_size = 128;
	current_error->location=calloc(sizeof(char),max_location_size);
	//Iterate each frame in the stack
	xmlNodePtr frame_node = currentNode->xmlChildrenNode;
	
	int reached_stack_depth=0;
	
	while(frame_node!=NULL){
	  if(xmlStrcmp(frame_node->name, (const xmlChar *)"frame")!=0){
	    frame_node = xmlNextElementSibling(frame_node);
	  }else{
	    xmlNodePtr frame_child = frame_node->xmlChildrenNode;
	    //get the <fn></fn> element
	    while(frame_child!=NULL){
	      if ((!xmlStrcmp(frame_child->name, (const xmlChar *)"fn"))){
		xmlChar *fn_value = xmlNodeListGetString(doc, frame_child->xmlChildrenNode, 1);
		
		if(testing_framework==1){
		  if(strncmp((char*)fn_value,"CuTestRun",strlen("CuTestRun"))==0){
		    reached_stack_depth=1;
		    break;
		  }
		}
		
		int total_len = (strlen(current_error->location)+strlen(" :: ")+strlen((char*)fn_value))+1;
		if(total_len>max_location_size){
		  //Realloc
		  current_error->location = realloc(current_error->location,(max_location_size*2)+total_len);
		}else{
		  if(strlen(current_error->location)==0){
		     strcat(current_error->location,(char*)fn_value);
		  }else{
		    strcat(current_error->location," :: ");
		    strcat(current_error->location,(char*)fn_value);
		  }
		}
	      }
	      frame_child=xmlNextElementSibling(frame_child);
	    }
	    if(reached_stack_depth){
	      break;
	    }
	    frame_node=xmlNextElementSibling(frame_node);
	  }
	}
	//Get the auxwhat if it exists
	currentNode=xmlNextElementSibling(currentNode);
	//CHeck if we have an auxwhat and either way go to the next error
	if(currentNode!=NULL &&(!xmlStrcmp(currentNode->name, (const xmlChar *)"auxwhat"))){
	  xmlChar *auxwhat_value = xmlNodeListGetString(doc, currentNode->xmlChildrenNode, 1);
	  current_error->auxwhat=(char*)auxwhat_value;
	}
	break;
      }else{
	currentNode=xmlNextElementSibling(currentNode);  
      }
    }
  }
  
}

ValgrindResult* valgrindXML_evaluate(char *xml_path,int testing_framework) {
  char *docname;
  xmlDocPtr doc;
  xmlChar *xpath = (xmlChar*) "//errorcounts/pair/count";
  xmlNodeSetPtr nodeset;
  xmlXPathObjectPtr result;
  int i;
  docname = xml_path;
  doc = getdoc(docname);
  result = getnodeset (doc, xpath);
  
  ValgrindResult *valgrindResult=NULL;
  //Get all the errors first
  if (result) {
    nodeset = result->nodesetval;
    if(nodeset->nodeNr==0){
      //NO valgrind errors for this mutant
      return NULL;
    }
    valgrindResult= malloc(sizeof(ValgrindResult));
    //Initialization of valgrindResult
    valgrindResult->valgrind_error_count=0;
    valgrindResult->unique_valgrind_error_count=0;
    
    //Capturing results in valgrindResult
    valgrindResult->unique_valgrind_error_count=nodeset->nodeNr;
    valgrindResult->valgrindErrors = malloc(nodeset->nodeNr*sizeof(ValgrindError));
    
    for (i=0; i < nodeset->nodeNr; i++) {
      //Get <count></count>
      xmlNodePtr countNode = nodeset->nodeTab[i];
      //Get <unique></unique>
      xmlNodePtr uniqueNode = xmlNextElementSibling(countNode);
      
      //Values for both of the nodes
      xmlChar *count_value = xmlNodeListGetString(doc, countNode->xmlChildrenNode, 1);
      xmlChar *unique_value = xmlNodeListGetString(doc, uniqueNode->xmlChildrenNode, 1);
      
      int error_count = (int)strtoumax((char*)count_value, NULL, 10);
      valgrindResult->valgrind_error_count+=error_count;
      
      //For each unique value, get all its error nodes
      evaluate_error_nodes(&valgrindResult->valgrindErrors[i],doc,error_count,unique_value,testing_framework);
      
      xmlFree(count_value);
      xmlFree(unique_value);
    }
    xmlXPathFreeObject (result);
  }
  xmlFreeDoc(doc);
  xmlCleanupParser();
  
  return valgrindResult;
}
