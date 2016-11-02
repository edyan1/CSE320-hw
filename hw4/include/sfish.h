#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>


const char* help = "help menu:\n blah blah\n some other stuff\n and some more stuff\n";

//return 1 on success, 0 on failure
int get_builtin(char* cmd, char** args);

//return 1 on success, 0 on failure
int get_exec(char* cmd, char** args);

void sfish_help();

void sfish_exit();

void sfish_cd(char** args);

void sfish_pwd();

void sfish_prt(int status);

void sfish_chpmt(char **args);

void sfish_chclr(char **args);

void setPrompt();