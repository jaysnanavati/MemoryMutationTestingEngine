#define GSTATS_PATH "gstats.xml"

void open_GStats();
void close_GStats();
void flush_GStats();
int clear_GStats();
void create_update_gstat(char*mut_code,char*key,int value);
int get_gstat_value(char*mut_code,char*key);