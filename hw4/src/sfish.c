#include "sfish.h"

/* BUILT-IN FUNCTIONS PSEUDOCODE */
/* input = readline()
if (builtin_func = get_builtin(input))
    fork()
    if child:
        builtin_func()
    else:
        wait_for_child() */

const char *built_in[11] = {"help", "cd", "pwd", "prt", "chpmt", "chclr", "jobs", "fg", "bg", "kill", "disown"};
static char* lineIn;
static int status = 0;
static char* user;
static char* home;
static char* hostName;
static char* dir;
static char* cwd;


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

static char *fileIn; //the input redirect <
static char *fileOut;//the output redirect >
static int inFlag = 0; // boolean flag for whether there is an input file
static int outFlag = 0; // boolean flag for whether there is an output file, set to 2 if writing to stderr

int main(int argc, char** argv) {
    //DO NOT MODIFY THIS. If you do you will get a ZERO.
    rl_catch_signals = 0;
    //This is disable readline's default signal handlers, since you are going
    //to install your own.
    rl_startup_hook = (void*)readlineKeybinds;

    char** args = malloc(200);
    memset(args, 0, 200);

    char *cmd;
    char *cmdCopy; //make a copy of the input command to manipulate
   // char *cmdExec; //the section to store the execution part of the input command

    fileIn = malloc(100); //the input redirect <
    fileOut = malloc(100); //the output redirect >
   // char *cmdPipe; //pipe redirect

    
   // int pipeNum = 0; //number of pipe executions

    cwd = malloc(50);

    int gotHost;
    hostName = malloc(20);

    
    if ((gotHost=gethostname(hostName, 20)) == -1) hostName = "{unspecified machine}";

    
    user = getenv("USER");
    home = getenv("HOME");
    getcwd(cwd, 50);

    dir = malloc(100);
    setDir();

    lineIn = malloc(100);
    setPrompt();

    while((cmd = readline(lineIn)) != NULL) {
        if (strlen(cmd)==0){}
        else {

            //parse the input command into a string array
            int i;


            cmdCopy = strdup(cmd);

            //find redirection
            char* fileInS = strchr(cmdCopy, '<');
            //find the first instance of <, then find the next >, < or |
            //if none found, then end of command string, insert null terminator
            //move the string to file In 
            if (fileInS) {
                inFlag = 1;
                
                char* fileInP;
                fileInS[0]=' ';
                fileInS+=1;
                if (fileInS[0] ==' ') fileInS+=1; //if theres a space, skip it
                fileInP = strpbrk(fileInS, "><| ");
                if (!fileInP) fileInP = fileInS + strlen(fileInS);

                memmove(fileIn, fileInS, fileInP-fileInS);
                for (int i = 0; i < (fileInP-fileInS); i++) fileInS[i] = ' ';
                //printf("%s\n", fileIn);
            }

            //find output redirection
            char* fileOutS = strchr(cmdCopy, '>');
            if (fileOutS) {
                outFlag = 1;

                if(fileOutS[-1]=='2'){ //2 found before >, set output to stderr
                    outFlag =2;
                    fileOutS[-1]=' ';
                }
                            
                char* fileOutP;
                fileOutS[0]=' ';
                fileOutS+=1;
                if (fileOutS[0] ==' ') fileOutS+=1; //if theres a space, skip it
                fileOutP = strpbrk(fileOutS, "><| ");
                if (!fileOutP) fileOutP = fileOutS + strlen(fileOutS);
                
                memmove(fileOut, fileOutS, fileOutP-fileOutS);
                for (int i = 0; i < (fileOutP-fileOutS); i++) fileOutS[i] = ' ';
                //printf("%s\n", fileOut);
            }

            //separate exec commands by arguments
            i = 0; //reset i to 0
            while ((args[i] = strsep(&cmdCopy, " ")) != NULL){
                if(strlen(args[i]) > 0) i++;
                else ;
            }

            

            if (strcmp(args[0],"quit")==0)
                break;

            if (strcmp(args[0], "exit")==0)
                break;

            //All your debug print statments should be surrounded by this #ifdef
            //block. Use the debug target in the makefile to run with these enabled.
            /*#ifdef DEBUG
            
            fprintf(stderr, "Length of command entered: %ld\n", strlen(cmd));
            for (int j=0; j < i;j++) fprintf(stderr,"%s\t", args[j]);
            fprintf(stderr,"\n"); 
            
            #endif*/
            //You WILL lose points if your shell prints out garbage values.            

            //call binaries, first checks builtin, if that fails, try to exec 
            if(!get_builtin(cmd, args)) get_exec(cmd, args);
        }

    }

    //Don't forget to free allocated memory, and close file descriptors.
    free(fileIn);
    free(fileOut);
    free(cwd);
    free(cmdCopy);
    free(cmd);
    free(dir);
    free(hostName);
    free(lineIn);
    free(args);
        //WE WILL CHECK VALGRIND!

    return EXIT_SUCCESS;
}

void inRedir(){

    //open the input file
    
    int fd = open(fileIn, O_RDWR);
    if(fd==-1) {
        close(fd);
        return;
    }
    else {
        dup2(fd, STDIN_FILENO);    
        close(fd);
    }
    
}


