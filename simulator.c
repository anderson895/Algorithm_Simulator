/*
 * A simulator for a process scheduling algorithm.
 * Author: David Paul (David.Paul@une.edu.au)
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <string.h>

/* Cross-platform specification of booleans */
#define TRUE (1 == 1)
#define FALSE !TRUE
typedef int bool;

// Specifies whether we are in debug mode (which is useful for determining whether to print out debug messages).
bool debug = FALSE;

/* Initial data read in for a process */
typedef struct process_initial {
    unsigned int pid;  // the process id
    unsigned int processing_time;  // the total amount of processing time required for this process
    unsigned int arrival_time;  // the time this process arrived in the system
} process_initial;

/*
 * Adds the given process to the ready queue, indicating it is ready to be scheduled.
 * parameters:
 *   process - the process to add to the ready queue
 */
void add_to_ready_queue(const process_initial process);

/*
 * Determines the next process to the sceduled.
 * returns:
 *   The PID of the process to be scheduled next, or 0 if no process should be scheduled
 */
unsigned int get_next_scheduled_process();

// The file where the above two functions need to be defined
#include "scheduler.c"

/* The maximum length of a line to read */
const int MAX_LINE_LENGTH = 1024;

/* The maximum amount of time to simulate */
const int TIMEOUT = 1000000;

/* The largest process id supported by the simulator */
const int MAX_PID = 999999;

/* Stats of a process to be simulated */
typedef struct process_stats {
    process_initial initial;  // initial process data
    unsigned int processed_time;  // the total amount of processing time this process has already had
    unsigned int end_time;  // the time this process finished execution
} process_stats;

/*
 * Converts the given string to an unsigned integer.
 * Similar to the standard strtoul, only ensures value is a valid unsigned integer.
 * parameters:
 *   s - the string to convert to an unsigned integer
 *   endptr - the character after the end of the unsigned integer
 *   base - the base of the number to be read
 * returns:
 *   The result of strtoul(s, endptr, base) cast to an unsigned int
 * side effects:
 *   Sets the value of errno to ERANGE if the value read in is greater than INT_MAX
 */
unsigned int strtoui(const char *s, char **endptr, int base) {
  unsigned long val = strtoul(s, endptr, base);
  if (val > INT_MAX) {
    errno = ERANGE;
  }
  return (unsigned int) val;
}

/*
  * Reads in a single line from the given file pointer and extracts a process from it.
  * It is assumed that the line with have the format "pid,arrival_time,processing_time".
  * parameters:
  *   fp - the file pointer to read from
  * returns:
  *   A process_initial that has the pid, processing time, and arrival time from the next
  *   line read in from the file, or a negative pid if there is an error
 */
process_initial read_process_initial(FILE* fp) {
    // The PID of the process (0 to indicate not yet set)
    unsigned int pid = 0;

    // Where to read the string in to
    char line[MAX_LINE_LENGTH];

    // Read in the string and, if there is an error, ensure PID invalid
    if (fgets(line, MAX_LINE_LENGTH, fp) == NULL) {
        pid = 0;
    }

    // The end of the string we have parsed
    char *end;

    // Errno is set if strtoui has an error - setting it to 0 first makes sure we can detect it
    errno = 0;

    // Read in pid
    pid = strtoui(line, &end, 10);
    // Ensure PID invalid if there was an error
    if (errno != 0 || *end != ',') {
      pid = 0;
    }

    // Read in arrival time, starting from one character beyond where the processing_time finished
    unsigned int arrival_time = strtoui(end + 1, &end, 10);
    // Ensure PID invalid if there was an error
    if (errno != 0 || *end != ',') {
      pid = 0;
    }

    // Read in processing_time, starting from one character beyond where the pid finished
    unsigned int processing_time = strtoui(end + 1, &end, 10);
    // Ensure PID invalid if there was an error
    if (errno != 0 || *end != '\n') {
      pid = 0;
    }

    // Set up and return structure
    process_initial initial = {pid, processing_time, arrival_time};
    return initial;
}

/*
 * Determine if there is still any process that requires more processing time
 * parameters:
 *   processes - an array of the process_stats to check
 *   num_processes - the number of processes in the given array
 * returns:
 *   TRUE if any process still requires more processing time, FALSE otherwise
 */
