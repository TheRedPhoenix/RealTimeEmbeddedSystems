/****************************************************************************/
/* Function: nanosleep and POSIX 1003.1b RT clock demonstration             */
/*                                                                          */
/* Sam Siewert - 02/05/2011                                                 */
/*                                                                          */
/****************************************************************************/

#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <string.h> // in order to use strlen

/**
 * @brief Number of nanoseconds per second
 */
#define NSEC_PER_SEC (1000000000)
/**
 * @brief Conversion factor between nanoseconds and seconds
 */
#define NSEC_TO_SECOND_FACTOR (double)(1/(double)NSEC_PER_SEC)

/**
 * @brief Number of nanoseconds per millisecond
 */
#define NSEC_PER_MSEC (1000000)
/**
 * @brief Number of nanoseconds per microsecod
 */
#define NSEC_PER_USEC (1000)

#define ERROR (-1)
#define OK (0)

/**
 * @brief Number of seconds for which we're requesting each thread to sleep
 */
#define TEST_SECONDS (0)
/**
 * @brief Number of nanoseconds for which we're requesting each thread to sleep
 */
#define TEST_NANOSECONDS (NSEC_PER_MSEC * 10) // 10 * number of nanoseconds in a millisecond = 10 milliseconds

// declaration of end_delay_test, in order to use it before implementation
void end_delay_test(void);

static struct timespec sleep_time = {0, 0};
static struct timespec sleep_requested = {0, 0};
static struct timespec remaining_time = {0, 0};

static unsigned int sleep_count = 0;

pthread_t main_thread;
pthread_attr_t main_sched_attr;
int rt_max_prio, rt_min_prio, min;
struct sched_param main_param;

FILE* csvFileOutput = NULL;

#define CSV_EXTENSION ".csv"

// Use a compilation switch to determine which clock to use
#ifdef USE_CLOCK_REALTIME
/**
 * @brief A  settable system-wide clock that measures real (i.e., wall-clock) time.  
 * 
 * This clock is affected by discontinuous jumps in the system time  
 * (e.g.,  if the system administrator manually changes the clock)
 */
#define MY_CLOCK CLOCK_REALTIME
#elif USE_CLOCK_MONOTONIC
/**
 * @brief  A nonsettable  system-wide  clock  that  represents  monotonic  time  since—as  described  by
 * POSIX—"some  unspecified  point in the past".  On Linux, that point corresponds to the number 
 * of seconds that the system has been running since it was booted. It is not affected by discontinuous 
 * jumps in the system time (e.g., if the  system administrator manually changes the clock)
 */
#define MY_CLOCK CLOCK_MONOTONIC
#elif USE_CLOCK_REALTIME_COARSE
/**
 * @brief A faster but less precise version of CLOCK_REALTIME.
 */
#define MY_CLOCK CLOCK_REALTIME_COARSE
#elif USE_CLOCK_MONOTONIC_COARSE
/**
 * @brief A faster  but less precise version of CLOCK_MONOTONIC. 
 */
#define MY_CLOCK CLOCK_MONOTONIC_COARSE
/**
 * @brief Similar to CLOCK_MONOTONIC, but provides access to a raw hardware-based time that is not
 * subject to NTP adjustments or the incremental adjustments performed by adjtime(3).  
 * This clock does not count time that the system is suspended.
 */
#elif USE_CLOCK_MONOTONIC_RAW
#define MY_CLOCK CLOCK_MONOTONIC_RAW
#else
// Default my clock to monotonic
#define MY_CLOCK CLOCK_MONOTONIC 
#endif

#define USED_CLOCK "Program compiled to use %s\n"

/**
 * @brief Get the used clock as a string by analysing the input value of type clockid_t
 * 
 * @param clockId the clockid_t to be transformed in string
 * @return const char* a human readable string indicating which clock type corresponds to the input parameter
 */
