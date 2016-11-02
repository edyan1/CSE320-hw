#include "sfish.h"

/* BUILT-IN FUNCTIONS PSEUDOCODE */
/* input = readline()
if (builtin_func = get_builtin(input))
    fork()
    if child:
        builtin_func()
    else:
        wait_for_child() */

const char *built_in[6] = {"help", "cd", "pwd", "prt", "chpmt", "chclr"};
static char* lineIn;
static int status = 0;
static char* user;
static char* home;
static char* hostName;
static char* dir;


/* 
promptFlag:
3 = user and machine displayed
2 = machine displayed, user not displayed
1 = user displayed, machine not displayed
0 = neither field displayed
*/
static int promptFlag = 3;
static int userColor = 37;
static int machineColor = 37;
static int userBold = 0;
static int machineBold = 0;

int main(int argc, char** argv) {
    //DO NOT MODIFY THIS. If you do you will get a ZERO.
    rl_catch_signals = 0;
    //This is disable readline's default signal handlers, since you are going
    //to install your own.

    char *cmd;
    char* cwd = malloc(50);

    int gotHost;
    hostName = malloc(20);

    
    if ((gotHost=gethostname(hostName, 20)) == -1) hostName = "{unspecified machine}";

    
    user = getenv("USER");
    home = getenv("HOME");
    getcwd(cwd, 50);
    dir = NULL; 

    /*if home and current working directly match, then set ~
      otherwise set dir as current working directory but skipping the home string */
    if (strcmp(home, cwd) == 0) dir = "~";
    else if (strncmp(home, cwd, strlen(home)) == 0) dir = &cwd[strlen(home)];

    lineIn = malloc(100);
    setPrompt();

    printf("\e[31mHello\e[32m, \e[34mBlue \e[32mWorld\e[0m.\n");

    while((cmd = readline(lineIn)) != NULL) {
        if (strcmp(cmd,"quit")==0)
            break;

        if (strcmp(cmd, "exit")==0)
            break;

        //All your debug print statments should be surrounded by this #ifdef
        //block. Use the debug target in the makefile to run with these enabled.
        #ifdef DEBUG
        fprintf(stderr, "Length of command entered: %ld\n", strlen(cmd));
        #endif
        //You WILL lose points if your shell prints out garbage values.

        get_builtin(cmd);

        

    }

    //Don't forget to free allocated memory, and close file descriptors.
    free(cwd);
    free(cmd);
    free(hostName);
    free(lineIn);
    //WE WILL CHECK VALGRIND!

    return EXIT_SUCCESS;
}

void get_builtin(char *cmd) {
    pid_t child_id;
    
    int i;
    
    char* input = cmd;

    if(strlen(cmd) > 4){
        input = strtok(cmd, " ");
    }

    for (i = 0; i < 6; i++){
        if (strcmp(input, built_in[i])==0) 
            break;
    }
    switch(i){
        case 0: /* help */
            if ((child_id=fork())==0) sfish_help();
            else wait(&status);
            break;
        case 1: /* cd */
            sfish_cd();
            break;
        case 2: /* pwd */
            if ((child_id=fork())==0) sfish_pwd();
            else wait(&status);
            break;
        case 3: /* prt */
            if ((child_id=fork())==0) sfish_prt(status);
            else wait(&status);
            break;
        case 4:
            sfish_chpmt(cmd);
           
            break;
        case 5:
            sfish_chclr(cmd);
            
            break;
        default: return;
    }
    
}

void sfish_help(){
    printf("%s", help);
    exit(EXIT_SUCCESS);
}

void sfish_cd(){

}

void sfish_pwd(){
    char* buf = malloc(100);
    getcwd(buf, 100);
    printf("%s\n", buf);
    free(buf);
    exit(EXIT_SUCCESS);
}

void sfish_prt(int status){
    printf("%d\n", status);
    exit(EXIT_SUCCESS);

}

void sfish_chpmt(char *cmd){
    
    char* arg1;
    char* arg2;
    if ((arg1 = strtok(NULL, " ")) != NULL) {
        if((arg2 = strtok(NULL, " ")) != NULL);
        else {
            fprintf(stderr, "Missing argument for chpmt command.\n");
            return;
        }
    }
    else {
        fprintf(stderr, "Missing argument for chpmt command.\n");
        return;
    }

    #ifdef DEBUG
    printf("%s\t%s\n",arg1, arg2);
    #endif

    //analyze arguments;
    //using strcmp, so if int = 0, then flag is true
    int userFlag = strcmp(arg1, "user"); 
    int machine = strcmp(arg1, "machine");
    int enable = strcmp(arg2, "1");
    int disable = strcmp(arg2, "0");

    if (userFlag!= 0 && machine!= 0){
        fprintf(stderr, "Invalid first argument for chpmt\n");
        exit(EXIT_FAILURE);
    }

    if (enable!= 0 && disable!= 0){
        fprintf(stderr, "Invalid second argument for chpmt\n");
        exit(EXIT_FAILURE);
    }

    /* 
    promptFlag:
    3 = user and machine displayed
    2 = user not displayed, machine displayed
    1 = user displayed, machine not displayed
    0 = neither field displayed
    */

    if (userFlag==0 && disable==0){
        if (promptFlag == 3 || promptFlag == 1) promptFlag--;
    }

    else if (userFlag==0 && enable==0){
        if (promptFlag == 0 || promptFlag == 2) promptFlag++;
    }

    else if (machine==0 && disable==0){
        if (promptFlag == 3 || promptFlag == 2) promptFlag-=2;
    }

    else if (machine==0 && enable==0){
        if (promptFlag == 0 || promptFlag == 1) promptFlag+=2;
    }

    else exit(EXIT_FAILURE);
    printf("promptFlag: %d\n", promptFlag);
    setPrompt();
}

