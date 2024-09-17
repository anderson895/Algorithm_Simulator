#include <stdio.h>
#include <stdlib.h>

#define QUANTUM 3

typedef struct {
    int pid;
    int arrival_time;
    int processing_time;
    int remaining_time;
    int completion_time;
} Process;

void round_robin(Process processes[], int n) {
    int time = 0;
    int remaining = n;
    int total_wt = 0, total_tat = 0;

    printf("Time\tPID\n");

    while (remaining > 0) {
        for (int i = 0; i < n; i++) {
            Process *p = &processes[i];

            if (p->remaining_time > 0 && p->arrival_time <= time) {
                int exec_time = (p->remaining_time < QUANTUM) ? p->remaining_time : QUANTUM;

                // Execute the process for the given quantum or less
                for (int j = 0; j < exec_time; j++) {
                    printf("%d\t%d\n", time++, p->pid);
                }

                p->remaining_time -= exec_time;

                // If the process is completed
                if (p->remaining_time == 0) {
                    p->completion_time = time;
                    remaining--;

                    // Calculate TAT and WT
                    int tat = p->completion_time - p->arrival_time;
                    int wt = tat - p->processing_time;
                    total_tat += tat;
                    total_wt += wt;
                }
            }
        }
    }

    printf("\nAverage Turnaround Time: %.2f\n", (float)total_tat / n);
    printf("Average Waiting Time: %.2f\n", (float)total_wt / n);
}

int main() {
     FILE *file = fopen("../schedules/spec_schedule.txt", "r");
    if (!file) {
        printf("Error opening file.\n");
        return 1;
    }

    int n;
    fscanf(file, "%d", &n);

    Process *processes = (Process *)malloc(n * sizeof(Process));

    for (int i = 0; i < n; i++) {
        fscanf(file, "%d,%d,%d", &processes[i].pid, &processes[i].arrival_time, &processes[i].processing_time);
        processes[i].remaining_time = processes[i].processing_time;
        processes[i].completion_time = 0;
    }

    fclose(file);

    round_robin(processes, n);
    free(processes);

    return 0;
}