const char * get_used_clock(clockid_t clockId){
  switch(clockId)
  {
    case CLOCK_REALTIME:
        return("CLOCK_REALTIME");
    case CLOCK_MONOTONIC:
        return("CLOCK_MONOTONIC");
    case CLOCK_REALTIME_COARSE:
        return("CLOCK_REALTIME_COARSE");
    case CLOCK_MONOTONIC_COARSE:
        return("CLOCK_MONOTONIC_COARSE");
    case CLOCK_MONOTONIC_RAW:
        return("CLOCK_MONOTONIC_RAW");
    default:
        return("UNKNOWN");
  }
}

/**
 * @brief Prints out information about clockId based
 * on the preconfigured string USED_CLOCK
 * 
 * @param clockId an instance of clockid_t
 */
void print_used_clock(clockid_t clockId){
  printf(USED_CLOCK, get_used_clock(clockId));
}


/**
 * @brief print_scheduler prints out information about 
 * scheduling policy currently used from the current process
 * 
 * Internally, it calls getpid() and sched_getscheduler() to
 * provide the required information 
 */
void print_scheduler(void)
{
  // getpid() returns the Id of the calling process,
  // whilst sched_getscheduler returns the policy of the
  // thread identified by the provided pid
  int schedulingType = sched_getscheduler(getpid());

  /** 
   * Switch among the possible values of scheudlingType
   * and print the information matching the value
   */
  switch(schedulingType)
  {
    case SCHED_FIFO:
          printf("Pthread Policy is SCHED_FIFO\n");
          break;
    case SCHED_OTHER:
          printf("Pthread Policy is SCHED_OTHER\n");
      break;
    case SCHED_RR:
          printf("Pthread Policy is SCHED_RR\n");
          break;
    default:
          /** 
           * If none of the previous types is matched, print that the policy
           * is unknown
           */
          printf("Pthread Policy is UNKNOWN\n");
          break;
  }

}

/**
 * @brief Converts the time information hold by the timeInfo variable (of type timespec)
 * to a double type holding the value of the seconds and nano seconds combined
 * 
 * @param timeInfo a parameter of type timespec
 * @return double the converted value of the time information
 */
double timespecToDouble(struct timespec *timeInfo){
  // Convert the information of seconds from integer to double (with a simple cast)
  double secondsAsDouble = (double)(timeInfo->tv_sec);  
  /** 
   * Convert the information of nanosecodns to seconds (by multiplying for the conversion factor)
   * and then cast to double)
   */
  double nanoSecondsAsDouble = (double)((timeInfo->tv_nsec) * NSEC_TO_SECOND_FACTOR);
  // Sum the two values obtained to get the time information as double in Seconds
  return (secondsAsDouble + nanoSecondsAsDouble);
}

/**
 * @brief Converts the time information from timespec to double for the two input parameters,
 * and returs the difference of the converted value as a double
 * 
 * @param timeInfo1 Time information 1
 * @param timeInfo2 Time information 2
 * @return double a value obtained by difference between timeInfo2 and timeInfo1, after converting them to double
 */
double d_ftime(struct timespec *timeInfo1, struct timespec *timeInfo2)
{
  // Convert the values hold by the input parameters into double
  double time1AsDouble = timespecToDouble(timeInfo1);
  double time2AsDouble = timespecToDouble(timeInfo2);
  // Subtract the two values and retur the difference
  return(time2AsDouble - time1AsDouble); 
}

/**
 * @brief Calculates the difference between two time structures and stores
 * results in the inout parameter delta_t
 * 
 * @param stopTime pointer to timespec structure. Time information assumed to be more recent
 * @param startTime pointer to timespec structure. Time information assumed to be less recent
 * @param delta_t pointer to timespec structure. Parameter holding return value with time difference
 * @return int OK (0) in case of SUCCESS, ERROR (-1) otherwise
 */
