#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/stat.h> 
#include <fcntl.h>


const char* help[11] = {
	"SFISH Help menu:\n",
	"cd [dir]\t\t\tGo to directory\n",
	"chclr [option] [color] [0|1]\tChange prompt color\n",
	"chpmt [option] [0|1]\t\tChange prompt settings\n",
	"exit\t\t\t\tExit shell\n",
	"help\t\t\t\tPrint this menu\n",
	"jobs\t\t\t\tDisplay job list\n",
	"kill [signal] [process id]\tSend signal to given process\n",
	"prt\t\t\t\tPrint status of last exited command\n",
	"pwd\t\t\t\tPrint current working directory\n",
	"quit\t\t\t\tExit shell\n"
};



typedef void (handler_t)(int);
handler_t* signal(int signum, handler_t *handler);

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

void stop_fgp();

void callHelp();

void sfish_info();

void sigint_handler(int sig); /*SIGINT handler*/

void sigstp_handler(int sig);

void sigcont_handler(int sig);