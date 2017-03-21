/*
 * withPipeandFIFO.c
 *
 *  Created on: Mar 20, 2017
 *      Author: Mahmut
 */

/*-----------------------------------------------------------------------------*/
/*Author        :  M.Ensari Güvengil                                           */
/*Date          :  March 9th 2016                                              */
/*Program Name  :  listdir                                                     */
/*Parameter     :  string dir                                                  */
/*Description   :  search for string in given directory and all subdirectories */
/*-----------------------------------------------------------------------------*/
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>

/*#include <termios.h> */

#define USAGEERR   -99
#define OUTPUT_FILE "log.txt"
#define PATH_MAX 4096
#define FIFO_PERM (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define BUFSIZE 256

/*functions*/
int listdir(char *dirName, char * key, int fifo);
int isdirectory(char *path);
char** alloc_array(int x, int y);
void retrieveFileSize(FILE* inFile, int* row, int* column);
void fillFileArray(FILE* inFile, char ** fileArray, int row, int column);
void fillStringIgnoreWhiteSpace(char * str, char ** file, int row, int column,
		int size, int iCurrent, int jCurrent);
int searchAndPrint(char ** file, int row, int column, char* key, char* filename,
		int filed);
int doWork(char * file, char* key, int filed);
static inline int usage(int argc, char* argv[]);

int main(int argc, char *argv[]) {

	pid_t cpid, w;
	int status;
	int count = 0, pidChild, iFifoFd;
	FILE * output;

	if (usage(argc, argv) == USAGEERR)
		return USAGEERR;
	output = fopen(OUTPUT_FILE, "a");
	if (output == NULL) {
		printf("Could not find %s\n", OUTPUT_FILE);
		exit(1);
	}

	/* Create fifo */
	char tfifo[30];
	sprintf(tfifo, "%ld", (long) getpid());
	mkfifo(tfifo, 666);
////////////////////////////////////////////////////////////////////////////////////////////////
	cpid = fork();
	if (cpid == -1) {
		perror("fork");
		exit(EXIT_FAILURE);
	}

	if (cpid == 0) { /* Code executed by child */
		printf("Child PID is %ld\n", (long) getpid());
		iFifoFd = open(tfifo, O_WRONLY);
		write(iFifoFd, "Aydin", 5);
		//count = listdir(argv[2], argv[1], iFifoFd);
		close(iFifoFd);
		_exit(atoi(argv[1]));

	} else { /* Code executed by parent */


		do {
			w = waitpid(cpid, &status, WUNTRACED | WCONTINUED);
			if (w == -1) {
				perror("wait pid");
				exit(EXIT_FAILURE);
			}

			if (WIFEXITED(status)) {
				printf("exited, status=%d\n", WEXITSTATUS(status));
			} else if (WIFSIGNALED(status)) {
				printf("killed by signal %d\n", WTERMSIG(status));
			} else if (WIFSTOPPED(status)) {
				printf("stopped by signal %d\n", WSTOPSIG(status));
			} else if (WIFCONTINUED(status)) {
				printf("continued\n");
			}
		} while (!WIFEXITED(status) && !WIFSIGNALED(status));
		printf("Parent PID is %ld\n", (long) getpid());
		iFifoFd = open(tfifo, O_RDONLY);
		//char chTemp = '\0';
		printf("%s\n", "Start");
		/*for ( ; ; )
		 {
		 int iBytesRead;
		 iBytesRead = read( iFifoFd, &chTemp, sizeof(char) );


		 if (chTemp == EOF) break;
		 else{
		 if (chTemp != '\n')
		 fprintf(stderr, "%c",chTemp);


		 //printf("%c", chTemp);
		 }
		 }*/
		printf("%s\n", "Finished.");
		close(iFifoFd);

		/* Wait for children */
		unlink(tfifo);
		wait(&status);
		count += status / 256;
		fprintf(output, "%d %s were found in total\n", count, argv[1]);
		exit(EXIT_SUCCESS);
	}
////////////////////////////////////////////////////////////////////////////////////////////////

	fclose(output);
	return 0;
}

