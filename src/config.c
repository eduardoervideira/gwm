#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include "config.h"
#include "utils.h"

char *trim(char *str){
    if (str == NULL) return str;
    char *end;

    while(isspace((unsigned char)*str)){
        str++;
    }

    if (*str == '\0') return str;

    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)){
        end--;
    }

    *(end + 1) = '\0';
    return str;
}

void handle_bool(void *config_field, char *value){
    *(bool *)config_field = (!strcmp(value, "true")) ? true : false;
}

void handle_uint32(void *config_field, char *value) {
    *((uint32_t *)config_field) = (uint32_t)abs(atoi(value));
}

void handle_uint16(void *config_field, char *value) {
    *((uint16_t *)config_field) = (uint16_t)abs(atoi(value));
}

void handle_uint8(void *config_field, char *value) {
    *((uint8_t *)config_field) = (uint8_t)abs(atoi(value));
}

void handle_float(void *config_field, char *value) {
    *((float *)config_field) = (float)(atof(value));
}

void handle_rules(void *config_field, char *value) {
    /*char *rule_token = strtok(value, ",");
    if(rule_token == NULL)
        return;

    char *rule_value = strtok(NULL, "");
    if(rule_value == NULL)
        return;

    //parse_rule_values(gwm, rule_token, rule_value);*/
}

void handle_uint8_array(void *config_field, char *value){
    uint8_t **field = (uint8_t **)config_field;

    // check if first and last char are '[' and ']'
    if (value[0] != '[' || value[strlen(value) - 1] != ']') {
        fprintf(stderr, "workspace_layouts configuration value must be an array of workspace layouts\n");
        exit(EXIT_FAILURE);
    }

    // copy the string to a new string without the first and last char
    char *value_copy = malloc(sizeof(char) * strlen(value));
    if (value_copy == NULL) {
        fprintf(stderr, "error allocating memory to handle array in config\n");
        exit(EXIT_FAILURE);
    }
    strncpy(value_copy, value + 1, strlen(value) - 2);
    value_copy[strlen(value) - 2] = '\0';

    // count how many commas are in the array (always 1 less than the number of elements)
    uint8_t array_length = 0;
    while (*value_copy) {
        if (*value_copy == ',')
            array_length++;

        value_copy++;
    }

    // reset pointer to the start of the string
    value_copy -= strlen(value) - 2;

    // allocate memory for the array
    *field = (uint8_t *)calloc(array_length + 1, sizeof(uint8_t));
    if (*field == NULL) {
        fprintf(stderr, "error allocating memory to handle array in config\n");
        exit(EXIT_FAILURE);
    }

    // split the string by commas and convert each element to uint8_t
    uint8_t i = 0;
    char *element = strtok(value_copy, ",");
    while (element != NULL) {
        (*field)[i] = (uint8_t)abs(atoi(element));
        i++;
        element = strtok(NULL, ",");
    }

    free(value_copy);
}

