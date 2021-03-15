#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <ctype.h>
#include <signal.h>

#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */
#define BUFFER_SIZE 50
#define HIST_COUNT 10

char buffer[BUFFER_SIZE];
char *hist[HIST_COUNT];
int current = 0;
 
/**
 * setup() reads in the next command line, separating it into distinct tokens
 * using whitespace as delimiters. setup() sets the args parameter as a
 * null-terminated string.
 */

void setup(char inputBuffer[], char *args[],int *background)
{
    int length, /* # of characters in the command line */
        i,      /* loop index for accessing inputBuffer array */
        start,  /* index where beginning of next command parameter is */
        ct;     /* index of where to place the next parameter into args[] */
   
    ct = 0;

    /* read what the user enters on the command line */
    length = read(STDIN_FILENO, inputBuffer, MAX_LINE); 

    start = -1;
    if (length == 0)
        exit(0);            /* ^d was entered, end of user command stream */
    if (length < 0){
        perror("error reading the command");
        exit(-1);           /* terminate with error code of -1 */
    }

    /* examine every character in the inputBuffer */
    for (i=0;i<length;i++) {
        switch (inputBuffer[i]){
          case ' ':
          case '\t' :               /* argument separators */
            if(start != -1){
                    args[ct] = &inputBuffer[start];    /* set up pointer */
                ct++;
            }
            inputBuffer[i] = '\0'; /* add a null char; make a C string */
            start = -1;
            break;
          case '\n':                 /* should be the final char examined */
            if (start != -1){
                    args[ct] = &inputBuffer[start];    
                ct++;
            }
                inputBuffer[i] = '\0';
                args[ct] = NULL; /* no more arguments to this command */
            break;
          default :             /* some other character */
            if (start == -1)
                start = i;
            if (inputBuffer[i] == '&'){
                *background  = 1;
                start = -1;
                inputBuffer[i] = '\0';
            }
          }
     }   
     args[ct] = NULL; /* just in case the input line was > 80 */
}

void toLower(char **args[]) 
{
    int i, j;

    printf("\n");

    /* Iterate through each args element (each word) */
    for(i = 1; i <= 4; i++) {
      
      /* Get size of each word */
      int size = strlen(args[i]);

      /* Initialize temporary string varialbe to hold current word */
      char tmp[size];
      strcpy(tmp, args[i]);

      /* Iterate through each character in each word, tolower-ing it and printing it to screen */
      for(j = 0; j < size; j++) {
        tmp[j] = tolower(tmp[j]);
	printf("%c", tmp[j]);
      }
      printf(" ");
    }
    printf("\n");
}

void pExit()
{
	/* Create command string */
	char command[50];

	pid_t pid = getpid();
	char mypid[6];
	
	/* Convert pid_t pid to string */
	sprintf(mypid, "%d", pid);

	/* Build command */
	strcpy( command, "ps -q ");
	strcat(command, mypid);
	strcat(command, " -eo pid,ppid,pcpu,pmem,etime,user,command");

	system(command);
	exit(0);
}

/* The signal handling function */
void handle_SIGQUIT()
{
  write(STDOUT_FILENO, buffer, strlen(buffer));
  history(hist, current);
}

/* The function to handle the history feature */
int history(char *hist[], int current)
{
  int i = current;
  int hist_num = 1;
  
  do {
    if(hist[i]) {
      printf("%4d %s\n", hist_num, hist[i]);
      hist_num++;
    }
  
    i = (i + 1) % HIST_COUNT;

  } while (i != current);

  return 0;
}

int main(void)
{
	char inputBuffer[MAX_LINE];      /* buffer to hold the command entered */
    	int background;              /* equals 1 if a command is followed by '&' */
   	char *args[(MAX_LINE/2)+1];  /* command line (of 80) has max of 40 arguments */
	int promptCount = 1; 	     /* Keep track for I/O purposes */
	int status;		     /* Argument for waitpid() call */
	pid_t pid; 		     /* PID variable for fork() call */
	int i;			     /* Generic iterator for history functionality */

	/* Initialize history data structure */
	for(i = 0; i < HIST_COUNT; i++) {
	  hist[i] = NULL;
	}

	/* set up the signal handler */
	struct sigaction handler;
	handler.sa_handler = handle_SIGQUIT;
	handler.sa_flags = SA_RESTART;
	sigaction(SIGQUIT, &handler, NULL);

	/* Generate the output message */
	strcpy(buffer, "Caught Control-backslash\n");

	printf("Welcome to cwshell. My PID is %d.\n\n", getpid());

   	while (1){            /* Program terminates normally inside setup */
	background = 0;
	printf("cwshell[%d]: ", promptCount);
	fflush(stdout);		/* For clean I/O */
      	setup(inputBuffer,args,&background);       /* get next command */
	promptCount++;

	free(hist[current]);
	
	/* Add latest command to history */
	hist[current] = strdup(inputBuffer);

     	/* the steps are:
      	(0) if built-in command, handle internally
      	(1) if not, fork a child process using fork()
       	(2) the child process will invoke execvp()
       	(3) if background == 0, the parent will wait,
		otherwise returns to the setup() function. */

	  /* Whisper command */
	  if(strcmp(args[0], "whisper") == 0) {; 
	    toLower(args);
	  }
	  /* Exit command */
	  else if(strcmp(args[0], "exit") == 0) {
	    pExit();
	  }
          /* History command */
	  else if(strcmp(args[0], "r" ) == 0) {
	    /* Check if just repeating latest command */
	    if(!isdigit(args[0][2])) {
	      char command[50];

	      strcpy(command, hist[current - 1]);
	      system(command);
	    }
	    else {
	      char command[50];
	      char tmpArg = args[0][2];
	      int tmp = tmpArg - '0';

	      strcpy(command, hist[tmp - 1]);
	      system(command);
	    }
	  }
	  /* Fork() function for every other command */
          else {
	    pid = fork();

	    if (pid < 0) { /* error occurred */
		fprintf(stderr, "Fork Failed\n");			
		return 1;
	    }
	    else if(pid == 0) { /* child process */
		execvp(args[0], args);
	    }	
	    else {
		printf("[Child pid = %d, background = %d]\n", pid, background);
		if(background == 0) {
		pid = waitpid(pid, &status, WUNTRACED | WCONTINUED);
		printf("child complete\n");
		}
	   }
    	 }

	current = (current + 1) % HIST_COUNT; 
       }
}
