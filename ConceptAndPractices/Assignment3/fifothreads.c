#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
/* Including syslog.h in order to be able to call openlog and syslog
 * functions */
#include <syslog.h>
/* Including sys/utsname.h to be able to retrieve the system 
 * information through the function uname() */
#include <sys/utsname.h>

#include <sys/types.h>
#include <sched.h>
#include <unistd.h>

// Specified number of threads for this assignment: 128
#define NUM_THREADS 128

// Using a #define statment to store the value of the syslog opening info
#define COURSE_ID_STRING "[COURSE:1][ASSIGNMENT:3]"
  

// Structure required by pthread_create
typedef struct
{
  int threadIdx;
} threadParams_t;


// POSIX thread declarations and scheduling attributes
//
pthread_t threads[NUM_THREADS];
pthread_t startthread;
threadParams_t threadParams[NUM_THREADS];

pthread_attr_t fifo_sched_attr;
struct sched_param fifo_param;

#define SCHED_POLICY SCHED_FIFO

void print_scheduling_policy(void){

  int schedType = sched_getscheduler(getpid());
  switch(schedType){
    case SCHED_FIFO:
      printf("Using SCHED_FIFO policy\n");
      break;
    case SCHED_OTHER:
      printf("Using SCHED_OTHER policy\n");
      break;
    case SCHED_RR:
      printf("Using SCHED_RR policy\n");
      break;
    default:
      printf("Using UNKNOWN policy (%d)\n", schedType);
      break;
  }

}

void set_scheduler(void){

  int max_prio, cpuidx;
  cpu_set_t cpuset;
  
  printf("INIITAL SETTINGS: ");
  print_scheduling_policy();

  // Initialises the fifo_sched_attr variable to default settings
  pthread_attr_init(&fifo_sched_attr);
  
  /* The inherit-scheduler attribute determines whether a thread created using the
   * thread attributes object attr will inherit its scheduling
   * attributes from the calling thread or whether it will take them
   * from attr.
   * 
   * When using PTHREAD_EXPLICIT_SCHED, threads that are created using 
   * attr take their scheduling attributes from the values specified 
   * by the attributes object */
  pthread_attr_setinheritsched(&fifo_sched_attr, PTHREAD_EXPLICIT_SCHED);
  
  // Sets the scheduling policy to SCHED_FIFO
  pthread_attr_setschedpolicy(&fifo_sched_attr, SCHED_POLICY);
  
  CPU_ZERO(&cpuset); // Clears the cpuset variables, so that it contains no CPU
  cpuidx=(3);
  CPU_SET(cpuidx, &cpuset); // Set the CPU set to the indicated cpuidx
  
  // Uses the cpuset to set the thread affinity attribute to the predefined core
  pthread_attr_setaffinity_np(&fifo_sched_attr, sizeof(cpu_set_t), &cpuset);
  
  // returns  the  maximum  priority value that can be used with the scheduling algorithm
  max_prio = sched_get_priority_max(SCHED_POLICY);
  
  // sets the priority to the max_prio value got at the line before
  fifo_param.sched_priority = max_prio;
  
  /*  sets both the scheduling policy and parameters for the thread whose
   *  ID is specified in pid */ 
  if(sched_setscheduler(getpid(), SCHED_POLICY, &fifo_param) < 0){
    perror("sched_setscheduler");
  }
  
  /* sets the scheduling  parameter  attributes  of  the  thread  attributes  
   * object referred to by attr to the values specified in the buffer pointed to by param */
  pthread_attr_setschedparam(&fifo_sched_attr, &fifo_param);
  
  printf("NEW SETTINGS: ");
  print_scheduling_policy();
  
}


/* Entry point function for the spawned thread. Will sum the information carried
 * by the threadp structure to provide information about the thread
 * being executed */
void counterThread(void *threadp)
{
    int sum=0, i;
    threadParams_t *threadParams = (threadParams_t *)threadp;

   /* performs sum calculation based on the parameter carried by the
    * input structure threadParams */
   for(i=1; i <(threadParams->threadIdx)+1; i++)
       sum=sum+i;
 
   // Provides syslog message as requested by the assignment
   syslog(LOG_INFO, "Thread idx=%d, sum[0...%d]=%d\n Running on core : %d",
            threadParams->threadIdx,
            threadParams->threadIdx, 
            sum,
            sched_getcpu());
}


/* Entry point for the start thread that will create the remaining threads */
void starterThread(void *threadp)
{
   int i;

   printf("starter thread running on CPU=%d\n", sched_getcpu());

   for(i=0; i < NUM_THREADS; i++)
   {
       threadParams[i].threadIdx=i;

       pthread_create(&threads[i],   // pointer to thread descriptor
                      &fifo_sched_attr,     // use FIFO RT max priority attributes
                      (void *)&counterThread, // thread function entry point
                      (void *)&(threadParams[i]) // parameters to pass in
                     );

   }

   for(i=0;i<NUM_THREADS;i++)
       pthread_join(threads[i], NULL);

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
    // configure our program to log to the syslog file
    initialiseSysLog();

    // Sets the scheduler according to configuraiton
    set_scheduler();

    pthread_create(&startthread,   // pointer to thread descriptor
                  &fifo_sched_attr,     // use FIFO RT max priority attributes
                  (void *)&starterThread, // thread function entry point
                  (void *)0 // parameters to pass in
                 );

    pthread_join(startthread, NULL);
    printf("\nTEST COMPLETE\n");
}