void sfish_chclr(char *cmd){

    char* arg1;
    char* arg2;
    char* arg3;
    if ((arg1 = strtok(NULL, " ")) != NULL){ 
        if((arg2 = strtok(NULL, " ")) != NULL) {
            if((arg3 = strtok(NULL, " ")) != NULL);
            else {
                fprintf(stderr, "Missing arguments for chclr command.\n");
                return;
            } 
        }
        else {
            fprintf(stderr, "Missing arguments for chclr command.\n");
            return;  
        }
    }
    else {
        fprintf(stderr, "Missing arguments for chclr command.\n");
        return;
    }
    #ifdef DEBUG
    printf("%s\t%s\t%s\n",arg1, arg2, arg3);
    #endif

    //analyze arguments;
    //using strcmp, so if int = 0, then flag is true
    

    int userFlag = strcmp(arg1, "user"); 
    int machine = strcmp(arg1, "machine");
    int color = 0;
    int enableBold = strcmp(arg3, "1");
    int disableBold = strcmp(arg3, "0");

    if (userFlag!= 0 && machine!= 0){
        fprintf(stderr, "Invalid first argument for chclr\n");
        return;
    }

    //analyze the color argument
    //30 black
    //31 red
    //32 green
    //33 yellow
    //34 blue
    //35 magenta
    //36 cyan
    //37 white
    if (strcmp(arg2, "black")==0) color = 30;
    else if (strcmp(arg2, "red")==0) color = 31;
    else if (strcmp(arg2, "green")==0) color = 32;
    else if (strcmp(arg2, "yellow")==0) color = 33;
    else if (strcmp(arg2, "blue")==0) color = 34;
    else if (strcmp(arg2, "magenta")==0) color = 35;
    else if (strcmp(arg2, "cyan")==0) color = 36;
    else if (strcmp(arg2, "white")==0) color = 37;
    else { 
        fprintf(stderr, "Invalid second argument (color) for chclr\n");
        return;
    }

    if (enableBold!= 0 && disableBold!= 0){
        fprintf(stderr, "Invalid third argument (boldness) for chclr\n");
        return;
    }
    else if (enableBold==0 && userFlag==0) userBold = 1; 
    else if (enableBold==0 && machine==0) machineBold = 1;
    else if (disableBold==0 && userFlag==0) userBold = 0;
    else if (disableBold==0 && machine==0) machineBold = 0;

    if (color > 0){
        if(userFlag==0) userColor = color;
        else if (machine==0) machineColor = color;
    }
    setPrompt();
}

void setPrompt(){

    //printf("\e[31mHello\e[32m, \e[34mBlue \e[32mWorld\e[0m.\n");

    char* colorUser; //set user color, checking for boldness

    if(userBold) {
        switch(userColor){
            case 30: 
                colorUser = "\e[1;30m"; 
                break;
            case 31: 
                colorUser = "\e[1;31m";
                break;
            case 32: 
                colorUser = "\e[1;32m";
                break;
            case 33: 
                colorUser = "\e[1;33m";
                break;
            case 34: 
                colorUser = "\e[1;34m";
                break;
            case 35: 
                colorUser = "\e[1;35m";
                break;
            case 36: 
                colorUser = "\e[1;36m";
                break;
            default: colorUser = "\e[1;37m";
        }
    }
    else {
         switch(userColor){
            case 30: 
                colorUser = "\e[30m"; 
                break;
            case 31: 
                colorUser = "\e[31m";
                break;
            case 32: 
                colorUser = "\e[32m";
                break;
            case 33: 
                colorUser = "\e[33m";
                break;
            case 34: 
                colorUser = "\e[34m";
                break;
            case 35: 
                colorUser = "\e[35m";
                break;
            case 36: 
                colorUser = "\e[36m";
                break;
            default: colorUser = "\e[37m";
        }
    }

    char* colorMachine; //set machine color, checking for boldness

    if (machineBold){
        switch(machineColor){
            case 30: 
                colorMachine = "\e[1;30m";
                break;
            case 31: 
                colorMachine = "\e[1;31m";
                break;

            case 32: 
                colorMachine = "\e[1;32m";
                break;
            case 33: 
                colorMachine = "\e[1;33m";
                break;
            case 34: 
                colorMachine = "\e[1;34m";
                break;
            case 35: 
                colorMachine = "\e[1;35m";
                break;
            case 36: 
                colorMachine = "\e[1;36m";
                break;
            default: colorMachine = "\e[1;37m";
        }
    }

    else {
        switch(machineColor){
            case 30: 
                colorMachine = "\e[30m";
                break;
            case 31: 
                colorMachine = "\e[31m";
                break;

            case 32: 
                colorMachine = "\e[32m";
                break;
            case 33: 
                colorMachine = "\e[33m";
                break;
            case 34: 
                colorMachine = "\e[34m";
                break;
            case 35: 
                colorMachine = "\e[35m";
                break;
            case 36: 
                colorMachine = "\e[36m";
                break;
            default: colorMachine = "\e[37m";
        }
    }
  
    memset(lineIn, 0, 100);
    strcpy(lineIn, "sfish");
    if (promptFlag >= 1) strcat(lineIn, "-");
 
    strcat(lineIn, colorUser);

    if (promptFlag == 1 || promptFlag == 3) strcat(lineIn, user);

    strcat(lineIn, colorMachine);
    if (promptFlag == 3) strcat(lineIn, "@");    

    if (promptFlag >= 2) strcat(lineIn, hostName);
    strcat(lineIn, "\e[0m");
    strcat(lineIn, ":");
    strcat(lineIn, "[");
    strcat(lineIn, dir);
    strcat(lineIn, "]> ");
}