int delta_t(struct timespec *stopTime, struct timespec *startTime, struct timespec *delta_t)
{
  // Calculate a difference between the seconds information hold by stopTime and starTime
  int deltaTime_seconds=stopTime->tv_sec - startTime->tv_sec;
  // Calculate a difference between the nanoseconds information hold by stopTime and starTime
  int deltaTime_nanoSeconds=stopTime->tv_nsec - startTime->tv_nsec;

  //printf("\ndt calcuation\n");

  /** 
   * When handling time information such in this situation, multiple cases may arise.
   * This function handles the following Cases:
   * 
   * I: less than a second occurred between startTime and stopTime
   *  I.1:  the amount of nanoseconds in deltaTime_nanoSeconds is in the interval [0, NSEC_PER_SEC],
   *        so the deltaTime_nanoSeconds holds a value that represents less than 1 second (no rollover required)
   *  I.2:  the amount of nanoseconds in deltaTime_nanoSeconds greater than NSEC_PER_SEC, 
   *        so the deltaTime_nanoSeconds holds a value that represents more than 1 second (rollover required)
   *  I.3:  Error Case: deltaTime_nanoSeconds is negative, meaning that stopTime is more recent
   *        than startTime. Ideally, this is impossible (unless in a corner case when using real system 
   *        time ~ e.g. CLOCK_REALTIME: check provided documentation)
   *        
   * II: more than a second occurred between startTime and stopTime
   *  II.1: the amount of nanoseconds in deltaTime_nanoSeconds is in the interval [0, NSEC_PER_SEC],
   *        so the deltaTime_nanoSeconds holds a value that represents less than 1 second (no rollover required)
   *  II.2: the amount of nanoseconds in deltaTime_nanoSeconds greater than NSEC_PER_SEC, 
   *        so the deltaTime_nanoSeconds holds a value that represents more than 1 second (rollover required)
   *  II.3: the amount of nanoseconds in deltaTime_nanoSeconds is negative, 
   *        so a rollover is required also in this case, but it will decrease the amount of seconds by one

   */

  // I: less than a second occurred between startTime and stopTime
  if(deltaTime_seconds == 0)
  {
    // I.1 the amount of nanoseconds in deltaTime_nanoSeconds is in the interval [0, NSEC_PER_SEC]
	  if(deltaTime_nanoSeconds >= 0 && deltaTime_nanoSeconds < NSEC_PER_SEC)
	  {
      // Simple assignation from deltaTime_*
		  delta_t->tv_sec = deltaTime_seconds; // 0 in this case
		  delta_t->tv_nsec = deltaTime_nanoSeconds;
	  }

    // I.2 the amount of nanoseconds in deltaTime_nanoSeconds greater than NSEC_PER_SEC
	  else if(deltaTime_nanoSeconds > NSEC_PER_SEC)
	  {
      // Perform Rollover
		  delta_t->tv_sec = deltaTime_seconds + 1; // 1 in this case
		  delta_t->tv_nsec = deltaTime_nanoSeconds-NSEC_PER_SEC;
	  }
    // I.3 Error Case: deltaTime_nanoSeconds is negative, meaning that stopTime is more recent than startTime
	  else 
	  {
      printf("stopTime is earlier than startTime\n");
      return(ERROR);  
	  }
  }

  // II more than a second occurred between startTime and stopTime
  else if(deltaTime_seconds > 0)
  {
    // II.1 the amount of nanoseconds in deltaTime_nanoSeconds is in the interval [0, NSEC_PER_SEC]
	  if(deltaTime_nanoSeconds >= 0 && deltaTime_nanoSeconds < NSEC_PER_SEC)
	  {
      // Simple assignation from deltaTime_*
		  delta_t->tv_sec = deltaTime_seconds;
		  delta_t->tv_nsec = deltaTime_nanoSeconds;
	  }

    // II.2 the amount of nanoseconds in deltaTime_nanoSeconds greater than NSEC_PER_SEC
	  else if(deltaTime_nanoSeconds > NSEC_PER_SEC)
	  {
      // Perform Rollover
		  delta_t->tv_sec = delta_t->tv_sec + 1;
		  delta_t->tv_nsec = deltaTime_nanoSeconds-NSEC_PER_SEC;
	  }
   
   //II.3: the amount of nanoseconds in deltaTime_nanoSeconds is negative
	  else 
	  {
	    // perform rollover
		  delta_t->tv_sec = deltaTime_seconds-1;
		  delta_t->tv_nsec = NSEC_PER_SEC + deltaTime_nanoSeconds;
	  }
  }

  return(OK);
}

