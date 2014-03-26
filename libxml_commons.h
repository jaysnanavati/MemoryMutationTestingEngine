#include <libxml/parser.h>
#include <libxml/xpath.h>

xmlDocPtr getdoc (char *docname);
xmlXPathObjectPtr getnodeset (xmlDocPtr doc, xmlChar *xpath);
