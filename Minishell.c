#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "main.h"
char *builtins[] = {"echo", "printf", "read", "cd", "pwd", "pushd", "popd", "dirs", "let", "eval",
						"set", "unset", "export", "declare", "typeset", "readonly", "getopts", "source",
						"exit", "exec", "shopt", "caller", "true", "type", "hash", "bind", "help","jobs","fg","bg",NULL};

char *command[20];
char *external_command[154];
char input_string[100] = {'\0'};
int i = 0,pid,status,child_pid;
 char promptstring[50] = "minisheel: $ ";

int main()
{

    system("clear");
   
    /*Registering signals*/

    signal(SIGINT,signal_handler);
    signal(SIGTSTP,signal_handler);
    signal(SIGTSTP,signal_handler);


    extract_external_commands(external_command);

    while (1)
    {
        

        scan_input(promptstring,input_string);
        

        if (strncmp(input_string, "PS1=", 4) == 0)
        {
            copy_change(promptstring,input_string);
            continue;

        }
        else if(input_string[0] == '\0')
        {
            continue;
        }
        
        get_command(input_string);

        int res = check_command_type(command[i]);
        
         pid = getpid();
    
        if(res == EXTERNAL)
        {
             int ret = fork();

            if(ret == -1)
            {
                perror("fork");
            }

            if(ret == 0)
            {
                signal(SIGINT,SIG_DFL);
                signal(SIGTSTP,SIG_DFL);
                execute_external_commands(command);
            }
            else if(ret > 0)
            {
                child_pid = ret;
                waitpid(ret,&status,WUNTRACED);
                child_pid = 0;
            }
        }
        else if(res == BUILTIN)
        {
           
            execute_internal_commands(command);
        }
        
        if(res == NO_COMMAND)
        {
            printf("invalid command\n");
        }
        bzero(input_string,sizeof(input_string));
    }

}