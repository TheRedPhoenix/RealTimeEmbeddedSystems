#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <sched.h>
/* Including syslog.h in order to be able to call openlog and syslog
 * functions */
#include <syslog.h>
/* Including sys/utsname.h to be able to retrieve the system 
 * information through the function uname() */
#include <sys/utsname.h>

// Specified number of threads for this assignment: 128
#define NUM_THREADS 128

// Structure required by pthread_create
typedef struct
{
    int threadIdx;
} threadParams_t;


// POSIX thread declarations and scheduling attributes
//
pthread_t threads[NUM_THREADS];
threadParams_t threadParams[NUM_THREADS];

// Using a #define statment to store the value of the syslog opening info
#define COURSE_ID_STRING "[COURSE:1][ASSIGNMENT:2]"

/* Entry point function for the spawned thread. Will sum the information carried
 * by the threadp structure to provide information about the thread
 * being executed */
void counterThread(void *threadp)
{
    int sum=0, i;
    threadParams_t *threadParams = (threadParams_t *)threadp;

   /* performs sum calculation based on the parameter carried by the
    * input structure threadParams */
   for(i=1; i < (threadParams->threadIdx)+1; i++)
       sum=sum+i;
 
   // Provides syslog message as requested by the assignment
   syslog(LOG_INFO, "Thread idx=%d, sum[0...%d]=%d\n",
            threadParams->threadIdx,
            threadParams->threadIdx, sum);
}

void initialiseSysLog() {
  
    /* First, we want to collect the system information ,to be provided
     * as the first line of our log file */
    
    /* the systemInfo variable will serve as container to hold the system 
     * information */
     
    struct utsname systemInfo;
    
    /* The function uname fills the systemInfo structure with values
     * to be used to construct a string equivalent to the command
     * uname -a 
     * It will return 0 to state that there was no error in retrieving the 
     * info, -1 otherwise. Therefore, if error_number < 0, the program 
     * will exit and print an error message*/
    int error_number = uname(&systemInfo);
    
    if (error_number < 0) {
        perror("Error executing uname() function");
        exit(EXIT_FAILURE);
    }

    /* Opens a chanlle to write to the syslog file, located at 
     * /var/log/syslog */
     
    openlog(COURSE_ID_STRING, /* the opening statement of any syslog 
                               * call from this instance */
            LOG_PERROR,       /* Prints the message both to syslog and 
                               * to stderr (the console where the 
                               * program is executed */
            LOG_USER          /* Determines that the messages written 
                               * to the syslog file through this calls
                               * are user originated */
            );
    
    /* Logs the first message to the syslog, equivalent 
     * to the uname -a */
    syslog(LOG_INFO,                          // level of the logging message 
          "%s %s %s %s %s GNU/Linux",         // string to log
          (const char *)&systemInfo.sysname,  // system name (from uname)
          (const char *)&systemInfo.nodename, // node name (from uname)
          (const char *)&systemInfo.release,  // sw release (from uname)
          (const char *)&systemInfo.version,  // sw version (from uname)
          (const char *)&systemInfo.machine   // machine architecture (from uname)
          );
 
}


int main (int argc, char *argv[])
{
    int index; // The index of our for loop

    // configure our program to log to the syslog file
    initialiseSysLog();

    /* Loop over the 128 items of the threadParams array to spawn the
     * respective thread and associate to it the entry point function
     * counterThread */
    for(index=0; index < NUM_THREADS; index++)
    {
      // Initialize the threadParams structure with the index of the current thread
      threadParams[index].threadIdx=index;

      // Creates the thread
      pthread_create(&threads[index],     // pointer to thread descriptor
                    (void *)0,            // use default attributes
                    (void *)&counterThread,        // thread function entry point
                    (void *)&(threadParams[index]) // parameters to pass in
                   );
    }

    // Join all the NUM_THREAD (128 in this case) before exiting the program
    for(index=0;index<NUM_THREADS;index++)
       pthread_join(threads[index], NULL);
}
