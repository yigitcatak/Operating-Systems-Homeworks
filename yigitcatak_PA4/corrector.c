#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>

#define BUFFER_SIZE 1024

typedef struct db_entry {
    char *gender;
    char *name;
    char *surname;
} db_entry;

void function(const db_entry *DB, const unsigned int linecount, const char *dir_path) {
	char *path = strdup(dir_path), *orig_path = strdup(dir_path), word[BUFFER_SIZE];
	unsigned int position;
	DIR *dir;
    	struct dirent *entry;
    	FILE *fp;
    	
    	if ((dir = opendir(dir_path)) == NULL){
    		printf("opendir error for %s\n", dir_path);
		return;
	}
	
    	while( entry = readdir(dir) ){
    		if ( (strcmp(entry->d_name, ".")==0) || (strcmp(entry->d_name, "..")==0) ) // the readdir has "." before all files in the directory and ".." after all files, skip them
    			continue;
		
		strcat(path, "/");
		strcat(path, entry->d_name);
		
    		if (entry->d_type == DT_DIR) 
    			function(DB, linecount, path);
    			
    		else if ( strstr(entry->d_name, ".txt") && !((strcmp(dir_path,".")==0 && strcmp(entry->d_name, "database.txt")==0))) {
    			if ((fp = fopen(path, "r+")) == NULL ) {
    				printf("error reading %s.\n", path);
    				return;
    			}
    			
    			while(fscanf(fp, "%s", word) != EOF) {
    				for (unsigned int i=0; i<linecount; i++) {
    					if(strcmp(word, DB[i].name)==0) {
    						position = ftell(fp);
    						fseek(fp, position - strlen(DB[i].name) - 4, SEEK_SET);
    						fputs(DB[i].gender, fp);
    						fseek(fp, position + 1, SEEK_SET);
    						fputs(DB[i].surname, fp);
    						break;					
    					}
    				}
    			}  			
    			fclose(fp);    			
    			
    		}
    		strcpy(path, orig_path); // retrieve the original path, after returning from recursive call the value of path in the caller is corrupted
    	}
    	closedir(dir);
}

db_entry * readDataBase(unsigned int *linecount) {
	char line[BUFFER_SIZE], *gender, *name, *surname;
	unsigned int itr=0;
	FILE *fp;
	
	if ((fp = fopen("database.txt", "r")) == NULL) {
		printf("Error reading \"database.txt\".\n");
		exit(1);
	}
	
	while(fgets(line, BUFFER_SIZE, fp)) (*linecount)++; // count lines to allocate space for DB
	rewind(fp); // get to the begining of the file
	
	db_entry *DB = malloc((*linecount) * sizeof(db_entry));
	while(fgets(line, BUFFER_SIZE, fp)) {
		gender = strtok(line, " \n");
        	name = strtok(NULL, " \n");
        	surname = strtok(NULL, " \n");
        	gender = (strcmp(gender,"m")==0) ? "Mr" : "Ms";
        	
        	db_entry e;
        	e.gender = malloc(2);
        	e.name = malloc(strlen(name));
        	e.surname = malloc(strlen(surname));
        	
        	strcpy(e.gender,gender);
        	strcpy(e.name,name);
        	strcpy(e.surname,surname);
        	DB[itr++] = e;
	}
	fclose(fp);
	return DB;
}

int main(int argc, char *argv[]) {
	unsigned int linecount = 0;
	db_entry *DB = readDataBase(&linecount);
		
	function(DB, linecount, "."); // start at the root directory with path as "." 

	return 0;
}
