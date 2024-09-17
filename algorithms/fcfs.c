#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int pid;
    int arrival_time;
    int processing_time;
    int waiting_time;
    int turnaround_time;
} Process;

// Function to calculate FCFS scheduling and print results
void calculate_fcfs(Process processes[], int n) {
    int current_time = 0;
    int total_waiting_time = 0, total_turnaround_time = 0;

    printf("Time\tPID\n");

    for (int i = 0; i < n; i++) {
        Process *p = &processes[i];

        // Ensure the current time is at least the arrival time of the process
        if (current_time < p->arrival_time) {
            current_time = p->arrival_time;
        }

        // Calculate waiting time and turnaround time
        p->waiting_time = current_time - p->arrival_time;
        p->turnaround_time = p->waiting_time + p->processing_time;

        // Schedule the process
        for (int j = 0; j < p->processing_time; j++) {
            printf("%d\t%d\n", current_time++, p->pid);
        }

        // Track the total waiting and turnaround times for averaging later
        total_waiting_time += p->waiting_time;
        total_turnaround_time += p->turnaround_time;
    }

    // Calculate average waiting and turnaround times
    float avg_waiting_time = (float)total_waiting_time / n;
    float avg_turnaround_time = (float)total_turnaround_time / n;

    // Print the average waiting and turnaround times
    printf("Average waiting time:\t%.2f\n", avg_waiting_time);
    printf("Average turnaround time:\t%.2f\n", avg_turnaround_time);
}

int main() {
    // Open the input file spec_schedule.txt
    FILE *file = fopen("../schedules/spec_schedule.txt", "r");
    if (!file) {
        printf("Error opening file.\n");
        return 1;
    }

    // Read the number of processes
    int n;
    fscanf(file, "%d", &n);

    // Dynamically allocate memory for the processes
    Process *processes = (Process *)malloc(n * sizeof(Process));
    if (!processes) {
        printf("Memory allocation failed.\n");
        return 1;
    }

    // Read process details from the input file
    for (int i = 0; i < n; i++) {
        fscanf(file, "%d,%d,%d", &processes[i].pid, &processes[i].arrival_time, &processes[i].processing_time);
        processes[i].waiting_time = 0;
        processes[i].turnaround_time = 0;
    }

    // Close the input file
    fclose(file);

    // Perform FCFS scheduling
    calculate_fcfs(processes, n);

    // Free dynamically allocated memory
    free(processes);

    return 0;
}
