#define GSTATS_PATH "gstats.xml"

void open_GStats();
void close_GStats();
void flush_GStats();
int clear_GStats();
void create_update_gstat_mutation(char*mut_code,char*key,int value);
int get_gstat_value_mutation(char*mut_code,char*key);

int get_gstat_value_aggr_results(char*key);
void create_update_aggr_results(char*key,int value);

void generate_derived_stats();