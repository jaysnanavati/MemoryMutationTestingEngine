#include "libxml_commons.h"

xmlDocPtr
getdoc (char *docname) {
  xmlDocPtr doc;
  doc = xmlParseFile(docname);
  return doc;
}

xmlXPathObjectPtr
getnodeset (xmlDocPtr doc, xmlChar *xpath){
  
  xmlXPathContextPtr context;
  xmlXPathObjectPtr result;
  
  context = xmlXPathNewContext(doc);
  if (context == NULL) {
    return NULL;
  }
  result = xmlXPathEvalExpression(xpath, context);
  xmlXPathFreeContext(context);
  if (result == NULL) {
    return NULL;
  }
  if(xmlXPathNodeSetIsEmpty(result->nodesetval)){
    xmlXPathFreeObject(result);
    return NULL;
  }
  return result;
}