#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <pthread.h>

// for debug purposes
#define PRINT_DEBUG 0

#define COMMAND_SIZE 350 // assuming a line wont be longer than 140 characters long
#define OPTION_SIZE COMMAND_SIZE/7 // 7 is chosen as the max number of arguments that a command can have
// 1) Command 2) Input 3) Option 4) Optional option string 5) Redirection direction 6) Redirection file 7) Background bool

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void* listen(void* arg){
	// if I choose the buffer size too low (except 1) outputs become ambigious (for example they repeat 3-5 chars in the end and get cut
	// this does not hapen at higher buffer sizes that are greater than the output to be printed 
	// I chose the buffer size as 1 to avoid excessive memory consumption and rather have higher time consumption for reading continuously
	const unsigned int OUTPUT_BUFFER_SIZE = 1; int* in_args = (int*) arg;
	int pipe_r = in_args[0], pipe_w = in_args[1], cpid = in_args[2];
	char buffer[OUTPUT_BUFFER_SIZE]; 
	
#if PRINT_DEBUG
	printf("Thread %ld of process %d has started\n", pthread_self(), cpid);
#endif

	waitpid(cpid, NULL, 0); // printer thread waits for the process that it is assigned to print to end
	close(pipe_w); // after writing has ended, close the write end
	
	pthread_mutex_lock(&lock);
    printf("---- %ld\n", pthread_self());
    
#if PRINT_DEBUG
    printf("I am thread of cpid: %d\n", cpid);
    printf("I am thread, pipe_r: %d\n", pipe_r);
    printf("I am thread, pipe_w: %d\n", pipe_w);
#endif
 
	while(!(read(pipe_r, buffer, OUTPUT_BUFFER_SIZE) < 1)){
		printf("%s", buffer);
	}
	printf("---- %ld\n", pthread_self());
	fflush(stdout);
	pthread_mutex_unlock(&lock);
	
    return NULL;
}