/*----------------------------------------------------------------------------*/
/*Fuction Name   : "listdir"                                                  */
/*Parameters     : 1- char* dirName : Current working directory               */
/*Description    : List all file into directory and sub directories           */
/*----------------------------------------------------------------------------*/
int listdir(char *dirName, char * key, int iFifoFd) {
	/* Variables                                                              */
	DIR* directory;
	struct dirent *dirEntry;
	char fullName[PATH_MAX];
	pid_t childPid;
	int status = 0, count = 0;
	int fd[2], ffd;
	FILE *stream, *output;
	int c;
	char myfifo[30];
	char buffer[BUFSIZE];
	/* base case                                                              */
	if ((directory = opendir(dirName)) == NULL) {
		perror("Failed to open directory");
		return -1;
	}

	/* Oldugumuz klasurde dosya oldugu surece  hepsini oku                    */
	while ((dirEntry = readdir(directory)) != NULL) {

		/* Current and parent'leri burada engelliyoruz                        */
		if (!strcmp(dirEntry->d_name, ".") || !strcmp(dirEntry->d_name, "..")) {
			continue;
		}
		/* Yeni klasurun path'ini elde etmek icin                             */
		sprintf(fullName, "%s/%s", dirName, dirEntry->d_name);
		/* Eger okunan dirEntry bir klasor ise recursive cagri ile onu acariz */

		if (isdirectory(fullName)) {
			sprintf(myfifo, "%ld", (long) getppid());
			if (mkfifo(myfifo, FIFO_PERM) == -1) { /* create a named pipe */
				if (errno != EEXIST) {
					fprintf(stderr, "[%ld]:failed to create named pipe: %s\n",
							(long) getpid(), strerror(errno));
					return 1;
				}
			}
		} else {
			if (pipe(fd) == -1) {
				perror("Failed to create the pipe");
				return 1;
			}
		}

		childPid = fork();

		if (childPid == -1) {
			perror("Failed to fork");
			closedir(directory);
			return 0;
		}

		if (childPid == 0) {
			int result = 0;
			char buf[BUFSIZE];
			memset(buf, 0, BUFSIZE);
			if (isdirectory(fullName)) {
				closedir(directory);
				result = listdir(fullName, key, ffd);
				ffd = open(myfifo, O_WRONLY);
				if (ffd == -1) {
					fprintf(stderr,
							"[%ld]:failed to open named pipe [%s] for write: %s\n",
							(long) getpid(), myfifo, strerror(errno));
					return 1;
				}
				sprintf(buf, "%d", result);
				write(ffd, buf, strlen(buf) + 1);
			} else {
				close(fd[0]);
				result = doWork(fullName, key, fd[1]);
				closedir(directory);
			}

			exit(result);
		}
		if (childPid > 0) {

			if (!isdirectory(fullName)) {
				output = fopen(OUTPUT_FILE, "a");
				if (output == NULL) {
					printf("Could not find %s\n", OUTPUT_FILE);
					exit(1);
				}

				close(fd[1]);
				char tBuff[100] = "";
				int i = 0;
				stream = fdopen(fd[0], "r");
				while ((c = fgetc(stream)) != EOF) {
					//fprintf(output, "%c", c);
					tBuff[i] = c;
					i++;
				}
				write(ffd, tBuff, strlen(tBuff) + 1);
				tBuff[0] = '\0';
				fclose(stream);
				fclose(output);
				wait(&status);
				count += status / 256;
			} else {
				ffd = open(myfifo, O_RDONLY);
				if (ffd == -1) {
					fprintf(stderr,
							"Parent [%ld]:failed to open named pipe [%s] for write: %s\n",
							(long) getpid(), myfifo, strerror(errno));
					return 1;
				}
				read(ffd, buffer, BUFSIZE);
				//printf("Helalua %s",buffer);
				sscanf(buffer, "%d", &c);
				unlink(myfifo);

				wait(&status);
				count += c;
			}
		}
	}
	/* close directory                                                        */
	closedir(directory);
	return count;
}

/*-------------------------------------------------------------------------------*/
/*Fuction Name   :   "doWork"                                                    */
/*Parameters     :  1- char* file    : name of file                              */
/*                  2- char* key     : key to search                             */
/*                  3- int filed     : file descriptor                           */
/*Description    :  copy file to array and search for the key                    */
/*-------------------------------------------------------------------------------*/
int doWork(char * file, char* key, int filed) {
	FILE* inFile;
	int count;
	int row, column, i;
	char** fileArray;

	inFile = fopen(file, "r");
	if (inFile == NULL) {
		printf("Could not find %s\n", file);
		return 0;
	}

	/*find file size*/
	retrieveFileSize(inFile, &row, &column);

	/*allocate space*/
	fileArray = alloc_array(row, column);

	/*fill array with file*/
	fillFileArray(inFile, fileArray, row, column);

	/*serch for key in the array and print result to file*/
	count = searchAndPrint(fileArray, row, column, key, file, filed);

	/*free the memory allocated*/
	for (i = 0; i < row; i++)
		free(fileArray[i]);
	free(fileArray);

	/*close file*/
	fclose(inFile);
	return count;
}

