#include "main.h"
extern char *command[20],input_string[100];
extern char *external_command[153];
extern char *builtins[];
extern int status;
extern int pid ,child_pid;
static int count;
extern  char promptstring[50];
signal_list *head = NULL;
 

void insert_at_first(signal_list **head,char **input_string,pid_t npid)
{
    static int count = 0;

    signal_list *new = malloc(sizeof(signal_list));

    if(new == NULL)
        return;

    new->sl_no = ++count;
    new->Pid = npid;

    new->cmd = malloc(strlen(input_string[0] +1));
    strcpy(new->cmd,input_string[0]);

    if (input_string[1] != NULL)
    {
        new->data = malloc(strlen(input_string[1]) + 1);
        strcpy(new->data, input_string[1]);
    }
    else
    {
        new->data = NULL;
    }
    new->link = *head;
    *head = new;
}

void delete_first(signal_list **head)
{
   

    if(*head == NULL)
    {
        return;
    }
    else
    {
         signal_list *temp = *head;
         *head = (*head)->link;
         free(temp->cmd);
         if(temp->data)
         free(temp->data);
         free(temp);
    }
}

void scan_input(char *prompt, char *input_string)
{
        printf("%s ",prompt);
        fflush(stdout);

        scanf("%[^\n]",input_string);
        getchar();   // Consume '\n'
}

void copy_change(char *prompt, char *input_string)
{
    if (strchr(input_string + 4, ' ') != NULL || strlen(input_string + 4) == 0)
    {
        printf("Invalid command\n");
    }
    else 
    {
        strcpy(prompt, input_string + 4);
    }
    
}

void extract_external_commands(char **external_commands)
{
    int fd;
    fd = open("external_command.txt",O_RDONLY);

    if(fd == -1)
    {
        perror("not able open file");
        return ;
    }
    char buffer[100] = "";
    char ch;
    int j = 0, i= 0;

    while(read(fd,&ch,1) != 0)
    {
        if(ch == '\n')
        {
            buffer[j] = '\0';

            external_commands[i] = malloc(strlen(buffer)+1);
            strcpy(external_commands[i],buffer);

            i++;
            j = 0;
        }
        else
        {
            buffer[j++] = ch;
        }
    }
    /*if last word/command does not end with newline*/
    if(j > 0)
    {
        buffer[j] = '\0';
        external_commands[i] = malloc(strlen(buffer)+1);
        strcpy(external_commands[i],buffer);
        i++;
    }
    external_commands[i] = NULL;

    close(fd);
}

void get_command(char *input_string)
{
    int i = 0;
    command[i] = strtok(input_string," ");
    while(command[i]!= NULL)
    {
        i++;
        count++;
        command[i] = strtok(NULL, " ");
    }
}

int check_command_type(char *command)
{
    int i = 0;
    while(builtins[i] != NULL)
    {
        if(strcmp(command,builtins[i]) == 0)
        {
            return BUILTIN;
        }
        i++;
    }
    i = 0;
    while (external_command[i] != NULL)
    {
        if(strcmp(command,external_command[i]) == 0)
        {
            return EXTERNAL;
        }
        i++;
    }
    return NO_COMMAND;
    
}

void execute_external_commands(char **ex_command)
{
    int pipe_count = 0;
    int cmd_index[25];
    int cmd_count = 0,i = 1;
    
    /* First command cannot be pipe */
    if(strcmp(ex_command[0], "|") == 0)
    {
        printf("bash: syntax error near unexpected token `|'\n");
        return ;
    }

    /* Store first command index */
    cmd_index[cmd_count++] = 0;

    /* Check pipes */
    while (ex_command[i])
    {
        if(strcmp(ex_command[i], "|") == 0)
        {
            pipe_count++;

            /* Pipe cannot be last */
            if(ex_command[i+1] == NULL)
            {
                printf("Error: Pipe cannot be last argument\n");
                return ;
            }
            /* Consecutive pipe check */
            if(strcmp(ex_command[i+1], "|") == 0)
            {
                printf("Error: Consecutive pipes are not allowed\n");
                return ;
            }


            /*stroing NUll in place of pipe*/
            ex_command[i] = NULL;

            /* Store next command starting index */
            cmd_index[cmd_count++] = i + 1;
        }
        i++;
    }

    if(pipe_count > 0)
    {
        int fd[2];

        for(int j = 0; j < count; j++)
        {
            /* create pipe except last command */
            if(i != (cmd_count - 1))
            {
                if(pipe(fd) == -1)
                {
                    perror("pipe");
                    return;
                }
            }

            int ret2 = fork();

            if(ret2 == 0)
            {

                /* send output to next pipe */
                if(j != (cmd_count - 1))
                {
                    dup2(fd[1], 1);
                }

                /* close unused descriptors */
                if(j != (cmd_count - 1))
                {
                    close(fd[0]);
                    close(fd[1]);
                }

                execvp(ex_command[cmd_index[j]], ex_command + (cmd_index[j]));

                perror("execvp");
                exit(1);
            }
            else if(ret2 > 0)
            {
                
                if(j != (cmd_count - 1))
                {
                    dup2(fd[0], 0);

                    close(fd[0]);
                    close(fd[1]);
                }
            }
        }

        for(int i = 0; i < cmd_count; i++)
        {
            wait(NULL);
        }
        exit(1);
    }
    else  if (pipe_count == 0)
    {
        if(execvp(ex_command[0], ex_command) == -1)
        {
            perror("execvp");   // Prints why execvp failed
            exit(1);
        }
    }
}

