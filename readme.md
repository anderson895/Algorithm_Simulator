# Process Scheduling Simulator

A program for simulating process scheduling using various algorithms.

## Usage

First, ensure ```cosc240_a4.sh``` is executable:

```sh
chmod +x cosc240_a4.sh
```

Then, to use the scheduling file named "algorithm.c" from the submission in the directory ```submissions/submission``` to simulate the scheduling of processes specified in the schedule files stored in the directory ```./schedules``` (the format of which is described below), you can execute the following command:

```sh
./cosc240_a4.sh -d schedules -u submissions/submission -1 algorithm
```

Note that there is a one minute timeout, after which the simulator will be terminated if it hasn't completed the processing for any particular schedule, so you should ensure your scheduling algorithm completes within that time.

For more details on how to use ```cosc240_a4.sh```, execute the command

```sh
./cosc240_a4.sh -h
```

## Input Format

The format of the schedule file that specifies the processes to be scheduled is described in this section.

* The first line is an integer, giving the number, *n*, of processes listed in the file.

* Each remaining line of the file (there will be *n* of them) will represent a single process, and will consist of three integers separated by commas (,). The three integers are *PID*, *Arrival time*, *Required processing time* respectively.

For example, consider the following:

```
3
1,0,10
2,2,1
3,6,2
```

This would represent three different processes:

1.  Process 1 arrives at time 0 and requires 10 units of processing time
2.  Process 2 arrives at time 2 and requires 1 unit of processing time
3.  Process 3 arrives at time 6 and requires 2 units of processing time

The number of processes, and each PID, arrival time, and required processing time must be a positive integer less than 1,000,000.

If the file passed to the program is not valid (i.e., it doesn't match the required format, each PID is not unique, or a process requires less than 1 unit of processing time), the program will exit with an appropriate error message.

## Output

Using the ```cosc240_a4.sh``` script, output from the simulator is directed to files in the output directory (which defaults to ```./output```).

The simulator program outputs the following details, followed by a blank line:

1. The string ```"Time\tPID"```
2.  For each time step where not all processes have completed, the time step and the id of the process scheduled to execute at that time separated by tabs (if no process should be scheduled at that time, then the line should just contain the time step followed by a tab)
3. The string ```"Average waiting time:\t"``` followed by the average waiting time for all processes (in time units, to two decimal places)
4. The string ```"Average turnaround time:\t"``` followed by average turnaround time for all processes (in time units, to two decimal places)

Note that the program assumes the context switching time is *zero*, which is not realistic but is used for this simulator.

Given the example input from the *Input Format* above, for example, and using FCFS, the program will produce the following output:

```
Time	PID
0	1
1	1
2	1
3	1
4	1
5	1
6	1
7	1
8	1
9	1
10	2
11	3
12	3
Average waiting time:	4.33
Average turnaround time:	8.67
```

## Implementing new algorithms

It is recommended you place all C files to implement the different scheduling algorithms in the ```./submissions/submission``` directory though any directory can be used by specifying it using the ```-u``` command line argument to ```cosc240_a4.sh```.

A good way to start is to copy the provided ```fcfs.c``` as the base for your implementation of the other algorithms (e.g., ```cp algorithms/fcfs.c submissions/submission/custom.c```).

Each algorithm must implement the following two functions:

* ```void add_to_ready_queue(const process_initial process)```

    This function adds the given process to the ready queue, indicating it is ready to be scheduled. It is called by the simulator whenever a new process enters the system.


* ```unsigned int get_next_scheduled_process()```

    This function determines the next process to the scheduled. It is called by the simulator at each time step.
    
By modifying these functions, you can specify how the ready queue is organised and which function will be scheduled for the next time step.

It is recommended that you read through the code to ensure you are familiar with how the simulator works with these functions.
