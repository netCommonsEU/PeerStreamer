/*
 *  Copyright (c) 2010 Luca Abeni
 *
 *  This is free software; see lgpl-2.1.txt
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "config-ml.h"

#define NAME_SIZE 32
#define VAL_SIZE 64

struct tag {
  char name[NAME_SIZE];
  char value[VAL_SIZE];
};

#define MAX_TAGS 16

struct tag *config_parse(const char *cfg)
{
  struct tag *res;
  int i = 0;
  const char *p = cfg;

  res = malloc(sizeof(struct tag) * MAX_TAGS);
  if (res == NULL) {
    return res;
  }
  while (p && *p != 0) {
    char *p1 = strchr(p, '=');
    
    memset(res[i].name, 0, NAME_SIZE);
    memset(res[i].value, 0, VAL_SIZE);
    if (p1) {
      if (i % MAX_TAGS == 0) {
        res = realloc(res, sizeof(struct tag) * (i + MAX_TAGS));
      }
      if (p1 - p > NAME_SIZE - 1) {
        fprintf(stderr, "Error, config name too long:%s\n", p);
        free(res);
        return NULL;
      }
      memcpy(res[i].name, p, p1 - p);
      p = strchr(p1, ',');
      if (p == NULL) {
        p = p1 + strlen(p1);
      }
      if (p - p1 > VAL_SIZE - 1) {
        fprintf(stderr, "Error, config value too long:%s\n", p1);
        free(res);
        return NULL;
      }
      memcpy(res[i++].value, p1 + 1, p - p1 - 1);
      if (*p) p++;
    } else {
      p = NULL;
    }
  }
  memset(res[i].name, 0, NAME_SIZE);
  memset(res[i].value, 0, VAL_SIZE);

  res = realloc(res, sizeof(struct tag) * (i + 1));

  return res;
}

const char *config_value_str(const struct tag *cfg_values, const char *value)
{
  int i, done;

  i = 0; done = 0;
  while (!done) {
    if (!strcmp(cfg_values[i].name, value)) {
      return cfg_values[i].value;
    }
    if (cfg_values[i].name[0] == 0) {
      done = 1;
    } else {
      i++;
    }
  }

  return NULL;
}

int config_value_int(const struct tag *cfg_values, const char *value, int *res)
{
  const char *str_res;

  str_res = config_value_str(cfg_values, value);
  if (str_res == NULL) {
    return 0;
  }

  *res = atoi(str_res);

  return 1;
}

int config_value_double(const struct tag *cfg_values, const char *value, double *res)
{
  const char *str_res;

  str_res = config_value_str(cfg_values, value);
  if (str_res == NULL) {
    return 0;
  }

  *res = strtod(str_res, NULL);

  return 1;
}

const char *config_value_str_default(const struct tag *cfg_values, const char *value, const char *default_value)
{
  const char *res;

  res = config_value_str(cfg_values, value);
  return res ? res : default_value;
}

int config_value_int_default(const struct tag *cfg_values, const char *value, int *res, int default_value)
{
  int r;

  r = config_value_int(cfg_values, value, res);
  if (!r) {
    *res = default_value;
  }
  return r;
}

int config_value_double_default(const struct tag *cfg_values, const char *value, double *res, double default_value)
{
  int r;

  r = config_value_double(cfg_values, value, res);
  if (!r) {
    *res = default_value;
  }
  return r;
}