void outRedir(){

    //set the mode for output files that may be opened
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    int fd = open(fileOut, O_RDWR | O_CREAT | O_TRUNC, mode);
    if(fd==-1){
        close(fd);
        return;
    }
    if(outFlag == 2) dup2(fd, STDERR_FILENO); //if flag==2, set to stderr
    else dup2(fd, STDOUT_FILENO);  //otherwise set to stdout
    close(fd);
}

void setDir(){

    getcwd(cwd, 50);
    memset(dir, 0, 100);
    
    int compare = strcmp(home, cwd);

    if (compare==0) strcat(dir, "~");
    else if (compare < 0) {
        strcat(dir, "~");
        strcat(dir, cwd+strlen(home));
    } 
    else dir = strcpy(dir,cwd);;
}

int get_builtin(char *cmd, char** args) {
    //returns 1 on success, 0 on failure
    pid_t child_id;
    int i;
    char* input = args[0];

    for (i = 0; i < 11; i++){
        if (strcmp(input, built_in[i])==0) //compare to static defined list of built in commands
            break;
    }
    switch(i){
        case 0: /* help */
            if ((child_id=fork())==0) {
                if(outFlag) outRedir();
                sfish_help();
                
            }
            else {
                wait(&status);
                if(outFlag){
                    memset(fileOut, 0, 100);
                    outFlag=0;
                }
            }
            break;
        case 1: /* cd */
            sfish_cd(args);
            break;
        case 2: /* pwd */
            if ((child_id=fork())==0) {
                if(outFlag) outRedir();
                sfish_pwd();
            }
            else {
                wait(&status);
                if(outFlag){
                    memset(fileOut, 0, 100);
                    outFlag=0;
                }
            }
            break;
        case 3: /* prt */
            if ((child_id=fork())==0) {
                if(outFlag) outRedir();
                sfish_prt(status);
            }
            else {
                wait(&status);
                if(outFlag){
                    memset(fileOut, 0, 100);
                    outFlag =0;
                }
            }
            break;
        case 4: //chpmt
            sfish_chpmt(args);
            break;
        case 5: //chclr
            sfish_chclr(args);
            break;
        case 6: //jobs
            if ((child_id=fork())==0) {
                if(outFlag) outRedir();
                sfish_jobs(args);
            }
            else {
                wait(&status);
                if(outFlag){
                    memset(fileOut, 0, 100);
                    outFlag =0;
                }
            }
            break;
        case 7: //fg
            sfish_fg(args);
            break;
        case 8: //bg
            sfish_bg(args);
            break;
        case 9: //kill
            sfish_kill(args);
            break;
        case 10: //disown
            sfish_disown(args);
            break;
        default: return 0; //not a built in command
    }

    return 1;
    
}

int statPath(char* bin){
    int n = -1;
    int i = 0; //path string sep counter
    int pathLen = strlen(getenv("PATH"))+1;
    char* path = malloc(pathLen);
    
    strncpy(path, getenv("PATH"), pathLen-1);
    
    char** checkPaths = malloc(pathLen+50);
    struct stat* buf = malloc(sizeof(struct stat));
    memset(checkPaths, 0, pathLen+50);
    
    while ((checkPaths[i] = strsep(&path, ":")) != NULL){
        i++;    
    }

    
    char* find = malloc(pathLen);

    memset(find, 0, pathLen);
    getcwd(find, 50);
    if (bin[0]=='.') bin+=1;
    else strcat(find, "/");
    strcat(find, bin);    
    n=stat(find, buf);

    if (n!=0){
        for (int j = 0; j < i; j++){
            
            memset(find, 0, pathLen);
            strcpy(find, checkPaths[j]);
            strcat(find, "/");
            strcat(find, bin);
            
            n=stat(find, buf);
            if (n==0) break;   
        }
    }

    free(find);
    free(checkPaths);
    free(buf);
    free(path);
    return n;
}

int get_exec(char *cmd, char** args){
    //returns 1 on success, 0 on failure

    int child_id;

    //TO DO: use stat system call to check file
    
    if ((child_id=fork())==0){
        if(inFlag) inRedir();
        if(outFlag) outRedir();

        /*execvp(args[0], args);
        exit(EXIT_SUCCESS);*/

        if (statPath(args[0]) == 0){
            
            execvp(args[0], args);
            exit(EXIT_SUCCESS);
        }
        else {
            fprintf(stderr, "No command %s found.\n", args[0]);
            exit(EXIT_FAILURE);
        }
        
    }

    else {
        wait(&status);
        if(inFlag){
            memset(fileIn, 0, 100);
            inFlag = 0;
        }
        if(outFlag){
            memset(fileOut, 0, 100);
            outFlag = 0;
        }

    }

    return 1;
}

void sfish_help(){
    printf("%s", help);
    exit(EXIT_SUCCESS);
}

