#ifndef GRAPES_CONFIG_H
#define GRAPES_CONFIG_H
struct tag;

/*
 * earlier name-value pairs in the config string take precedence
 */

struct tag *config_parse(const char *cfg);
int config_value_int(const struct tag *cfg_values, const char *value, int *res);
int config_value_int_default(const struct tag *cfg_values, const char *value, int *res, int default_value);
int config_value_double(const struct tag *cfg_values, const char *value, double *res);
int config_value_double_default(const struct tag *cfg_values, const char *value, double *res, double default_value);
const char *config_value_str(const struct tag *cfg_values, const char *value);
const char *config_value_str_default(const struct tag *cfg_values, const char *value, const char *default_value);

#endif /* CONFIG_H */
