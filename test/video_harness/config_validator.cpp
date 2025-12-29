/**
 * @file config_validator.cpp
 * @brief JSON Schema validation implementation using nlohmann/json + pboettch/json-schema-validator
 */

#include "config_validator.h"

#include <fstream>
#include <iostream>
#include <sstream>

// nlohmann/json v3.12.0 - JSON for Modern C++
#include <nlohmann/json.hpp>

// pboettch/json-schema-validator v2.3.0 - JSON Schema Draft-07 validator
#include <nlohmann/json-schema.hpp>

using json = nlohmann::json;
using json_validator = nlohmann::json_schema::json_validator;

/**
 * Custom error handler for validation errors
 */
class validation_error_handler : public nlohmann::json_schema::basic_error_handler
{
private:
  std::vector<std::string> errors;

public:
  void error(const nlohmann::json::json_pointer &pointer,
             const json &instance,
             const std::string &message) override
  {
    (void)instance; // Required by interface but unused
    std::ostringstream oss;
    oss << "  ❌ " << pointer.to_string() << ": " << message;
    errors.push_back(oss.str());
  }

  const std::vector<std::string> &get_errors() const { return errors; }
  bool has_errors() const { return !errors.empty(); }
};

/**
 * Read file contents into string
 */
static std::string read_file(const char *path)
{
  std::ifstream file(path);
  if (!file.is_open())
    {
      std::cerr << "[VALIDATOR] Failed to open file: " << path << std::endl;
      return "";
    }

  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

extern "C" bool
validate_config(const char *config_path, const char *schema_path)
{
  if (!config_path || !schema_path)
    {
      std::cerr << "[VALIDATOR] ERROR: Null path provided" << std::endl;
      return false;
    }

  std::cout << "[VALIDATOR] Validating config: " << config_path << std::endl;
  std::cout << "[VALIDATOR] Using schema: " << schema_path << std::endl;

  // Read files
  std::string config_str = read_file(config_path);
  std::string schema_str = read_file(schema_path);

  if (config_str.empty())
    {
      std::cerr << "[VALIDATOR] ERROR: Failed to read config file" << std::endl;
      return false;
    }

  if (schema_str.empty())
    {
      std::cerr << "[VALIDATOR] ERROR: Failed to read schema file" << std::endl;
      return false;
    }

  return validate_config_string(config_str.c_str(), schema_str.c_str());
}

extern "C" bool
validate_config_string(const char *config_json, const char *schema_json)
{
  if (!config_json || !schema_json)
    {
      std::cerr << "[VALIDATOR] ERROR: Null JSON string provided" << std::endl;
      return false;
    }

  try
    {
      // Parse JSON
      json config = json::parse(config_json);
      json schema = json::parse(schema_json);

      // Create validator
      json_validator validator;

      // Set schema
      try
        {
          validator.set_root_schema(schema);
        }
      catch (const std::exception &e)
        {
          std::cerr << "[VALIDATOR] ERROR: Invalid schema: " << e.what() << std::endl;
          return false;
        }

      // Create error handler
      validation_error_handler error_handler;

      // Validate
      validator.validate(config, error_handler);

      if (error_handler.has_errors())
        {
          std::cerr << "\n[VALIDATOR] ❌ Configuration validation FAILED:\n" << std::endl;
          for (const auto &error : error_handler.get_errors())
            {
              std::cerr << error << std::endl;
            }
          std::cerr << std::endl;
          return false;
        }

      std::cout << "[VALIDATOR] ✅ Configuration is valid" << std::endl;
      return true;
    }
  catch (const json::parse_error &e)
    {
      std::cerr << "[VALIDATOR] ERROR: JSON parse error: " << e.what() << std::endl;
      return false;
    }
  catch (const std::exception &e)
    {
      std::cerr << "[VALIDATOR] ERROR: Validation error: " << e.what() << std::endl;
      return false;
    }
}