/*----------------------------------------------------------------------------*/
/*Fuction Name   : "isdirectory"                                              */
/*Parameters     : 1- char* path : path of directory or file                  */
/*Description    : return non-zero if path is directory else return 0         */
/*----------------------------------------------------------------------------*/
int isdirectory(char *path) {
	struct stat statbuf;

	if (stat(path, &statbuf) == -1)
		return 0;
	else
		return S_ISDIR(statbuf.st_mode);
}

/*----------------------------------------------------------------------------*/
/*Fuction Name   : "usage"                                                    */
/*Parameters     : int   argc  : Holds the number of the program parameters   */
/*                 char* argv[]: Holds program parameters as character arrays */
/*Description    : Checks for valid number of command-line arguments          */
/*----------------------------------------------------------------------------*/
static inline int usage(int argc, char* argv[]) {
	if (argc != 3) {
		fprintf(stderr, "Usage: %s <string> <filename>\n", argv[0]);

		return USAGEERR;
	}
	else {
		fprintf(stderr, "%s, %s, %s, \n", argv[0], argv[1], argv[2]);
	}
	return 1;
}

/*----------------------------------------------------------------------------*/
/*Fuction Name   : "retrieveFileSize"                                         */
/*Parameters     : FILE*   inFile: file pointer                               */
/*                 int* row: will keep line number of the file                */
/*                 int* column: will keep the max line length                 */
/*Description    : retrieve row and column size of file                       */
/*----------------------------------------------------------------------------*/
void retrieveFileSize(FILE* inFile, int* row, int* column) {
	int i = 0, line = 0, max = 0;
	char ch;

	rewind(inFile);
	while ((ch = fgetc(inFile)) != EOF) {
		i++;
		if (ch == '\n') {
			line++;
			if (max < i) {
				max = i;
			}
			i = 0;
		}
	}

	*row = line;
	*column = max;

}

/*copy file to array*/
void fillFileArray(FILE* inFile, char ** fileArray, int row, int column) {
	char ch;
	int i, j;

	rewind(inFile);

	for (i = 0; i < row; i++) {
		for (j = 0; j < column; j++) {
			ch = fgetc(inFile);
			if (ch != '\n') {
				fileArray[i][j] = ch;
			} else {
				fileArray[i][j] = '\0';
				j = column;
			}
		}
	}

}

/*allocate space for array*/
char** alloc_array(int x, int y) {
	char **a;
	int i;

	a = (char**) calloc(x, sizeof(char *));
	for (i = 0; i != x; i++) {
		a[i] = calloc(y, sizeof(char));
	}

	return a;
}

/*search for key and print result to screen*/
int searchAndPrint(char ** file, int row, int column, char* key, char* filename,
		int filed) {
	int i, j, count = 0;
	int lenOfKey;
	char *remainPart;
	FILE * stream;

	stream = fdopen(filed, "w");

	/*compare key with array if found print to screen*/
	lenOfKey = strlen(key);
	remainPart = calloc(lenOfKey - 1, sizeof(char));
	for (i = 0; i < row; i++) {
		for (j = 0; j < column; j++) {
			/*first char is matched check for remaining part*/
			if (file[i][j] == key[0]) {
				fillStringIgnoreWhiteSpace(remainPart, file, row, column,
						lenOfKey - 1, i, j);

				if (strncmp(remainPart, key + 1, lenOfKey - 1) == 0) {
					fprintf(stream, "%s: [%d,%d] %s first character is found\n",
							filename, i + 1, j + 1, key);
					count++;
				}
				/*free the allocated space*/
				free(remainPart);
				/*allocate new space*/
				remainPart = calloc(lenOfKey - 1, sizeof(char));

			}
		}
	}
	free(remainPart);
	fclose(stream);

	return count;
}

/*start from the place which caclulated based on given parameter and fill the str*/
void fillStringIgnoreWhiteSpace(char * str, char ** file, int row, int column,
		int size, int iCurrent, int jCurrent) {
	int i, j, index = 0;

	for (i = iCurrent; i < row; i++) {
		for (j = jCurrent + 1; j < column; j++) {

			jCurrent = -1;
			if (file[i][j] != ' ' && file[i][j] != '\t' && file[i][j] != '\n'
					&& file[i][j] != '\0') {
				str[index] = file[i][j];
				index++;

				if (index == size) {
					j = column;
					i = row;
				}
			}
		}
	}
}
