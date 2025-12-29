/**
 * @file config_validator.h
 * @brief JSON Schema validation for OSD configuration
 *
 * C-compatible interface for C++ JSON schema validator
 */

#ifndef CONFIG_VALIDATOR_H
#define CONFIG_VALIDATOR_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Validate OSD configuration JSON against schema
 *
 * @param config_path Path to JSON config file
 * @param schema_path Path to JSON schema file
 * @return true if valid, false if invalid
 *
 * On failure, prints detailed validation errors to stderr
 */
bool validate_config(const char *config_path, const char *schema_path);

/**
 * Validate OSD configuration JSON string against schema
 *
 * @param config_json JSON config string
 * @param schema_json JSON schema string
 * @return true if valid, false if invalid
 *
 * On failure, prints detailed validation errors to stderr
 */
bool validate_config_string(const char *config_json, const char *schema_json);

#ifdef __cplusplus
}
#endif

#endif // CONFIG_VALIDATOR_H
