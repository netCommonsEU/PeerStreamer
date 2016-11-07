#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stddef.h>

#include "config-ALTO.h"

#define CONFIG_TYPE_INT    1
#define CONFIG_TYPE_FLOAT  2
#define CONFIG_TYPE_STRING 3

Config_t g_config;

typedef struct {
  void* entry;
  const char* string;
  unsigned int type;
  const char* default_value;
} ConfigEntry_t;

/* NOTE: Add additional entries here */
ConfigEntry_t config_entries[] = {
    { &g_config.neighborhood_target_size, "neighborhood_target_size", CONFIG_TYPE_INT, "15" },
    { &g_config.alto_server,        "alto_server",        CONFIG_TYPE_STRING, "" },
    { &g_config.alto_factor,        "alto_factor",        CONFIG_TYPE_FLOAT,  "0.8" },
    { &g_config.alto_pri_criterion, "alto_pri_criterion", CONFIG_TYPE_INT,    "1" },
    { &g_config.alto_sec_criterion, "alto_sec_criterion", CONFIG_TYPE_INT,    "0" },
    { &g_config.update_interval,    "update_interval",    CONFIG_TYPE_INT,    "60" },

    { NULL, "", 0 }	/* end marker -- should always be last */
};

int config_parse_entry(void *dst, const char *src, const char *string, int type) {
  int len = strlen(string);
  if (!strncmp(src, string, len) && isspace(src[len])) {
    /* match found */

    char* value = src+len+1;
    while (isspace(*value)) ++value; /* skip white spaces */

    if (type == CONFIG_TYPE_INT) *((int*)dst) = atoi(value);
    else if (type == CONFIG_TYPE_FLOAT) *((float*)dst) = atof(value);
    else if (type == CONFIG_TYPE_STRING) {
      if (*(char**)dst) free(*(char**)dst); /* free any already existing strings */
      *(char**)dst = strdup(value);
    }
    else return 0; /* other types not yet implemented */
    return 1;      /* correct match found */
  }
  return 0;        /* no match */
}

void config_init_entry(ConfigEntry_t* entry) {
  if (entry->type == CONFIG_TYPE_INT)         *((int*)entry->entry) = atoi(entry->default_value);
  else if (entry->type == CONFIG_TYPE_FLOAT)  *((float*)entry->entry) = atof(entry->default_value);
  else if (entry->type == CONFIG_TYPE_STRING) *(char**)(entry->entry) = strdup(entry->default_value);
}

void config_init() {
  int e = 0;
  while (config_entries[e].entry) {
    config_init_entry(&config_entries[e]);
    e++;
  }
}

int config_load(const char *filename) {
  char line[512];
  int len;
  FILE *f = NULL;
  int e = 0;

  f = fopen(filename, "rt");
  if (!f) {
    fprintf(stderr,"Unable to open config file.\n");
    return 0;
  }
  while (fgets(line, 512, f)) {
    char *p = line;
    while (isspace(*p)) ++p; /* skip white spaces */

    /* ignore comments and empty lines */
    if ( (*p=='#') || (*p=='\n') || (*p==0)) continue;

	/* remove newline */
	len = strlen(p);
    if (p[len-1]=='\n') p[len-1]='\0';

    while (config_entries[e].entry) {
      if (config_parse_entry(config_entries[e].entry, p, config_entries[e].string, config_entries[e].type)) break;
      e++;
    }
  }
  fclose(f);
  return 1;
}

void config_free() {
  int e = 0;
  while (config_entries[e].entry) {
    if (config_entries[e].type == CONFIG_TYPE_STRING) {
      free(*(char**)config_entries[e].entry);
	}
    config_entries[e].entry = NULL;
    config_entries[e].string = NULL;
    config_entries[e].type = 0;
    e++;
  }
}

void config_dump() {
  int e = 0;
  while (config_entries[e].entry) {
    fprintf(stderr,"%s ", config_entries[e].string);
    if (config_entries[e].type == CONFIG_TYPE_INT) fprintf(stderr,"%d", *(int*)config_entries[e].entry);
    else if (config_entries[e].type == CONFIG_TYPE_FLOAT) fprintf(stderr,"%f", *(float*)config_entries[e].entry);
    else if (config_entries[e].type == CONFIG_TYPE_STRING) fprintf(stderr,"%s", *(char**)config_entries[e].entry);
    fprintf(stderr,"\n");
    e++;
  }
}
