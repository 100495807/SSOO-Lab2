//P2-SSOO-23/24

//  MSH main file
// Write your msh source code here

//#include "parser.h"
#include <stddef.h>			/* NULL */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_COMMANDS 8

int accum = 0;
// files in case of redirection
char filev[3][64];

//to store the execvp second parameter
char *argv_execvp[8];

void siginthandler(int param)
{
	printf("****  Exiting MSH **** \n");
	//signal(SIGINT, siginthandler);
	exit(0);
}

/* myhistory */

void mycalc(char ***argvv, int accum) {
    // Verificar si el comando es mycalc
    if (strcmp(argvv[0][0], "mycalc") == 0) {
        // Verificar si se proporcionan los tres argumentos requeridos
        if (argvv[0][1] != NULL && argvv[0][2] != NULL && argvv[0][3] != NULL) {
            // Obtener los operandos y el operador
            int op1 = atoi(argvv[0][1]);
            int op2 = atoi(argvv[0][3]);
            char *operator = argvv[0][2];

            int result;
            char buf[100];  // Buffer para almacenar el mensaje de salida
            // Realizar la operación según el operador
            if (strcmp(operator, "add") == 0) {
                // Suma: sumar operandos y actualizar la variable de entorno "Acc"
                result = op1 + op2;
                accum += result;
                // Crear el mensaje de salida
                sprintf(buf, "[OK] %d + %d = %d; Acc %d\n", op1, op2, result, result);
            } else if (strcmp(operator, "mul") == 0) {
                // Multiplicación: multiplicar operandos
                result = op1 * op2;
                // Crear el mensaje de salida
                sprintf(buf, "[OK] %d * %d = %d\n", op1, op2, result);
            } else if (strcmp(operator, "div") == 0) {
                // División: calcular cociente y resto
                int quotient = op1 / op2;
                int remainder = op1 % op2;
                // Crear el mensaje de salida
                sprintf(buf, "[OK] %d / %d = %d; Resto %d\n", op1, op2, quotient, remainder);
            } else {
                // Operador no válido
                // Crear el mensaje de salida de error
                sprintf(buf, "[ERROR] La estructura del comando es mycalc <operando 1> <add/mul/div> <operando 2>\n");
            }

            // Escribir el mensaje en la salida estándar de error
            if (write(2, buf, strlen(buf)) < strlen(buf)) {
                perror("Error in write\n");
            }
        } else {
            // Estructura de comando incorrecta
            // Crear el mensaje de salida de error
            char *error_msg = "[ERROR] La estructura del comando es mycalc <operando 1> <add/mul/div> <operando 2>\n";
            // Escribir el mensaje en la salida estándar
            if (write(1, error_msg, strlen(error_msg)) < strlen(error_msg)) {
                perror("Error in write\n");
            }
        }
    }
}

/* myhistory */

struct command
{
  // Store the number of commands in argvv
  int num_commands;
  // Store the number of arguments of each command
  int *args;
  // Store the commands
  char ***argvv;
  // Store the I/O redirection
  char filev[3][64];
  // Store if the command is executed in background or foreground
  int in_background;
};

int history_size = 20;
struct command * history;
int head = 0;
int tail = 0;
int n_elem = 0;

void free_command(struct command *cmd)
{
    if((*cmd).argvv != NULL)
    {
        char **argv;
        for (; (*cmd).argvv && *(*cmd).argvv; (*cmd).argvv++)
        {
            for (argv = *(*cmd).argvv; argv && *argv; argv++)
            {
                if(*argv){
                    free(*argv);
                    *argv = NULL;
                }
            }
        }
    }
    free((*cmd).args);
}

