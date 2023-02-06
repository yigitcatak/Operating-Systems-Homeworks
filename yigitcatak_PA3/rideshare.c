#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>

unsigned int waiting_a=0, waiting_b=0, riders_left=0;
sem_t mutex, allow_a, allow_b, allow_newcomers;

void* Fan(void* arg){
	char* team = arg;
	char ride_found = 0, waiting = 0, ride_initiated;
	
	// I HAVE PREVENTED INIT AND NEW RIDE LOOKING MESSAGES FOR A MORE CLEAR OUTPUT WITH 
	// ALLOW NEWCOMERS MUTEX EVEN IF IT WAS NOT NECESSARY. IF A RIDE IS INITIATED BETWEEN
	// 4 FANS, NEW FANS CAN NOT SEARCH FOR A RIDE OR FORM A RIDE UNTIL THE INITIATED RIDE
	// COMPLETES ITS PRINT STATEMENTS.
	
	
	
	// PUTTING THE INIT LINE 32-34 INSTEAD OF LINE 41 ALLOWS MORE RANDOMIZED OUTPUT AND A BETTER SIMULATION IMO.
	// LEAVING THE INIT HERE, A FAN CAN STILL JOIN BEFORE A RIDE IS FORMED DUE TO CONTEXT SWITCH EVEN IF A VALID RIDE WAS AVAILABLE.
	// THIS DOES NOT CAUSE A MESSY OUTPUT AND CLARIFICATION ISSUES, AND THE FLOW IS NOT WRONG.
	
	// YET, I AM CHOSING TO DO OTHERWISE. DOING SO, AFTER A NEWCOMER JOINS, THE RIDE IS CHECKED BY THE NEWCOMMER IMMEDIATELY AND 
	// INITIADED IF POSSIBLE WITHOUT ALLOWING ANY OTHER NEWCOMER TO INTERLEAVE.
	
	/*
	sem_wait(&allow_newcomers);
	printf("Thread ID: %ld, Team: %s, I am looking for a car\n", pthread_self(), team);
	sem_post(&allow_newcomers);
	*/
	
	while (!ride_found) {
		if (!waiting) {
			sem_wait(&allow_newcomers);
			sem_wait(&mutex);
			printf("Thread ID: %ld, Team: %s, I am looking for a car\n", pthread_self(), team);
			if (*team == 'A'){
				if (waiting_a >= 3){ // if there are more than 3 FAN_A is waiting and you are a FAN_A form a 4 FAN_A ride
					riders_left = 4;
					ride_initiated = 1;
					sem_post(&allow_a);
					sem_post(&allow_a);
					sem_post(&allow_a);
					sem_post(&allow_a);
				} else if (waiting_b >= 2 && waiting_a >= 1) { // if there are more than 2 FAN_B waiting and 1 FAN_A waiting and you are a FAN_A form a 2 FAN_A 2 FAN_B ride
					riders_left = 4;
					ride_initiated = 1;
					sem_post(&allow_a);
					sem_post(&allow_a);
					sem_post(&allow_b);
					sem_post(&allow_b);
				} else {
					ride_initiated = 0;
				}
				waiting_a++;
				
			} else {
				if (waiting_b >= 3){ // if there are more than 3 FAN_B is waiting and you are a FAN_B form a 4 FAN_B ride
					riders_left = 4;
					ride_initiated = 1;
					sem_post(&allow_b);
					sem_post(&allow_b);
					sem_post(&allow_b);
					sem_post(&allow_b);
				} else if (waiting_a >= 2 && waiting_b >= 1) { // if there are more than 2 FAN_B waiting and 1 FAN_A waiting and you are a FAN_A form a 2 FAN_A 2 FAN_B ride
					riders_left = 4;
					ride_initiated = 1;
					sem_post(&allow_a);
					sem_post(&allow_a);
					sem_post(&allow_b);
					sem_post(&allow_b);
				} else {
					ride_initiated = 0;
				}
				waiting_b++;
			}
			waiting = 1;
			sem_post(&mutex);
			if (!ride_initiated)
				sem_post(&allow_newcomers);

		}
		if (waiting) {
			if (*team == 'A'){
				sem_wait(&allow_a);
				sem_wait(&mutex);
				printf("Thread ID: %ld, Team: %s, I have found a spot in a car\n", pthread_self(), team);
				riders_left--;
				waiting_a--;
				ride_found = 1;
				
			} else {
				sem_wait(&allow_b);
				sem_wait(&mutex);
				printf("Thread ID: %ld, Team: %s, I have found a spot in a car\n", pthread_self(), team);
				riders_left--;
				waiting_b--;
				ride_found = 1;
				
			}
			if (!riders_left) { // last fan of that ride prints the captain line
				printf("Thread ID: %ld, Team: %s, I am the captain and driving the car\n", pthread_self(), team);
				sem_post(&mutex);
				sem_post(&allow_newcomers);
			} else {
				sem_post(&mutex);
			}
		}
	}
	return NULL;
}

int main(int argc, char *argv[]) {
	
	if(argc != 3){
		// printf("Enter exactly 2 arguments for team counts!\n");
		printf("The main terminates\n");
		exit(1);
	}
	
	int count_a = atoi(argv[1]), count_b = atoi(argv[2]), count_total = count_a + count_b, i=0;
	
	if ( (count_a%2 != 0) || (count_b%2 != 0) ){
		// printf("Fan counts for both teams must be even!\n");
		printf("The main terminates\n");
		exit(1);
	}
	
	if ( count_total%4 != 0 ){
		// printf("Total fan count must be a multiple of 4!\n");
		printf("The main terminates\n");
		exit(1);
	}
	
	sem_init(&mutex, 0, 1);
	sem_init(&allow_newcomers, 0, 1);
	sem_init(&allow_a, 0, 0); // initially no one can get in a ride, no resource to be given
	sem_init(&allow_b, 0, 0); // initially no one can get in a ride, no resource to be given
	
	pthread_t thread_list[count_total];
	for (; i<count_a; i++){
		pthread_t t;
		pthread_create(&t, NULL, Fan, (void*) "A");
		thread_list[i] = t;
	}
	
	for (; i<count_total; i++){
		pthread_t t;
		pthread_create(&t, NULL, Fan, (void*) "B");
		thread_list[i] = t;
	}
	
	for (i--; i>-1; i--)
		pthread_join(thread_list[i], NULL);
		
	printf("The main terminates\n");
	return 0;
}
