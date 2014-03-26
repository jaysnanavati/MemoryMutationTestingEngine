#define GSTATS_PATH "gstats.stats"

void open_GStats();
void close_GStats();
int clear_GStats();
int create_update_GStats(char*mut_code,char*key,char*value);