#include "config_validator.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <config.json> <schema.json>\n", argv[0]);
        return 1;
    }
    
    const char *config_path = argv[1];
    const char *schema_path = argv[2];
    
    printf("Testing JSON validation...\n");
    printf("Config: %s\n", config_path);
    printf("Schema: %s\n\n", schema_path);
    
    bool valid = validate_config(config_path, schema_path);
    
    return valid ? 0 : 1;
}
