
void open_GStats(char* GSTATS_PATH);
void close_GStats(char* GSTATS_PATH);
void flush_GStats(char* GSTATS_PATH);
int clear_GStats(char* GSTATS_PATH);

void create_update_gstat_mutation(char*mut_code,char*key,int value);
int get_gstat_value_mutation(char*mut_code,char*key);

int get_gstat_value_aggr_results(char*key);
void create_update_aggr_results(char*key,int value);

void generate_derived_stats();