void sfish_cd(char** args){
    char* arg;
    if ((arg = args[1]) != NULL){

        if (strcmp(arg, ".")==0) {
            chdir(".");
            return;
        }  

        else if (strcmp(arg, "..")==0){
            chdir("..");

            setDir();
            setPrompt();
            return;

        }

        else { //must include / in directory path
            char* cdDir = malloc(50);
            memset(cdDir, 0 , 50);
            getcwd(cwd, 50);
            strcat(cdDir, cwd);
            if(arg[0]!='/') strcat(cdDir, "/");
            strcat(cdDir, arg);
            if(chdir(cdDir)==-1) {
                fprintf(stderr,"Invalid directory. Error number:%d\n", errno);
                free(cdDir);
                return;
            }

            setDir();
            setPrompt();
            free(cdDir);
            return;
        }

        return;
    }
    else { //if command is just cd with no argument, go to home directory
        chdir(getenv("HOME"));
        setDir();
        setPrompt();
        return;
    }
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

void sfish_chpmt(char **args){
    
    char* arg1;
    char* arg2;
    if ((arg1 = args[1]) != NULL) {
        if((arg2 = args[2]) != NULL);
        else {
            fprintf(stderr, "Missing argument for chpmt command.\n");
            return;
        }
    }
    else {
        fprintf(stderr, "Missing argument for chpmt command.\n");
        return;
    }

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

void sfish_chclr(char **args){

    char* arg1;
    char* arg2;
    char* arg3;
    if ((arg1 = args[1]) != NULL){ 
        if((arg2 = args[2]) != NULL) {
            if((arg3 = args[3]) != NULL);
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

void sfish_jobs(char **args){

    printf("JID\tStatus\tPID\tJob\n");
    exit(EXIT_SUCCESS);
}

void sfish_fg(char **args){

}

void sfish_bg(char **args){

}

void sfish_kill(char **args){

}

void sfish_disown(char **args){

}

void setPrompt(){

    //printf("\e[31mHello\e[32m, \e[34mBlue \e[32mWorld\e[0m.\n");

    char* colorUser; //set user color, checking for boldness

    if(userBold) {
        switch(userColor){
            case 30: 
                colorUser = "\001\e[1;30m\002"; 
                break;
            case 31: 
                colorUser = "\001\e[1;31m\002";
                break;
            case 32: 
                colorUser = "\001\e[1;32m\002";
                break;
            case 33: 
                colorUser = "\001\e[1;33m\002";
                break;
            case 34: 
                colorUser = "\001\e[1;34m\002";
                break;
            case 35: 
                colorUser = "\001\e[1;35m\002";
                break;
            case 36: 
                colorUser = "\001\e[1;36m\002";
                break;
            default: colorUser = "\001\e[1;37m\002";
        }
    }
    else {
         switch(userColor){
            case 30: 
                colorUser = "\001\e[30m\002"; 
                break;
            case 31: 
                colorUser = "\001\e[31m\002";
                break;
            case 32: 
                colorUser = "\001\e[32m\002";
                break;
            case 33: 
                colorUser = "\001\e[33m\002";
                break;
            case 34: 
                colorUser = "\001\e[34m\002";
                break;
            case 35: 
                colorUser = "\001\e[35m\002";
                break;
            case 36: 
                colorUser = "\001\e[36m\002";
                break;
            default: colorUser = "\001\e[37m\002";
        }
    }

    char* colorMachine; //set machine color, checking for boldness

    if (machineBold){
        switch(machineColor){
            case 30: 
                colorMachine = "\001\e[1;30m\002";
                break;
            case 31: 
                colorMachine = "\001\e[1;31m\002";
                break;

            case 32: 
                colorMachine = "\001\e[1;32m\002";
                break;
            case 33: 
                colorMachine = "\001\e[1;33m\002";
                break;
            case 34: 
                colorMachine = "\001\e[1;34m\002";
                break;
            case 35: 
                colorMachine = "\001\e[1;35m\002";
                break;
            case 36: 
                colorMachine = "\001\e[1;36m\002";
                break;
            default: colorMachine = "\001\e[1;37m\002";
        }
    }

    else {
        switch(machineColor){
            case 30: 
                colorMachine = "\001\e[30m\002";
                break;
            case 31: 
                colorMachine = "\001\e[31m\002";
                break;
            case 32: 
                colorMachine = "\001\e[32m\002";
                break;
            case 33: 
                colorMachine = "\001\e[33m\002";
                break;
            case 34: 
                colorMachine = "\001\e[34m\002";
                break;
            case 35: 
                colorMachine = "\001\e[35m\002";
                break;
            case 36: 
                colorMachine = "\001\e[36m\002";
                break;
            default: colorMachine = "\001\e[37m\002";
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
    strcat(lineIn, "\001\e[0m\002");
    strcat(lineIn, ":");
    strcat(lineIn, "[");
    strcat(lineIn, dir);
    strcat(lineIn, "]> ");
}

int readlineKeybinds(){
    rl_command_func_t* helpCmd= (rl_command_func_t*)callHelp;
    rl_bind_keyseq("\\C-h", helpCmd);

    return 1;
}

void callHelp (){
    printf("\n");
    char* help[1] = {"help"};
    get_builtin("help", help);
    readline(lineIn);
}