bool still_running(process_stats processes[], unsigned int num_processes) {
  for (unsigned int i = 0; i < num_processes; i++) {
    if (processes[i].processed_time < processes[i].initial.processing_time) {
      return TRUE;
    }
  }
  return FALSE;
}

/*
 * Determine the index of the process with the given PID.
 * parmeters:
 *   pid - the PID to search for
 *   processes - an array of the processes to check
 *   num_processes - the number of processes in the given array
 * returns:
 *   The array index of the process with the given PID, or -1 if it is not found
 */
unsigned int get_pid_index(unsigned int pid, process_stats processes[], unsigned int num_processes) {
  for (unsigned int i = 0; i < num_processes; i++) {
    if (processes[i].initial.pid == pid) {
      return i;
    }
  }
  return -1;
}

/*
 * Runs the simulation from start to finish.
 * parameters:
 *   processes - an array of processes to simulate
 *   num_processes - the number of processes in the given array
 *   time_bound - the time limit for the simulation - if the simulation has not finished in that time, it is a failure
 * returns:
 *   TRUE if the simulation completed successfully, FALSE otherwise
 */
bool run_simulation(process_stats processes[], unsigned int num_processes, unsigned int time_bound) {
    printf("Time\tPID\n");
    // Continue running as long as a process still requires more processing time and we haven't run out of time
    for (unsigned int time = 0; time < time_bound && still_running(processes, num_processes); time++) {

      // Add any new processes to the queue
      for (unsigned int i = 0; i < num_processes; i++) {
        if (processes[i].initial.arrival_time == time) {
          add_to_ready_queue(processes[i].initial);
        }
      }

      // Find process to schedule
      unsigned int pid = get_next_scheduled_process();

      // If a process is scheduled:
      if (pid > 0) {
        // Find the process
        unsigned int index = get_pid_index(pid, processes, num_processes);
        if (index < 0) {
          // Invalid process scheduled - stop
          printf("Invalid pid %d!\n", pid);
          return FALSE;
        }
        // Give the selected process a time unit of execution
        processes[index].processed_time++;
        // Check if process has ended
        if (processes[index].processed_time == processes[index].initial.processing_time) {
          processes[index].end_time = time;
        } else if (processes[index].processed_time > processes[index].initial.processing_time) {
          // Process scheduled for too long - stop
          printf("Process %d scheduled for too long!\n", pid);
          return FALSE;
        }
      }

      // Output time step
      if (pid > 0) {
        printf("%d:\t%d\n", time, pid);
      } else {
        printf("%d:\t\n", time);
      }
    }
    return !still_running(processes, num_processes);
}

/*
 * Calculates the turnaround time for the given process.
 * parameters:
 *   process - The process to calculate the turnaround time for
 * returns:
 *   The turnaround time for the given process
 */
unsigned int calculate_turnaround_time(process_stats process) {
    return process.end_time - process.initial.arrival_time + 1;
}

/*
 * Calculates the wait time for the given process.
 * parameters:
 *   process - The process to calculate the wait time for
 * returns:
 *   The wait time for the given process
 */
unsigned int calculate_wait_time(process_stats process) {
    return calculate_turnaround_time(process) - process.processed_time;
}

/*
 * Calculates the average turnaround time for the given processes.
 * parameters:
 *   processes - an array of processes to calculate the average turnaround time for
 *   num_processes - the number of processes
 * returns:
 *   The average turnaround time for the given processes
 */
double calculate_average_turnaround_time(process_stats processes[], unsigned int num_processes) {
  double average_turnaround = 0;
  if (num_processes > 0) {
    for (unsigned int i = 0; i < num_processes; i++) {
      average_turnaround += calculate_turnaround_time(processes[i]);
    }
    average_turnaround /= num_processes;
  }
  return average_turnaround;
}

/*
 * Calculates the average wait time for the given processes.
 * parameters:
 *   processes - an array of processes to calculate the average wait time for
 *   num_processes - the number of processes
 * returns:
 *   The average wait time for the given processes
 */