static struct timespec realTimeClock_dt = {0, 0};
static struct timespec realTimeClock_start_time = {0, 0};
static struct timespec realTimeClock_stop_time = {0, 0};
static struct timespec delay_error = {0, 0};

// A constant value holding the amount of iterations the program shall perform
#define TEST_ITERATIONS (100)

/**
 * @brief Entry point function for the thread to execute
 * 
 * @param threadID pthread parameter of he new thread
 */
void delay_test(void *threadID)
{
  int index, rc;
  unsigned int max_sleep_calls=3;
  struct timespec realTimeClock_resolution;

  sleep_count = 0;

  // Attempts to get the resolution (precision) of the clock_id currently used for the program
  if(clock_getres(MY_CLOCK, &realTimeClock_resolution) == ERROR)
  {
      perror("clock_getres");
      exit(-1);
  }
  else
  {
      printf("\n\nPOSIX Clock demo using system RT clock with resolution:\n\t%ld secs, %ld microsecs, %ld nanosecs\n",
                                                                             realTimeClock_resolution.tv_sec,
                                                                             (realTimeClock_resolution.tv_nsec/NSEC_PER_USEC),
                                                                             realTimeClock_resolution.tv_nsec);
  }

  for(index=0; index < TEST_ITERATIONS; index++)
  {
    printf("test %d\n", index);

    /** 
     * Populate the sleep_time struct with the fixed amount of seconds
     * and nanoseconds for which the program should sleep 
     */
    sleep_time.tv_sec=TEST_SECONDS;
    sleep_time.tv_nsec=TEST_NANOSECONDS;

    /** 
     * Initialise the sleep_requested structure with the 
     * values in sleep_time
     */
    sleep_requested.tv_sec=sleep_time.tv_sec;
    sleep_requested.tv_nsec=sleep_time.tv_nsec;

    /**
     * Get the time of the clock specified by the clockid_t MY_CLOCK and stores
     * it in the var realTimeClock_start_time
     */ 
    clock_gettime(MY_CLOCK, &realTimeClock_start_time);

    /* request sleep time and repeat if time remains */
    do 
    {
      /** 
       * nanosleep() suspends the execution of the calling thread until either at least the time specified in
       * sleep_time has elapsed, or the delivery of a signal that triggers the invocation of a handler in the calling 
       * thread or that terminates the process.
       *  
       * If  the  call  is  interrupted by a signal handler, nanosleep() returns -1, sets errno to EINTR, and
       * writes the remaining time into the structure pointed to by remaining_time unless remaining_time is  NULL.
       * The  value  of remaining_time can then be used to call nanosleep() again and complete the specified pause
       */

      if((rc=nanosleep(&sleep_time, &remaining_time)) == 0) break;
      
      sleep_time.tv_sec = remaining_time.tv_sec;
      sleep_time.tv_nsec = remaining_time.tv_nsec;
      sleep_count++;
    } 
    while (((remaining_time.tv_sec > 0) || (remaining_time.tv_nsec > 0))
        && (sleep_count < max_sleep_calls));

    /**
     * Get the time of the clock specified by the clockid_t MY_CLOCK and stores
     * it in the var realTimeClock_stop_time
     */ 
    clock_gettime(MY_CLOCK, &realTimeClock_stop_time);

    // Calculate the delta time between the stop and start time
    delta_t(&realTimeClock_stop_time, &realTimeClock_start_time, &realTimeClock_dt);
    delta_t(&realTimeClock_dt, &sleep_requested, &delay_error);

    end_delay_test();
  }

}

