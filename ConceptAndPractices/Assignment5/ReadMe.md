# The assignment

In this particular exercise, we deal with the seqgen3.c code, in a Scenario where we want to implement the scheduling of three different threads at three particular frequencies, namely 6.67Hz, 10Hz and 50Hz.

The excercize consists in using semaphores (sem_post) to trigger the release of a specific thread with a predefined frequencies, log the release time in /var/log/syslog and, after the execution, perform a verification by inspection on the syslog file to check that the release time of each thread is respected.
My plan is to add a video walkthrough of the code on youtube. I would then post the link here