double calculate_average_wait_time(process_stats processes[], unsigned int num_processes) {
  double average_wait = 0;
  if (num_processes > 0) {
    for (unsigned int i = 0; i < num_processes; i++) {
      average_wait += calculate_wait_time(processes[i]);
    }
    average_wait /= num_processes;
  }
  return average_wait;
}

/*
 * Prints out average turnaround time and average wait time for the given processes.
 * parameters:
 *   processes - an array of processes to display statistics for
 *   num_processes - the number of processes
 */
void print_statistics(process_stats processes[], unsigned int num_processes) {
    printf("Average turnaround time:\t%.2f\n", calculate_average_turnaround_time(processes, num_processes));
    printf("Average wait time:\t%.2f\n", calculate_average_wait_time(processes, num_processes));
}

/*
 * Prints out usage information fdr the program.
 * parameters:
 *   cmd - the command used to start the program
 *   error - the error message that should be displayed (ignored if NULL)
 */
void usage(char *cmd, char *error) {
  if (error) {
    printf("Error: %s\n\n", error);
  }

  printf("Usage: %s [-d] FILE\n", cmd);
  printf("Where:\n");
  printf("\t-d\tspecifies that the simulator should execute in debug mode\n");
  printf("\tFILE\tis the name of the file to read processes from\n");
}

/*
 *  Program entry point.
 *  Checks the required command line argument (which should be the name of the file to process) is present,
 *  attempts to read in the file, and runs the simulations.
 */
int main(int argc, char *argv[]) {
    // Check arguments
    char *filename = NULL;
    if (argc == 2) {
      filename = argv[1];
    } else if (argc == 3 && strcmp(argv[1], "-d") == 0) {
      debug = TRUE;
      filename = argv[2];
    }
    if (!filename) {
        usage(argv[0], "Invalid command line arguments");
        return -1;
    }

    // Attempt to open file
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf("Unable to read %s!\n", filename);
        return 1;
    }

    // Attempt to read number of processes
    unsigned int num_processes;
    int count = fscanf(fp, "%u\n", &num_processes);
    if (count == EOF) {
        printf("Error reading number of processes.\n");
        printf("Please ensure the file begins with a line containing the number of processes in the file.\n");
        return 1;
    }
    if (num_processes < 1 || num_processes >= TIMEOUT) {
        printf("Error reading number of processes.\n");
        printf("Please ensure there are between 1 and 1,000,000 processes.\n");
        return 1;
    }

    // Attempt to read processes, giving appropriate error messages if necessary
    process_stats processes[num_processes];
    for (unsigned int i = 0; i < num_processes; i++) {
        process_initial initial = read_process_initial(fp);
        if (initial.pid == 0) {
            printf("Error reading process on line %d!\n", i + 1);
            printf("Please ensure each process line matches the following format (with pid>0):\n");
            printf("\tpid,processing_time,arrival_time\n");
            return 1;
        } else if (initial.pid >= MAX_PID) {
          printf("Error reading process on line %d!\n", i + 1);
          printf("Please ensure each process id is less than 1,000,000.\n");
          return 1;
        } else {
          if (initial.arrival_time < 0 || initial.arrival_time >= TIMEOUT) {
              printf("Error reading process on line %d!\n", i + 1);
              printf("Please ensure each process has an arrival time between 0 and 1,000,000.\n");
              return 1;
          }
          if (initial.processing_time <= 0 || initial.processing_time >= TIMEOUT) {
              printf("Error reading process on line %d!\n", i + 1);
              printf("Please ensure each process has a processing time between 1 and 1,000,000.\n");
              return 1;
          }
          for (unsigned int j = 0; j < i; j++) {
            if (initial.pid == processes[j].initial.pid) {
              printf("Error reading process on line %d!\n", i + 1);
              printf("Please ensure each process's PID is unique.\n");
              return 1;
            }
          }
          process_stats process = {initial, 0, 0};
          processes[i] = process;
        }
    }

    // Close file
    fclose(fp);

    // Run simulation for 1,000,000 time steps, outputting results if successful
    if (run_simulation(processes, num_processes, TIMEOUT)) {
      print_statistics(processes, num_processes);
    }
}
