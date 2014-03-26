typedef struct ValgrindError ValgrindError;

struct ValgrindError {
  ValgrindError* siblings;
  int occurrences;
  char* kind;
  char* what;
  char* auxwhat;
  char* location;
};

typedef struct {
  ValgrindError *valgrindErrors;
  int valgrind_error_count;
  int unique_valgrind_error_count;
} ValgrindResult;

ValgrindResult* valgrindXML_evaluate(char *xml_path,int testing_framework);