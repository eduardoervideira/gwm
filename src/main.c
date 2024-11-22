#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "window_manager.h"
#include "events.h"

int main(int argc, char** argv){
    if(argc <= 1){
        printf("[info]: usage: gwm [start]\n");
        exit(EXIT_FAILURE);
    }

    if(!strcmp(argv[1], "start")){
        WindowManager *gwm = setup();

        if(!gwm){
            printf("[fatal]: failed to initialize gwm\n");
            exit(EXIT_FAILURE);
        }

        handle_events(gwm);
        cleanup(gwm);
    }

    return 0;
}