int main(int argc, char *argv[]) {
	FILE* fp;
	char line[COMMAND_SIZE], command_taken=0, input_taken=0, option_taken=0, redirection_taken=0, background_taken=0, redirection_idx;
	char* token; 
	int commandcount=0, p[2], command_idx=0, option_idx=0;
	
	if ((fp = fopen("commands.txt", "r")) == NULL) {
		printf("Failure during opening commands.txt.\n");
		exit(1);
	}
	if (pipe(p) < 0) {
		printf("Pipe failed.\n");
		exit(1);
	}
	
	// read commands.txt line by line, send every line to pipe to process them later
	while(fgets(line, COMMAND_SIZE, fp)){
		write(p[1], line, COMMAND_SIZE);
		commandcount++;
	}
	close(p[1]);
	fclose(fp);
	
	// PARSE
	char ***commands = (char***)malloc(commandcount*sizeof(char**)); //char commands[commandcount][7][OPTION_SIZE];
	int cpid_list[commandcount], pipe_list[commandcount][2], thread_args[commandcount][3]; //int* cpid_list = (int*)malloc(commandcount*sizeof(int));
	pthread_t thread_list[commandcount];

    for (int i = 0; i<commandcount; i++) {
		commands[i] = (char**) malloc(7*sizeof(char*));
		thread_list[i] = -1;
		cpid_list[i] = -1;
		
		for (int j = 0; j < 7; j++) {
			commands[i][j] = (char*)malloc(OPTION_SIZE*sizeof(char));
		}
    }
	
	
	if ((fp = fopen("parse.txt", "w+")) == NULL) {
		printf("Failure during opening parse.txt.\n");
		return 1;
	}
	
	while(!(read(p[0], line, COMMAND_SIZE) < 1)){
		fprintf(fp, "----------\n");
		token = strtok(line, " \n");
		
		while(token != NULL){
			if(!command_taken){
				fprintf(fp, "Command: %s\n", token);
				strcpy(commands[command_idx][option_idx++], token);
				command_taken = 1;
				
			} else {
				if(!background_taken && (strcmp(token, "&")==0)){
					if(!input_taken){
						fprintf(fp, "Inputs:\n");
						input_taken = 1;
					}
					
					if(!option_taken){
						fprintf(fp, "Options:\n");
						option_taken = 1;
					}
			
					if(!redirection_taken){
						fprintf(fp, "Redirection: -\n");
						redirection_taken = 1;
					}
					
					fprintf(fp,"Background Job: y\n");
					strcpy(commands[command_idx][option_idx++], "y");
					background_taken = 1;
					
				} else if(!redirection_taken && ((strcmp(token, "<")==0)||(strcmp(token,">")==0))){
					if(!input_taken){
						fprintf(fp, "Inputs:\n");
						input_taken = 1;
					}
					
					if(!option_taken){
						fprintf(fp, "Options:\n");
						option_taken = 1;
					}
					
					fprintf(fp, "Redirection: %s\n", token);
					strcpy(commands[command_idx][option_idx++], token);
					token = strtok(NULL, " \n"); // take the redirection location
					strcpy(commands[command_idx][option_idx++], token);
					redirection_taken = 1;
					
				} else if (!option_taken && (token[0] == '-')) {
					if(!input_taken){
						fprintf(fp, "Inputs:\n");
						input_taken = 1;
					}
					fprintf(fp, "Options: %s", token);
					strcpy(commands[command_idx][option_idx++], token);
					option_taken = 1;
					
					// take one more argument
					token = strtok(NULL, " \n");
						
					if (token != NULL){
						if (strcmp(token, "&")==0){
							fprintf(fp, "\nRedirection: -\nBackground Job: y\n");
							strcpy(commands[command_idx][option_idx++], "y");
							redirection_taken = 1;
							background_taken = 1;
							
						} else if ((strcmp(token, "<")==0) || (strcmp(token, ">")==0)) { // if it is redirection it is not a part of option
							fprintf(fp, "\nRedirection: %s\n", token);
							strcpy(commands[command_idx][option_idx++], token);
							token = strtok(NULL, " \n"); // take the redirection location
							strcpy(commands[command_idx][option_idx++], token);
							redirection_taken = 1;	
							
						} else if (token != NULL) {
							fprintf(fp, " %s\n", token); // else it is the input of the option, append it to the option		
							strcpy(commands[command_idx][option_idx++], token);
						}
					} else {
						fprintf(fp, "\n"); 
					}
				} else if (!input_taken) {
					fprintf(fp, "Inputs: %s\n", token);
					strcpy(commands[command_idx][option_idx++], token);
					input_taken = 1;
				}
			}
			token = strtok(NULL, " \n");
		}
		if(!input_taken)
			fprintf(fp, "Inputs:\n");
		if (!option_taken)
			fprintf(fp, "Options:\n");
		if(!redirection_taken)
			fprintf(fp, "Redirection: -\n");
		if(!background_taken){
			fprintf(fp, "Background Job: n\n");
			strcpy(commands[command_idx][option_idx++], "n");
		}
		fprintf(fp, "----------\n");
		
		command_taken = input_taken = option_taken = redirection_taken = background_taken = 0;
		command_idx++;
		option_idx = 0;
	}
	fclose(fp);
	close(p[0]);
	
	// FETCH & EXECUTE
	for(int i=0; i<commandcount; i++){
		redirection_taken = background_taken = 0;
	
#if PRINT_DEBUG
		printf("\n**********\nI AM PARENT, COMMAND %d IS:\n", i);
#endif
		// FETCHING
		for(int j=0; j<7; j++){
#if PRINT_DEBUG
			printf("%s ", commands[i][j]);
#endif
			if (strcmp(commands[i][j],">")==0){
				commands[i][j] = NULL; // end the argument list for exec here
				redirection_taken = 1;
				redirection_idx = j+1;	// next option is the redirection address
						
			} else if (strcmp(commands[i][j],"<")==0){
				commands[i][j] = NULL; // end the argument list for exec here
				redirection_taken = 2;
				redirection_idx = j+1; // next option is the redirection address
				
			} else if (strcmp(commands[i][j],"y")==0){
				commands[i][j] = NULL; // end the argument list for exec here, if there is redirection exec will take that NULL instead
				background_taken = 1;
				break;
				
			} else if (strcmp(commands[i][j],"n")==0){
				commands[i][j] = NULL; // end the argument list for exec here, if there is redirection exec will take that NULL instead
				break;
			}
		}
#if PRINT_DEBUG
		printf("\n**********\n");
#endif
		// EXECUTION
		if (strcmp(commands[i][0],"wait")==0){
			for (int k=0; k<i; k++){ // k < i instead of k < commandcount since only command up to i is fetched and executed at this poi
				
				printf("%c",'\0'); // CRUCIAL!!!! IF I REMOVE THIS, OUTPUT OF THE THREAD AFTER THE WAIT COMMAND GETS CORRUPTED !!!!
				// REALIZED THIS AS WHEN I SET "PRINT_DEBUG" FLAG TO 1, THE CORRUPTION DID NOT OCCUR AS SOMETHING WAS BEING PRINTED IN BETWEEN
				// SO I DECIDED TO PRINT A INVISIBLE CHARACTER THAT WILL DO THE JOB
				
				if(thread_list[k] != -1){
#if PRINT_DEBUG	
					printf("WAIT PROCESS, Waiting for THREAD %ld\n", thread_list[k]);
#endif
					pthread_join(thread_list[k], NULL);
					thread_list[k] = -1; // revert it to default so it is not waited again in the end unnecessarily
				}
					
				else if(cpid_list[k] != -1){
#if PRINT_DEBUG
					printf("WAIT PROCESS, Waiting for PROCESS %d\n", cpid_list[k]);
#endif
					waitpid(cpid_list[k], NULL, 0); // thread alread waits for its assigned process, wait process directly if it does not have a printer thread (outpur redirection process) 
					cpid_list[k] = -1; // revert it to default so it is not waited again in the end unnecessarily
				}
				
			}
#if WAIT_DEBUG
			//printf("WAIT PROCESS HAS ENDED, We were at COMMAND: %d\n", i);
#endif
		} else if (redirection_taken == 1) { // Output redirection
			int rc;
			if ((rc=fork()) < 0) {
				printf("First fork failed.\n");
				exit(1);
			}
			if (rc == 0) {		
				close(STDOUT_FILENO); // close the console output
				open(commands[i][redirection_idx], O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU); // open file instead
				execvp(commands[i][0], commands[i]);				
			} else {
				if (!background_taken)
					waitpid(rc, NULL, 0); // shell (parent) waits for command to end
				else 
					cpid_list[i] = rc; // add process to waitlist
			}
			
		} else {// No output redirection
			int rc2;
						
			if (pipe(pipe_list[i]) < 0) {
				printf("Redirection pipe failed.\n");
				exit(1);
			}
			if ((rc2=fork()) < 0) {
				printf("Second fork failed.\n");
				exit(1);
			}
			
			if (rc2 == 0) {		
				if (redirection_taken == 2) { // Input redirection
					int fd = open(commands[i][redirection_idx], O_RDONLY);
					dup2(fd,STDIN_FILENO); // input is from file
				}
				dup2(pipe_list[i][1], STDOUT_FILENO); // output is to pipe
				execvp(commands[i][0], commands[i]);
			} else {
				
				thread_args[i][0] = pipe_list[i][0];
				thread_args[i][1] = pipe_list[i][1];
				thread_args[i][2] = rc2;
				
#if PRINT_DEBUG
				printf("COMMAND %d, cpid: %d\n", i, rc2);
				printf("COMMAND %d, pipe_r: %d\n", i, pipe_list[i][0]);
				printf("COMMAND %d, pipe_w: %d\n", i, pipe_list[i][1]);
#endif
				pthread_t listener_thread;
				pthread_create(&listener_thread, NULL, listen, (void*) thread_args[i]);
				
				if(!background_taken){
					waitpid(rc2, NULL, 0); // shell (parent) waits for command to end
				}
				thread_list[i] = listener_thread; // add thread to waitlist eitherway since we need to wait for its print to end before exitting
			}
		}
	}
	
	for (int i=0; i<commandcount; i++){
		if(thread_list[i] != -1){
#if PRINT_DEBUG
			printf("IN THE END, Waiting for THREAD %ld\n", thread_list[i]);
#endif
			pthread_join(thread_list[i], NULL);
		}
		if(cpid_list[i] != -1){
#if PRINT_DEBUG
			printf("IN THE END, Waiting for PROCESS %d\n", cpid_list[i]);
#endif
			waitpid(cpid_list[i], NULL, 0); // thread alread waits for its assigned process, wait process directly if it does not have a printer thread (outpur redirection process) 
		}
	}
	return 0;
}
