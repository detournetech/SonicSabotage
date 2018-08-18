#ifndef APP_API_H
#define APP_API_H

#include "ble_api.h"

void cmd_scan_burst(const char *arg);
void cmd_connect_bond(const char *arg);
void cmd_buzz(const char *arg);
void cmd_rssistats(const char *arg);
void cmd_solarstats(const char *arg);
void cmd_randint(const char *arg);
void cmd_fixedint(const char *arg);
void cmd_solarmin(const char *arg); 
void cmd_rssimin(const char *arg);
void cmd_ota(const char *arg);

typedef const struct {
	const char *cmd;
	const char *arg;
	void (*func) (const char *arg);
	const char *desc;
	char * (*gen) (const char *text, int state);
	void (*disp) (char **matches, int num_matches, int max_length);
} cmd_table_entry;

const cmd_table_entry cmd_table[19];

void init_client(void);


#endif	/* APP_API_H */