void end_delay_test(void)
{
    double real_dt;
#if 0
  printf("MY_CLOCK start seconds = %ld, nanoseconds = %ld\n", 
         realTimeClock_start_time.tv_sec, realTimeClock_start_time.tv_nsec);
  
  printf("MY_CLOCK clock stop seconds = %ld, nanoseconds = %ld\n", 
         realTimeClock_stop_time.tv_sec, realTimeClock_stop_time.tv_nsec);
#endif

  // Calculates the difference between start time and stop time of the thread execution and returns it as double
  real_dt=d_ftime(&realTimeClock_start_time, &realTimeClock_stop_time);
  printf("%s clock DT seconds = %ld, msec=%ld, usec=%ld, nsec=%ld, sec=%6.9lf\n", 
        get_used_clock(MY_CLOCK),
        realTimeClock_dt.tv_sec,
        realTimeClock_dt.tv_nsec/NSEC_PER_MSEC,
        realTimeClock_dt.tv_nsec/NSEC_PER_USEC,
        realTimeClock_dt.tv_nsec, real_dt);

#if 0
  printf("Requested sleep seconds = %ld, nanoseconds = %ld\n", 
         sleep_requested.tv_sec, sleep_requested.tv_nsec);

  printf("\n");
  printf("Sleep loop count = %ld\n", sleep_count);
#endif
  printf("%s delay error = %ld, nanoseconds = %ld\n", 
        get_used_clock(MY_CLOCK), 
        delay_error.tv_sec, 
        delay_error.tv_nsec);


  if(csvFileOutput!=NULL){
    // print to csv file
    fprintf(csvFileOutput, "%6.9lf;%6.9lf\n", timespecToDouble(&realTimeClock_dt), timespecToDouble(&delay_error));
  }

}

#define RUN_RT_THREAD

int main(int argc, char *argv[])
{
  // Print the clock used in this execution
  print_used_clock(MY_CLOCK);

  // Print scheduler policy before setting new configuration
  printf("Before adjustments to scheduling policy:\n");
  print_scheduler();

  char csvFileName[strlen(argv[0]) + strlen(CSV_EXTENSION)];

  strcpy(csvFileName, argv[0]);
  strcat(csvFileName, CSV_EXTENSION);

  // Attempt to open a CSV file for logging 
  csvFileOutput = fopen(csvFileName, "w");

  if(csvFileOutput != NULL){
    fprintf(csvFileOutput, "Clock Time;Delay Error\n");
  }

#ifdef RUN_RT_THREAD

  /* Configure main_sched_attr to hold the configuration to run our program with SCHED_FIFO */
  pthread_attr_init(&main_sched_attr);
  pthread_attr_setinheritsched(&main_sched_attr, PTHREAD_EXPLICIT_SCHED);
  pthread_attr_setschedpolicy(&main_sched_attr, SCHED_FIFO);

  rt_max_prio = sched_get_priority_max(SCHED_FIFO);
  rt_min_prio = sched_get_priority_min(SCHED_FIFO);

  main_param.sched_priority = rt_max_prio;
  int rc=sched_setscheduler(getpid(), SCHED_FIFO, &main_param);
  // sched_setscheduler returns 0 on success. So, if rc is != 0, print error and exit
  if (rc) {
      printf("ERROR; sched_setscheduler rc is %d\n", rc);
      perror("sched_setschduler"); \
      exit(-1);
  }
  /* Configuration successfully completed */

  // Print scheduler policy after setting new configuration
  printf("After adjustments to scheduling policy:\n");
  print_scheduler();

  // Configure main_param in order to setup main_thread
  main_param.sched_priority = rt_max_prio;
  pthread_attr_setschedparam(&main_sched_attr, &main_param);

  // Create main_thread
  rc = pthread_create(&main_thread,         //pointer to the thread structure
                      &main_sched_attr,     //pointer to attributes to setup the thread
                      (void *)&delay_test,  //entry point function for the new thread
                      (void *)0);           //arguments for the entry point function

  // pthread_create returns 0 on success. So, if rc is != 0, print error and exit
  if (rc)
  {
      printf("ERROR; pthread_create() rc is %d\n", rc);
      perror("pthread_create");
      exit(-1);
  }

  // Join the thread upon completion
  pthread_join(main_thread, NULL);

  // attempts to destroys the thread attribute structure main_sched_attr
  // and retusn an error if not succesful
  if(pthread_attr_destroy(&main_sched_attr) != 0)
    perror("attr destroy");
#else
  delay_test((void *)0);
#endif
  
  if(csvFileOutput != NULL){
    fclose(csvFileOutput);
  }

  printf("TEST COMPLETE\n");

  return 0;
}

