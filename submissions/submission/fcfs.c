/*
 * An implementation of FCFS scheduling.
 * Author: David Paul (David.Paul@une.edu.au)
 */

/* The process details we're interested in for the FCFS algorithm.*/
typedef struct fcfs_process {
  unsigned int pid;
  unsigned int processing_time;
  unsigned int arrival_time;
  unsigned int processed_time;
  struct fcfs_process *next_process;
} fcfs_process;

/* The list of all processes we know about.*/
fcfs_process process_list = {0, 0, 0, 0, NULL};

/*
 * Prints out the list of all fcfs_processes after the given one.
 * parameters:
 *   node: The fcfs_process to print all later processes from
 */
void print_list(fcfs_process *node) {
  while (node->next_process) {
    fcfs_process *next = node->next_process;
    printf("pid: %d, processing_time %d, arrival_time: %d, processed_time: %d, next_process.pid: %d\n",
        next->pid,
        next->processing_time,
        next->arrival_time,
        next->processed_time,
        next->next_process ? next->next_process->pid : 0);
    node = node->next_process;
  }
}

/*
 * Adds the fcfs_process pointed to by next to be immediately after node.
 * parameters:
 *   node: The fcfs_process to add the next fcfs_process after
 *   next: The new next_process for node
 */
void add_next(fcfs_process *node, fcfs_process *next) {
  next->next_process = node->next_process;
  node->next_process = next;
}

/*
 * Removes the next_process after the given node.
 * If the next_process has a next_process, that next_process becomes node's next_process.
 * parameters:
 *   node: The fcfs_process to remove the immediate next_process from
 * returns:
 *   The fcfs_process removed from next to the given node
 */
fcfs_process* remove_next(fcfs_process *node) {
  fcfs_process *next = node->next_process;
  if (next) {
    node->next_process = next->next_process;
  }
  return next;
}

/*
 * Adds the given process to the ready queue, indicating it is ready to be scheduled.
 * Keeps a list of all processes, sorted by arrival time, then pid.
 * parameters:
 *   process - the process to add to the ready queue
 */
void add_to_ready_queue(const process_initial process) {
  // Construct the new fcfs_process
  fcfs_process *new_process = malloc(sizeof(fcfs_process));
  new_process->pid = process.pid;
  new_process->processing_time = process.processing_time;
  new_process->arrival_time = process.arrival_time;
  new_process->processed_time = 0;
  new_process->next_process = NULL;

  // Determine where in the queue it should be added
  fcfs_process *end = &process_list;
  // Skip past processes with an earlier arrival time
  while (end->next_process && end->next_process->arrival_time < new_process->arrival_time) {
    end = end->next_process;
  }
  // Skip past processes with the same arrival time but an earlier PID
  while (end->next_process && end->next_process->arrival_time == new_process->arrival_time && end->next_process->pid < new_process->pid) {
    end = end->next_process;
  }

  // Add new process to the queue in the correct location
  add_next(end, new_process);

  // If in debug mode, print out the process list after it has changed
  if (debug) {
    printf("Process list after adding process with pid %d:\n", process.pid);
    print_list(&process_list);
  }
}

/*
 * Determines the next process to the scheduled.
 * Implementes FCFS, meaning it will select the process with the earliest arrival time
 * that has not yet completed.
 * If two processes have the same arrival time, it will select the one with the lowest PID first.
 * returns:
 *   The PID of the process to be scheduled next, or 0 if no process should be scheduled
 */
unsigned int get_next_scheduled_process() {
  fcfs_process *node = &process_list;
  // While there are still processes to check
  while (node->next_process) {
    fcfs_process *next = node->next_process;

    // If the next process needs more processing time, schedule it
    if (next->processed_time < next->processing_time) {
      unsigned int pid = next->pid;
      next->processed_time++;
      // Check if the process has finished
      if (next->processed_time == next->processing_time) {
        // If so, remove it from the list
        remove_next(node);
        free(next);

        // If in debug mode, print out the process list after it has changed
        if (debug) {
          printf("Process list after process with pid %d has completed:\n", pid);
          print_list(&process_list);
        }
      }
      return pid;
    }

    // Move to the next process
    node = node->next_process;
  }
  return 0;
}