void store_command(char ***argvv, char filev[3][64], int in_background, struct command* cmd)
{
    int num_commands = 0;
    while(argvv[num_commands] != NULL){
        num_commands++;
    }

    for(int f=0;f < 3; f++)
    {
        if(strcmp(filev[f], "0") != 0)
        {
            strcpy((*cmd).filev[f], filev[f]);
        }
        else{
            strcpy((*cmd).filev[f], "0");
        }
    }

    (*cmd).in_background = in_background;
    (*cmd).num_commands = num_commands-1;
    (*cmd).argvv = (char ***) calloc((num_commands) ,sizeof(char **));
    (*cmd).args = (int*) calloc(num_commands , sizeof(int));

    for( int i = 0; i < num_commands; i++)
    {
        int args= 0;
        while( argvv[i][args] != NULL ){
            args++;
        }
        (*cmd).args[i] = args;
        (*cmd).argvv[i] = (char **) calloc((args+1) ,sizeof(char *));
        int j;
        for (j=0; j<args; j++)
        {
            (*cmd).argvv[i][j] = (char *)calloc(strlen(argvv[i][j]),sizeof(char));
            strcpy((*cmd).argvv[i][j], argvv[i][j] );
        }
    }
}


/**
 * Get the command with its parameters for execvp
 * Execute this instruction before run an execvp to obtain the complete command
 * @param argvv
 * @param num_command
 * @return
 */
void getCompleteCommand(char*** argvv, int num_command) {
	//reset first
	for(int j = 0; j < 8; j++)
		argv_execvp[j] = NULL;

	int i = 0;
	for ( i = 0; argvv[num_command][i] != NULL; i++)
		argv_execvp[i] = argvv[num_command][i];
}


/**
 * Main sheell  Loop  
 */
