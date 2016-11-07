#ifndef NAPA_CONFIG_H
#define NAPA_CONFIG_H

typedef struct {
  unsigned int neighborhood_target_size;
  char* alto_server;
  float alto_factor;
  unsigned int alto_pri_criterion;
  unsigned int alto_sec_criterion;
  unsigned int update_interval;
} Config_t;

extern Config_t g_config;

void config_init();
int  config_load(const char *filename);
void config_dump();
void config_free();

#endif