void execute_internal_commands(char **input_string)
{
    int j = 0;
    if(strcmp("cd",input_string[j]) == 0)
    {
        if(input_string[1] == NULL)
        {
            char *home = getenv("HOME");
            if(home)
            {
                chdir(home);
            }
        }
        else 
        {
            if(chdir(input_string[1]) != 0)
            {
                perror("chdir");
            }
        }
        
    }  
    else if(strcmp("exit",input_string[j]) == 0)
    {
        exit(1);
    } 
    else if(strcmp("pwd",input_string[j])==0)
    {
        char *pwd = getcwd(NULL,0);
        // char *pwd = malloc(1024);
    
        // if(pwd == NULL)
        // {
        //     perror("malloc");
        //     return;
        // }

        if(pwd != NULL)
        {
            printf("%s\n",pwd);

        }
        else
        {
            perror("getcwd");
        }
        free(pwd);
    }
    else if(strcmp("echo",input_string[j])==0)
    {
        if(strcmp("$SHELL",input_string[j+1]) == 0)
        {
            char *buffer = getenv("SHELL");
            printf("%s\n",buffer);
        } 
        else if(strcmp("$$",input_string[j+1] )== 0)
        {
            printf("%d\n",pid);
        }
        else if(strcmp("$?",input_string[j+1])== 0)
        {
            printf("%d\n",status);
        }
    }
    else if(strcmp("jobs",input_string[j])==0)
    {
        signal_list *node[100];
        int n = 0;
        for(signal_list *temp = head; temp != NULL; )
        {  
             node[n++] = temp;
             temp = temp->link;
        }
        while(n--)
        {
            printf("[%d]  stopped     %s",node[n]->sl_no,node[n]->cmd);
            if(node[n]->data)
            printf("  %s\n",node[n]->data);
        }

    }
    else if(strcmp("fg",input_string[j])==0)
    {
        if(head == NULL)
        {
            printf("bash: fg: current: no such job\n");
            return;
        }
        else
        {
            kill(head->Pid,SIGCONT);
            printf("%s %s\n",head->cmd,head->data);
            waitpid(head->Pid,&status,WUNTRACED);
            delete_first(&head);
        }
    }
    else if(strcmp("bg",input_string[j])==0)
    {
        if(head == NULL)
        {
            printf("bash: bg: current: no such job\n");
            return;
        }
        else 
        {
            kill(head->Pid,SIGCONT);

            if(head->sl_no == 1)
            {
                printf("[%d] %s %s &\n",head->sl_no,head->cmd,head->data);
                printf("[%d] Done   %s %s\n",head->sl_no,head->cmd,head->data);

            }
            else
            {
                printf("[%d] %s %s &\n",head->sl_no,head->cmd,head->data);
            }
        
            delete_first(&head);
        }

    }
}

void signal_handler(int sig_num)
{
    if(sig_num == SIGINT)
    {
        /* without command*/
        if(child_pid == 0)
        {
            write(STDOUT_FILENO, "\n", 1);
            write(STDOUT_FILENO, promptstring, strlen(promptstring));
        }
    }
    else if(sig_num == SIGTSTP)
    {
        if(child_pid == 0)
        {
            write(STDOUT_FILENO, "\n", 1);
            write(STDOUT_FILENO, promptstring, strlen(promptstring));
        }
        else if(child_pid > 0)
        {
            int i = 0;
            static int count = 0;
            insert_at_first(&head,command,child_pid);
            if(command[i+1] == NULL)
            printf("[%d] Stopped        %s \n",++count,command[i]);
            else
             printf("[%d] Stopped        %s %s\n",++count,command[i],command[i+1]);

        }
    }
    else if(sig_num == SIGCHLD)
    {
        waitpid(-1,&status,WNOHANG);
    }

}

