#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/stat.h> 
#include <fcntl.h>


const char* help = "help menu:\n blah blah\n some other stuff\n and some more stuff\n";



//input redirection method
void inRedir();

//output redirection method
void outRedir();

//set the static dir char pointer
void setDir();

//return 1 on success, 0 on failure
int get_builtin(char* cmd, char** args);

//use stat on each path in $PATH to check for the executable binary
int statPath(char* bin);

//return 1 on success, 0 on failure
int get_exec(char* cmd, char** args);

void sfish_help();

void sfish_exit();

void sfish_cd(char** args);

void sfish_pwd();

void sfish_prt(int status);

void sfish_chpmt(char **args);

void sfish_chclr(char **args);

void sfish_jobs(char **args);

void sfish_fg(char **args);

void sfish_bg(char **args);

void sfish_kill(char **args);

void sfish_disown(char **args);

void setPrompt();

int readlineKeybinds();

void callHelp();

void sfish_info();