int create_config_dir(char *path){
    struct stat st;
    if(stat(path, &st) == -1){
        if(errno == ENOENT){
            if(mkdir(path, 0700) != 0){
                fprintf(stderr, "error creating config directory: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
        } else {
            fprintf(stderr, "error checking the configuration directory: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}

/* TODO: problema desta configuração: todas as workspaces que usarem stacking mode
vão ficar com um master window ratio = ao que tiver na config, mesmo que não seja o desejado.
seria MELHOR cada workspace ter o seu config e haver outra config para o gwm em geral */

// TODO: após esta funçao é necessário dump_config_file para guardar as configurações
int set_default_config_for_undefined_values(Config *config){
    config->workspace_count = config->workspace_count != 0 ? config->workspace_count : 3;
    config->workspace_layouts = config->workspace_layouts != NULL ? config->workspace_layouts : (uint8_t *)calloc(config->workspace_count, sizeof(uint8_t));
    config->max_clients = config->max_clients != 0 ? config->max_clients : 5;

    config->focus_follows_mouse = config->focus_follows_mouse != 0 ? config->focus_follows_mouse : false;
    config->mod1_key = config->mod1_key != 0 ? config->mod1_key : XCB_MOD_MASK_1;
    config->mod4_key = config->mod4_key != 0 ? config->mod4_key : XCB_MOD_MASK_4;

    config->clients_gap = config->clients_gap != 0 ? config->clients_gap : 5;
    //config->border_width = config->border_width != 0 ? config->border_width : 2;

    config->border_color = config->border_color != 0 ? config->border_color : 0xB6F474;
    config->focused_border_color = config->focused_border_color != 0 ? config->focused_border_color : 0xe9f3de;
    config->unfocused_border_color = config->unfocused_border_color != 0 ? config->unfocused_border_color : 0xabcdef;

    config->margin_left = config->margin_left != 0 ? config->margin_left : 0;
    config->margin_top = config->margin_top != 0 ? config->margin_top : 0;
    config->margin_right = config->margin_right != 0 ? config->margin_right : 0;
    config->margin_bottom = config->margin_bottom != 0 ? config->margin_bottom : 0;

    config->min_window_width = config->min_window_width != 0 ? config->min_window_width : 50;
    config->min_window_height = config->min_window_height != 0 ? config->min_window_height : 50;

    config->stacking_mode_master_window_ratio = config->stacking_mode_master_window_ratio != 0 ? config->stacking_mode_master_window_ratio : 0.5;
    config->stacking_mode_master_window_position = config->stacking_mode_master_window_position != 0 ? config->stacking_mode_master_window_position : 0;
    config->stacking_mode_master_window_orientation = config->stacking_mode_master_window_orientation != 0 ? config->stacking_mode_master_window_orientation : 0;

    config->n_columns_mode_number_of_columns = config->n_columns_mode_number_of_columns != 0 ? config->n_columns_mode_number_of_columns : 2;
    config->n_rows_mode_number_of_rows = config->n_rows_mode_number_of_rows != 0 ? config->n_rows_mode_number_of_rows : 2;
    return 0;
}

// TODO: implement a better rules parser - stop using strcpy()
void parse_rule_values(WindowManager *gwm, char *rule_token, char *rule_value){
    int num_rules = gwm->num_rules;
    char *pair = strtok(rule_value, ",");
    
    while (pair != NULL) {
        char key[100];
        char value[100];
        
        sscanf(pair, "%[^'^']^%[^'^']", key, value);
        
        strcpy(gwm->rules[num_rules].name, rule_token);
        strcpy(gwm->rules[num_rules].key, key);
        strcpy(gwm->rules[num_rules].value, value);
        num_rules++;
        
        pair = strtok(NULL, ",");
    }

    gwm->num_rules = num_rules;
}

// TODO: implement a better config parser - stop using gets()
int parse_config_file(ConfigMap *config_handlers, const char *path){
    printf("parsing config file: %s\n", path);
    FILE *config_file = fopen(path, "r");
    if(config_file == NULL){
        fprintf(stderr, "error opening configuration file: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    char line[256];
    char *equals_sign;
    char *key = NULL;
    char *value = NULL;
    while(fgets(line, sizeof(line), config_file) != NULL){
        if(line[0] == '/' || line[0] == '#' || line[0] == '\n')
            continue;

        equals_sign = strchr(line, '=');
        if (equals_sign) {
            key = line;
            value = equals_sign + 1;
            *equals_sign = '\0';

            for (size_t i = 0; config_handlers[i].key != NULL; i++) {
                if(config_handlers[i].key == NULL){
                    //fprintf(stderr, "unknown configuration key: %s\n", key);
                    break;
                }

                if (!strcmp(config_handlers[i].key, trim(key))) {
                    config_handlers[i].handler(config_handlers[i].field, trim(value));
                    break;
                }
            }
        }
    }

    fclose(config_file);
    return 0;
}

int load_configuration(Config *config){
    const char *xdg_config_home = getenv("XDG_CONFIG_HOME");
    const char *home = getenv("HOME");
    char gwm_config_dir_path[512];
    char gwm_config_file_path[512];

    if(xdg_config_home != NULL){
        snprintf(gwm_config_dir_path, sizeof(gwm_config_dir_path), "%s/gwm", xdg_config_home);
        snprintf(gwm_config_file_path, sizeof(gwm_config_file_path), "%s/gwm/gwmrc", xdg_config_home);
    } else if(home != NULL){
        snprintf(gwm_config_dir_path, sizeof(gwm_config_dir_path), "%s/.config/gwm", home);
        snprintf(gwm_config_file_path, sizeof(gwm_config_file_path), "%s/.config/gwm/gwmrc", home);
    } else {
        fprintf(stderr, "failed loading configuration - XDG_CONFIG_HOME or HOME environment variable not defined\n");
        exit(EXIT_FAILURE);
    }

    // TODO: check return
    create_config_dir(gwm_config_dir_path);

    if(access(gwm_config_file_path, F_OK) == -1){
        fprintf(stderr, "configuration file not found. create a configuration file at %s\n", gwm_config_file_path);
        return -1;
    }

    ConfigMap config_handlers[] = {
        { "workspace_count", handle_uint8, &config->workspace_count },
        { "workspace_layouts", handle_uint8_array, &config->workspace_layouts },
        { "max_clients", handle_uint8, &config->max_clients},
        { "focus_follows_mouse", handle_bool, &config->focus_follows_mouse },
        { "clients_gap", handle_uint16, &config->clients_gap },
        { "border_width", handle_uint16, &config->border_width },
        { "border_color", handle_uint32, &config->border_color },
        { "focused_border_color", handle_uint32, &config->focused_border_color },
        { "unfocused_border_color", handle_uint32, &config->unfocused_border_color },
        //{ "margin", handle_uint8_array, &config->margin_left },
        { "margin_left", handle_uint16, &config->margin_left },
        { "margin_top", handle_uint16, &config->margin_top },
        { "margin_right", handle_uint16, &config->margin_right },
        { "margin_bottom", handle_uint16, &config->margin_bottom },
        { "min_window_width", handle_uint16, &config->min_window_width },
        { "min_window_height", handle_uint16, &config->min_window_height },
        /*{ "mod1_key", handle_int },
        { "mod4_key", handle_int },*/
        { "stacking_mode_master_window_ratio", handle_float, &config->stacking_mode_master_window_ratio },
        { "stacking_mode_master_window_position", handle_uint8, &config->stacking_mode_master_window_position },
        { "stacking_mode_master_window_orientation", handle_uint8, &config->stacking_mode_master_window_orientation },
        { "n_columns_mode_number_of_columns", handle_uint8, &config->n_columns_mode_number_of_columns },
        { "n_rows_mode_number_of_rows", handle_uint8, &config->n_rows_mode_number_of_rows },
        //{ "client_rule", handle_rules, &config->rules},
        { NULL, NULL, NULL}
    };

    parse_config_file(config_handlers, gwm_config_file_path);
    //set_default_config_for_undefined_values(config); // TODO: rever este codigo

    printf("workspace_count: %d\n", config->workspace_count);
    printf("workspace_layouts: ");
    for(int i = 0; i < config->workspace_count; i++){
        printf("%d ", config->workspace_layouts[i]);
    }
    printf("\n");
    printf("max_clients: %d\n", config->max_clients);
    printf("focus_follows_mouse: %d\n", config->focus_follows_mouse);
    printf("clients_gap: %d\n", config->clients_gap);
    printf("border_width: %d\n", config->border_width);
    printf("border_color: %d\n", config->border_color);
    printf("focused_border_color: %d\n", config->focused_border_color);
    printf("unfocused_border_color: %d\n", config->unfocused_border_color);
    printf("margin_left: %d\n", config->margin_left);
    printf("margin_top: %d\n", config->margin_top);
    printf("margin_right: %d\n", config->margin_right);
    printf("margin_bottom: %d\n", config->margin_bottom);
    printf("min_window_width: %d\n", config->min_window_width);
    printf("min_window_height: %d\n", config->min_window_height);
    printf("mod1_key: %d\n", config->mod1_key);
    printf("mod4_key: %d\n", config->mod4_key);
    printf("stacking_mode_master_window_ratio: %f\n", config->stacking_mode_master_window_ratio);
    printf("stacking_mode_master_window_position: %d\n", config->stacking_mode_master_window_position);
    printf("stacking_mode_master_window_orientation: %d\n", config->stacking_mode_master_window_orientation);
    printf("n_columns_mode_number_of_columns: %d\n", config->n_columns_mode_number_of_columns);
    printf("n_rows_mode_number_of_rows: %d\n", config->n_rows_mode_number_of_rows);
    return 0;
}