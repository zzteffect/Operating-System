#include "parser.h"
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdio.h>
#include <signal.h>
#include<stdlib.h>
#include <sys/wait.h>
#include<unistd.h>

void sigint_handler(int sigNum){
    signal(SIGINT, sigint_handler);
    //printf("\n Ctrl+C catched. But currently there is no foreground process running. \n");
    fflush(stdout);
}

int array[100];
int tracker;
void trackBackground(){
    int value;
    for(int i = 0; i < tracker; i++){
        value = waitpid(array[i], NULL, WNOHANG);
        if(value == -1){//if 0 -> still running
            perror("wait() error");
        }else if(value != 0){//pid has finished /not running
            array[i] = -1;
            tracker--;
        }
    }
}

void printcmd(struct cmd *cmd)
{
    struct backcmd *bcmd = NULL;
    struct execcmd *ecmd = NULL;
    struct listcmd *lcmd = NULL;
    struct pipecmd *pcmd = NULL;
    struct redircmd *rcmd = NULL;

    int i = 0;

    if(cmd == NULL)
    {
        PANIC("NULL addr!");
        return;
    }
    

    switch(cmd->type){
        case EXEC:
            ecmd = (struct execcmd*)cmd;
            if(ecmd->argv[0] == 0)
            {
                goto printcmd_exit;
            }

            MSG("COMMAND: %s", ecmd->argv[0]);
            for (i = 1; i < MAXARGS; i++)
            {            
                if (ecmd->argv[i] != NULL)
                {
                    MSG(", arg-%d: %s", i, ecmd->argv[i]);
                }
            }
            MSG("\n");

            break;

        case REDIR:
        //head -3 < main.c (redirect input to head -3 with the context in main.c)
        //ls -al > result (redirect output to result)
        //head -3 < main.c > result (input & output at same time)
            rcmd = (struct redircmd*)cmd;

            printcmd(rcmd->cmd);

            if (0 == rcmd->fd_to_close)
            {
                MSG("... input of the above command will be redirected from file \"%s\". \n", rcmd->file);
            }
            else if (1 == rcmd->fd_to_close)
            {
                MSG("... output of the above command will be redirected to file \"%s\". \n", rcmd->file);
            }
            else
            {
                PANIC("");
            }

            break;

        case LIST:
        //ls ; 
            lcmd = (struct listcmd*)cmd;

            printcmd(lcmd->left);
            MSG("\n\n");
            printcmd(lcmd->right);
            
            break;

        case PIPE:
            pcmd = (struct pipecmd*)cmd;

            printcmd(pcmd->left);
            MSG("... output of the above command will be redrecited to serve as the input of the following command ...\n");            
            printcmd(pcmd->right);

            break;

        case BACK:
            bcmd = (struct backcmd*)cmd;

            printcmd(bcmd->cmd);
            MSG("... the above command will be executed in background. \n");    

            break;


        default:
            PANIC("");
    
    }
    
    printcmd_exit:

    return;
}


void runcmd(struct cmd *cmd, int check)
{
    struct backcmd *bcmd = NULL;
    struct execcmd *ecmd = NULL;
    struct listcmd *lcmd = NULL;
    struct pipecmd *pcmd = NULL;
    struct redircmd *rcmd = NULL;
    struct scmd *scmd = NULL;

    pid_t pid;
    int i = 0;
    int staticP;
    int input_descriptor;
    int output_descriptor;

    int pipefd1[2];
    pid_t pid2, pid3;
    pid_t pidb;
    int staticP2;
    
    if(cmd == NULL)
    {
        PANIC("NULL addr!");
        return;
    }
    

    switch(cmd->type){
        case EXEC:
            ecmd = (struct execcmd*)cmd;
            if(ecmd->argv[0] == 0)
            {
                goto printcmd_exit;
            }

            if(check == 0){
                pid = fork();
                if(pid == 0){
                    if(execvp(ecmd->argv[0],  &ecmd->argv[0]) < 0){
                        printf("Command %s does not exist\n", ecmd->argv[0]);
                        exit(0);
                    }
                }
                    trackBackground();
                    waitpid(pid, &staticP, 0);
                    
            }else{
                if(execvp(ecmd->argv[0],  &ecmd->argv[0]) < 0){
                    printf("Command %s does not exist\n", ecmd->argv[0]);
                    exit(0);
                }
                trackBackground();
            }    
            break;

        case REDIR:
            rcmd = (struct redircmd*)cmd;
            int save_descriptor = dup(rcmd->fd_to_close);//stand in & stand out
            if (0 == rcmd->fd_to_close)
            {
                input_descriptor = open(rcmd->file, O_RDWR);
                dup2(input_descriptor, STDIN_FILENO);//refer to thr inputfile descriptor 
                runcmd(rcmd->cmd, 0);
                close(input_descriptor);
                dup2(save_descriptor, STDIN_FILENO);
            }
            else if (1 == rcmd->fd_to_close)
            {
                output_descriptor = open(rcmd->file, O_CREAT|O_TRUNC|O_RDWR, S_IRWXU);
                dup2(output_descriptor, STDOUT_FILENO); 
                runcmd(rcmd->cmd, 0);
                close(output_descriptor);
                dup2(save_descriptor, STDOUT_FILENO);//if not, they will mess up
            }
            else
            {
                PANIC("");//print error code
            }
            break;

        case LIST:
            lcmd = (struct listcmd*)cmd;
            runcmd(lcmd->left, 0);
            runcmd(lcmd->right, 0);
            break;

        case PIPE:
        //cat ./Makefile | head -1
        //cat ./Makefile | head -1 | wc -c
            pcmd = (struct pipecmd*)cmd;

            if(pipe(pipefd1) == -1){
                perror("pipe");
                exit(EXIT_FAILURE);
            }
            
            pid2 = fork();
            if(pid2 == -1){
                perror("pipe");
                exit(EXIT_FAILURE);
            }
            
            if(pid2 == 0){
                dup2(pipefd1[1], STDOUT_FILENO);
                close(pipefd1[1]);
                close(pipefd1[0]);
                runcmd(pcmd->left, 1);
                exit(0);
            }else{
                pid3 = fork();
                if(pid3 == 0){
                    dup2(pipefd1[0], STDIN_FILENO);
                    close(pipefd1[1]);
                    close(pipefd1[0]);
                    runcmd(pcmd->right, 1);
                    exit(0);
                }
                 close(pipefd1[1]);
                 close(pipefd1[0]);
            }
            waitpid(pid2, &staticP,0);
            waitpid(pid3, &staticP,0);
            break;

        case BACK:
        //
        //ls
        //press enter
        //
            bcmd = (struct backcmd*)cmd; 
            pidb = fork();
            if(pidb == 0){
                runcmd(bcmd->cmd, 1); //don't need to fork in exec
            }
            array[tracker] = pidb;
            tracker++;
            break;

        default:
            PANIC("");
    }

    printcmd_exit:
    return;
}

int main(void)
{
    static char buf[1024];
    int fd;

    setbuf(stdout, NULL);
    // Read and run input commands.
    while(getcmd(buf, sizeof(buf)) >= 0)
    {
        struct cmd * command;
        command = parsecmd(buf);
        runcmd(command, 0);
    }
    PANIC("getcmd error!\n");
    return 0;
}
