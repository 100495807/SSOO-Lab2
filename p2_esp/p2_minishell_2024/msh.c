//---------------------------------------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------------------------------------

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
//
int contador = 0;

/* myhistory */
int accum = 0;
void mycalc(char ***argvv) {
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
                sprintf(buf, "[OK] %d + %d = %d; Acc %d\n", op1, op2, result, accum);
            } 
            else if (strcmp(operator, "mul") == 0) {
                // Multiplicación: multiplicar operandos
                result = op1 * op2;
                // Crear el mensaje de salida
                sprintf(buf, "[OK] %d * %d = %d\n", op1, op2, result);
            } 
            else if (strcmp(operator, "div") == 0) {
                // División: calcular cociente y resto
                int quotient = op1 / op2;
                int remainder = op1 % op2;
                // Crear el mensaje de salida
                sprintf(buf, "[OK] %d / %d = %d; Resto %d\n", op1, op2, quotient, remainder);
            } 
            else {
                // Operador no válido
                // Crear el mensaje de salida de error
                sprintf(buf, "[ERROR] La estructura del comando es mycalc <operando 1> <add/mul/div> <operando 2>\n");
            }

            // Escribir el mensaje en la salida estándar de error
            if (write(2, buf, strlen(buf)) < strlen(buf)) {
                perror("Error in write\n");
            }
        } 
        else {
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

void myhistory(char ***argvv) {
    if ((argvv)[0][1] == NULL) {
        // Mostrar la lista de los últimos 20 comandos introducidos
        for (int i = 0; i < history_size; i++) {
            fprintf(stderr, "%d %s\n", i, history[i].argvv[0][0]);
        }
    } else {
        int command_index = atoi((argvv)[0][1]);
        if (command_index >= 0 && command_index < history_size) {
            // Ejecutar el comando correspondiente
            fprintf(stderr, "Ejecutando el comando %d\n", command_index);
            // Aquí puedes usar execvp() o cualquier otra función según la lógica de tu minishell
        } else {
            // Mostrar un mensaje de error si el comando no se encuentra en el historial
            fprintf(stderr, "ERROR: Comando no encontrado\n");
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


		/************************ STUDENTS CODE ********************************/        
        int comando_act = -1; // Identifica qué hijo está a cargo de qué comando
        if (command_counter > 0) {
            if (command_counter > MAX_COMMANDS){
                printf("Error: El número máximo de comandos es %d \n", MAX_COMMANDS);
            }
            else {
                if (strcmp(argvv[0][0],"myhistory") != 0){
                    store_command(argvv,filev,in_background,&history[contador]);
                    contador++;
                }
                if (strcmp(argvv[0][0], "mycalc") == 0){
                        /* ejecutar mycalc */
                        mycalc(argvv);
                        continue;
                } 
                else if (strcmp(argvv[0][0],"myhistory")==0) {    
                    // Manejar el comando "myhistory"
                    int i; // Variable para el contador en los bucles
                    if (argvv[0][1] == NULL) {
                        // Mostrar el historial completo
                        if (n_elem > history_size) {
                            head = n_elem - 20;
                        }
                        for (int i = 0; i < history_size; i++) {
                            struct command cmd = history[i];
                            if (cmd.num_commands > 0) {
                                fprintf(stderr, "%d ", i);
                                for (int j = 0; j < cmd.num_commands; j++) {
                                    n_elem = n_elem + 1;
                                    for (int k = 0; k < cmd.args[j]; k++) {
                                        fprintf(stderr, "%s ", cmd.argvv[j][k]);
                                    }
                                    if (j < cmd.num_commands - 1) {
                                        fprintf(stderr, "| ");
                                    }
                                }
                                fprintf(stderr, "\n");
                            }
                        }
                        continue;
                    } 
                    else {
                        // Mostrar un comando específico del historial
                        int index = atoi(argvv[0][1]) - 1;
                        store_command(argvv, filev, in_background, &history[contador]);
                        if (index + 1 < 0 || index >= history_size || index > n_elem) {
                            fprintf(stdout, "ERROR: Comando no encontrado\n");
                            continue; // Ir al siguiente bucle
                        } 
                        else {
                            fprintf(stderr, "Ejecutando el comando %d\n", index + 1);
                            struct command *cmd = &history[index + 1];
                            int pid;
                            pid = fork();
                            if (pid < 0) {
                                fprintf(stdout, "Error en fork\n");
                                exit(-1);
                            } else if (pid == 0) {
                                // Proceso hijo
                                char **argv = cmd->argvv[0];
                                execvp(argv[0], argv);
                                fprintf(stdout, "Error en el hijo\n");
                                exit(-1);
                            } else {
                                // Proceso padre
                                int status;
                                waitpid(pid, &status, 0);
                            }
                        }
                        continue;
                    }
                }
                
                else{
                    // PIPES
                /* Configuración de tuberías */
                int pipe0[2], pipe1[2];
                pipe(pipe0);
                pipe(pipe1);

                /* Crear procesos hijos */
                int pid = 1;
                for (int i = 0; i < command_counter; i++){
                    if (pid != 0){ // Solo el padre puede crear hijos
                        if ((pid = fork()) == 0){ /* hijo */
                            comando_act = i;
                        }
                    } 
                    else {
                        break;
                    }
                }

                if (pid == -1){ /* error */
                    perror("Error en fork");
                    return -1;
                } 
                else if (pid != 0){ /* proceso padre */

                    /* Cerrar tuberías */
                    close(pipe1[0]); 
                    close(pipe1[1]);
                    close(pipe0[0]); 
                    close(pipe0[1]);

                    /* EN SEGUNDO PLANO */
                    if (in_background != 1){
                        while (wait(&status) != pid){ // Esperar a que los hijos terminen
                            if (status != 0){
                                perror("Error ejecutando el hijo");
                            } 
                        }
                    }
                } 
                else { /* proceso hijo */
                        
                    /* REDIRECCIÓN */
                    if ((comando_act == 0) && (filev[0][0] != '0')){
                        /* Redirigir desde la entrada, file[0] como stdin */
                        close(STDIN_FILENO); // Liberar el descriptor de archivo 0
                        int fd = open(filev[0], O_RDONLY); // fd ahora es 0
                    } 
                    else if ((comando_act == command_counter - 1) && (filev[1][0] != '0')){
                        /* Redirigir hacia la salida, file[1] como stdout */
                        close(STDOUT_FILENO);
                        int fd = open(filev[1], O_CREAT | O_RDWR, S_IRWXU);                          
                    }
                    if (filev[2][0] != '0'){
                        /* Redirigir error, file[1] como stderr */
                        close(STDERR_FILENO);
                        int fd = open(filev[2], O_CREAT | O_RDWR, S_IRWXU);                          
                    }

                    /* TUBERÍAS */
                    if (command_counter > 1){
                        if (comando_act == 0){ /* primer comando - salida de pipe0 */
                            close(pipe1[0]); 
                            close(pipe1[1]);
                            close(pipe0[0]);
                            dup2(pipe0[1], STDOUT_FILENO); // stdout es ahora la escritura de la tubería
                            close(pipe0[1]);
                        } 
                        else if (comando_act == command_counter - 1){ /* último comando - entrada de pipe */
                            if ((comando_act % 2) != 0){ // impar - lee desde pipe0
                                close(pipe1[0]); 
                                close(pipe1[1]);
                                close(pipe0[1]);
                                dup2(pipe0[0], STDIN_FILENO); // stdin es ahora la lectura de la tubería
                                close(pipe0[0]);
                            } 
                            else{ /* par - entrada de pipe1 */
                                close(pipe0[0]); 
                                close(pipe0[1]);
                                close(pipe1[1]);
                                dup2(pipe1[0], STDIN_FILENO);
                                close(pipe1[0]);
                            }
                        } 
                        else if ((comando_act % 2) != 0){ /* comando intermedio, impar - entrada/salida de tubería */
                            close(pipe0[1]);
                            close(pipe1[0]);
                            dup2(pipe0[0], STDIN_FILENO);
                            dup2(pipe1[1], STDOUT_FILENO);
                        } 
                        else{ /* comando intermedio, par - entrada/salida de tubería */
                            close(pipe1[1]);
                            close(pipe0[0]);
                            dup2(pipe1[0], STDIN_FILENO); // como es par, tenemos que alternar para completar el bucle
                            close(pipe1[0]);
                            dup2(pipe0[1], STDOUT_FILENO);
                            close(pipe0[1]);
                        }
                    }

                    /* EJECUTAR EL COMANDO ACTUAL */
                    /* COMANDOS INTERNOS */
                if (strcmp(argvv[0][0],"myhistory")!=0 || strcmp(argvv[0][0],"mycalc")!=0 ){
                    getCompleteCommand(argvv, comando_act);
                    execvp(argvv[comando_act][0], argvv[comando_act]); // ejecutar el comando
                    perror("Error en execvp\n");
                    exit(0);
                }
                }
                }
            }
        }

    }
    return 0;
}