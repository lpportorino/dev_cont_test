#include "config_validator.h"
#include <stdio.h>

int main(void) {
    const char *config_path = "../../resources/test_invalid.json";
    const char *schema_path = "../../resources/schemas/osd_config.schema.json";
    
    printf("Testing JSON validation with invalid config...\n\n");
    
    bool valid = validate_config(config_path, schema_path);
    
    if (valid) {
        printf("\nResult: Config is VALID\n");
        return 0;
    } else {
        printf("\nResult: Config is INVALID (as expected)\n");
        return 1;
    }
}
