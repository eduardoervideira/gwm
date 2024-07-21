#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "window_manager.h"
#include "events.h"
#include "utils.h"

int main(int argc, char** argv){
    if(argc <= 1){
        log_message(LOG_INFO, "usage: gwm [start]\n");
        exit(EXIT_FAILURE);
    }

    if(!strcmp(argv[1], "start")){
        init_log();
        WindowManager *gwm = setup();

        if(!gwm){
            log_message(LOG_FATAL, "failed to initialize gwm\n");
            exit(EXIT_FAILURE);
        }

        handle_events(gwm);
        cleanup(gwm);
        free(gwm);
        close_log();
    }

    return 0;
}