int main(int argc, char* argv[])
{
	/**** Do not delete this code.****/
	int end = 0; 
	int executed_cmd_lines = -1;
	char *cmd_line = NULL;
	char *cmd_lines[10];

	if (!isatty(STDIN_FILENO)) {
		cmd_line = (char*)malloc(100);
		while (scanf(" %[^\n]", cmd_line) != EOF){
			if(strlen(cmd_line) <= 0) return 0;
			cmd_lines[end] = (char*)malloc(strlen(cmd_line)+1);
			strcpy(cmd_lines[end], cmd_line);
			end++;
			fflush (stdin);
			fflush(stdout);
		}
	}

	/*********************************/

	char ***argvv = NULL;
	int num_commands;

	history = (struct command*) malloc(history_size *sizeof(struct command));
	int run_history = 0;

	while (1) 
	{
		int status = 0;
		int command_counter = 0;
		int in_background = 0;
		signal(SIGINT, siginthandler);

		if (run_history)
    {
        run_history=0;
    }
    else{
        // Prompt 
        write(STDERR_FILENO, "MSH>>", strlen("MSH>>"));

        // Get command
        //********** DO NOT MODIFY THIS PART. IT DISTINGUISH BETWEEN NORMAL/CORRECTION MODE***************
        executed_cmd_lines++;
        if( end != 0 && executed_cmd_lines < end) {
            command_counter = read_command_correction(&argvv, filev, &in_background, cmd_lines[executed_cmd_lines]);
        }
        else if( end != 0 && executed_cmd_lines == end)
            return 0;
        else
            command_counter = read_command(&argvv, filev, &in_background); //NORMAL MODE
    }
		//************************************************************************************************
        int curr_command = -1; // identifies which child is in charge of which command

		/************************ STUDENTS CODE ********************************/
	   if (command_counter > 0) {
			if (command_counter > MAX_COMMANDS){
				printf("Error: Maximum number of commands is %d \n", MAX_COMMANDS);
			}
			else {
                
				/* TUBERIAS */

                // CONFIGURACION DE TUBERIAS
                int pipe0[2];
                int pipe1[2];
                pipe(pipe0);
                pipe(pipe1);

                // creacion de subprocesos 
                int pid = 1;
                for (int i = 0; i < command_counter; i++) {
                    if (pid > 0) {                                              // los forks solo los puede hacer el padre
                        pid = fork();
                        if (pid == 0) { // hijo
                            curr_command = i;
                        }
                    } 
                    else break;                                                 //si es el hijo no hace frok y sale del bucle
                }
                
                //error
                if (pid == -1) { 
                    perror("ERROR: en el fork");
                    return -1;
                } 

                 // proceso padre
                else if (pid != 0) {

                    //cerramos las tuberias
                    close(pipe0[0,1]);
                    close(pipe1[0,1]);

                    /* BACKGROUND */
                           
                    if (in_background != 1) {
                        while (wait(&status) != pid) {                          // esparamos a que termine el hijo
                            if (status != 0) {
                                perror("ERROR: ejecutando el hijo");
                            } 
                        }
                    }
                } 

                // proceso hijo
                else {                                                          
                           
                    /* REDIRECCIONAMIENTO */
                    
                    // redireccion de entrada, file[0] como stdin 
                    if ((curr_command == 0) && (filev[0][0] != '0')) {
                        close(STDIN_FILENO);                                                        
                        int fd = open(filev[0], O_RDONLY);                                          
                    } 

                    // redireccion de salida, file[1] como stdout 
                    else if ((curr_command == command_counter - 1) && (filev[1][0] != '0')) {
                        close(STDOUT_FILENO);
                        int fd = open(filev[1], O_CREAT | O_RDWR, S_IRWXU);                          
                    }

                    // redireccion de error, file[1] como stderr 
                    if (filev[2][0] != '0') {
                        close(STDERR_FILENO);
                        int fd = open(filev[2], O_CREAT | O_RDWR, S_IRWXU);                          
                    }

                    /* TUBERIAS */

                    if (command_counter > 1) {

                        // primer comando
                        if (curr_command == 0) {                                
                            close(pipe1[0,1]);
                            close(pipe0[0]);
                            dup2(pipe0[1], STDOUT_FILENO);                      // stdout es ahora la tuberia de escritura
                            close(pipe0[1]);
                        } 

                        // ultimo comando, tuberia de entrada 
                        else if (curr_command == command_counter - 1) {         
                            if ((curr_command % 2) != 0) {                      // si el comando es inpar leemos de pipe0
                                close(pipe1[0,1]);
                                close(pipe0[1]);
                                dup2(pipe0[0], STDIN_FILENO);                   // stdin es ahora tuberia de lectura
                                close(pipe0[0]);
                            } 
                            else {                                              // si el comando es par leemos de pipe1
                                close(pipe0[0,1]);
                                close(pipe1[1]);
                                dup2(pipe1[0], STDIN_FILENO);
                                close(pipe1[0]);
                            }
                        } 
                        else if ((curr_command % 2) != 0) {                     // comando intermedio inpar - tuberia de entrada y salida
                            close(pipe0[1]);
                            close(pipe1[0]);
                            dup2(pipe0[0], STDIN_FILENO);
                            dup2(pipe1[1], STDOUT_FILENO);
                        } 
                        else {                                                  // comando intermedio par - tuberia de entrada y salida
                            close(pipe1[1]);
                            close(pipe0[0]);
                            dup2(pipe1[0], STDIN_FILENO);                       // como es inpar, hay que alternar para completar el bucle
                            close(pipe1[0]);
                            dup2(pipe0[1], STDOUT_FILENO);
                            close(pipe0[1]);
                        }
                        
                    }

                    /* COMANDOS */
                    
                    //se ejecuta mycalc
                    if (strcmp(argvv[curr_command][0], "mycalc") == 0) { 
                        /* execute mycalc */
                        mycalc(argvv, accum);
                        exit(0);
                    }

                    //se ejecuta cualquier comando que no sea interno (siempre que exista)
                    else {
                        getCompleteCommand(argvv, curr_command);
                        execvp(argvv[curr_command][0], argvv[curr_command]);
                        perror("ERROR: en el execvp\n");
                        exit(0);
                    }
                }
			}
		}
	}
	return 0;
}