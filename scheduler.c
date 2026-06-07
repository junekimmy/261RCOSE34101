#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define MAX_PROCESS 10
#define MAX_GANTT 1000
#define INF 999999

#define NEW 0
#define READY 1
#define RUNNING 2
#define WAITING 3
#define DONE 4

#define FCFS 1
#define SJF_NP 2
#define SJF_P 3
#define PRIORITY_NP 4
#define PRIORITY_P 5
#define RR 6

typedef struct {
    int pid;
    int arrival_time;
    int cpu_burst1;
    int io_burst;
    int cpu_burst2;
    int priority;

    int rem_cpu1;
    int rem_io;
    int rem_cpu2;
    int state;
    int ready_since;
    int unblock_time;

    int completion_time;
    int waiting_time;
    int turnaround_time;
} Process;

typedef struct {
    int pid;
    int start;
    int end;
} Gantt;

typedef struct {
    char name[40];
    double avg_waiting;
    double avg_turnaround;
} Result;

int next_cpu(Process p) {
    if (p.rem_cpu1 > 0) return p.rem_cpu1;
    return p.rem_cpu2;
}

void add_gantt(Gantt g[], int *cnt, int pid, int start, int end) {
    if (*cnt > 0 && g[*cnt - 1].pid == pid && g[*cnt - 1].end == start) {
        g[*cnt - 1].end = end;
    } else {
        g[*cnt].pid = pid;
        g[*cnt].start = start;
        g[*cnt].end = end;
        (*cnt)++;
    }
}

void create_processes(Process p[], int n) {
    srand(time(NULL));

    for (int i = 0; i < n; i++) {
        p[i].pid = i + 1;
        p[i].arrival_time = rand() % 8;
        p[i].cpu_burst1 = (rand() % 6) + 1;
        p[i].io_burst = (rand() % 5) + 1;
        p[i].cpu_burst2 = (rand() % 6) + 1;
        p[i].priority = (rand() % 5) + 1;

        p[i].rem_cpu1 = p[i].cpu_burst1;
        p[i].rem_io = p[i].io_burst;
        p[i].rem_cpu2 = p[i].cpu_burst2;
        p[i].state = NEW;
        p[i].ready_since = p[i].arrival_time;
        p[i].unblock_time = -1;
        p[i].completion_time = 0;
        p[i].waiting_time = 0;
        p[i].turnaround_time = 0;
    }
}

void copy_processes(Process dest[], Process src[], int n) {
    for (int i = 0; i < n; i++) {
        dest[i] = src[i];

        dest[i].rem_cpu1 = src[i].cpu_burst1;
        dest[i].rem_io = src[i].io_burst;
        dest[i].rem_cpu2 = src[i].cpu_burst2;
        dest[i].state = NEW;
        dest[i].ready_since = src[i].arrival_time;
        dest[i].unblock_time = -1;
        dest[i].completion_time = 0;
        dest[i].waiting_time = 0;
        dest[i].turnaround_time = 0;
    }
}

void print_process_table(Process p[], int n) {
    printf("\nGenerated Process Table\n");
    printf("-----------------------------------------------------------------------\n");
    printf("PID\tArrival\tCPU1\tI/O\tCPU2\tPriority\tTotal CPU\n");
    printf("-----------------------------------------------------------------------\n");

    for (int i = 0; i < n; i++) {
        printf("P%d\t%d\t%d\t%d\t%d\t%d\t\t%d\n",
               p[i].pid,
               p[i].arrival_time,
               p[i].cpu_burst1,
               p[i].io_burst,
               p[i].cpu_burst2,
               p[i].priority,
               p[i].cpu_burst1 + p[i].cpu_burst2);
    }

    printf("-----------------------------------------------------------------------\n");
}

void update_new_and_io(Process p[], int n, int time) {
    for (int i = 0; i < n; i++) {
        if (p[i].state == NEW && p[i].arrival_time <= time) {
            p[i].state = READY;
            p[i].ready_since = time;
        }

        if (p[i].state == WAITING && p[i].unblock_time <= time) {
            p[i].state = READY;
            p[i].ready_since = time;
        }
    }
}

void add_waiting_time(Process p[], int n, int running) {
    for (int i = 0; i < n; i++) {
        if (p[i].state == READY && i != running) {
            p[i].waiting_time++;
        }
    }
}

int select_process(Process p[], int n, int algorithm) {
    int selected = -1;

    for (int i = 0; i < n; i++) {
        if (p[i].state != READY) continue;

        if (selected == -1) {
            selected = i;
        } else if (algorithm == FCFS) {
            if (p[i].ready_since < p[selected].ready_since) {
                selected = i;
            } else if (p[i].ready_since == p[selected].ready_since && p[i].pid < p[selected].pid) {
                selected = i;
            }
        } else if (algorithm == SJF_NP || algorithm == SJF_P) {
            if (next_cpu(p[i]) < next_cpu(p[selected])) {
                selected = i;
            } else if (next_cpu(p[i]) == next_cpu(p[selected]) && p[i].pid < p[selected].pid) {
                selected = i;
            }
        } else if (algorithm == PRIORITY_NP || algorithm == PRIORITY_P) {
            if (p[i].priority < p[selected].priority) {
                selected = i;
            } else if (p[i].priority == p[selected].priority && p[i].pid < p[selected].pid) {
                selected = i;
            }
        }
    }

    return selected;
}

void execute_one_time_unit(Process p[], int idx, int time, int *completed) {
    if (p[idx].rem_cpu1 > 0) {
        p[idx].rem_cpu1--;

        if (p[idx].rem_cpu1 == 0) {
            p[idx].state = WAITING;
            p[idx].unblock_time = time + 1 + p[idx].io_burst;
        }
    } else if (p[idx].rem_cpu2 > 0) {
        p[idx].rem_cpu2--;

        if (p[idx].rem_cpu2 == 0) {
            p[idx].state = DONE;
            p[idx].completion_time = time + 1;
            p[idx].turnaround_time = p[idx].completion_time - p[idx].arrival_time;
            (*completed)++;
        }
    }
}

void print_gantt(Gantt g[], int cnt) {
    printf("\nGantt Chart\n");

    for (int i = 0; i < cnt; i++) {
        if (g[i].pid == 0) printf("| IDLE ");
        else printf("| P%d ", g[i].pid);
    }

    printf("|\n");

    for (int i = 0; i < cnt; i++) {
        printf("%d\t", g[i].start);
    }

    if (cnt > 0) printf("%d", g[cnt - 1].end);

    printf("\n");
}

void print_result(char name[], Process p[], int n, Gantt g[], int gcnt, Result *r) {
    double total_waiting = 0;
    double total_turnaround = 0;

    printf("\n\n==================== %s ====================\n", name);
    print_gantt(g, gcnt);

    printf("\nResult Table\n");
    printf("-----------------------------------------------------------------------\n");
    printf("PID\tArrival\tComplete\tWaiting\tTurnaround\tPriority\n");
    printf("-----------------------------------------------------------------------\n");

    for (int i = 0; i < n; i++) {
        printf("P%d\t%d\t%d\t\t%d\t%d\t\t%d\n",
               p[i].pid,
               p[i].arrival_time,
               p[i].completion_time,
               p[i].waiting_time,
               p[i].turnaround_time,
               p[i].priority);

        total_waiting += p[i].waiting_time;
        total_turnaround += p[i].turnaround_time;
    }

    r->avg_waiting = total_waiting / n;
    r->avg_turnaround = total_turnaround / n;
    strcpy(r->name, name);

    printf("-----------------------------------------------------------------------\n");
    printf("Average Waiting Time: %.2f\n", r->avg_waiting);
    printf("Average Turnaround Time: %.2f\n", r->avg_turnaround);
}

void run_normal_algorithm(Process original[], int n, int algorithm, char name[], Result *r) {
    Process p[MAX_PROCESS];
    Gantt g[MAX_GANTT];

    int gcnt = 0;
    int time = 0;
    int completed = 0;
    int current = -1;
    int preemptive = (algorithm == SJF_P || algorithm == PRIORITY_P);

    copy_processes(p, original, n);

    while (completed < n) {
        update_new_and_io(p, n, time);

        if (preemptive && current != -1 && p[current].state == RUNNING) {
            p[current].state = READY;
            p[current].ready_since = time;
            current = -1;
        }

        if (current == -1) {
            current = select_process(p, n, algorithm);

            if (current != -1) {
                p[current].state = RUNNING;
            }
        }

        if (current == -1) {
            add_gantt(g, &gcnt, 0, time, time + 1);
            add_waiting_time(p, n, -1);
            time++;
            continue;
        }

        add_gantt(g, &gcnt, p[current].pid, time, time + 1);
        add_waiting_time(p, n, current);
        execute_one_time_unit(p, current, time, &completed);
        time++;

        if (p[current].state == WAITING || p[current].state == DONE) {
            current = -1;
        }
    }

    print_result(name, p, n, g, gcnt, r);
}

int in_queue(int q[], int front, int rear, int value) {
    for (int i = front; i < rear; i++) {
        if (q[i] == value) return 1;
    }

    return 0;
}

void enqueue_ready(Process p[], int n, int q[], int *rear, int front, int current) {
    for (int i = 0; i < n; i++) {
        if (p[i].state == READY && i != current && !in_queue(q, front, *rear, i)) {
            q[*rear] = i;
            (*rear)++;
        }
    }
}

void run_round_robin(Process original[], int n, int quantum, Result *r) {
    Process p[MAX_PROCESS];
    Gantt g[MAX_GANTT];
    int q[MAX_GANTT];

    int front = 0;
    int rear = 0;
    int gcnt = 0;
    int time = 0;
    int completed = 0;
    int current = -1;
    int used_quantum = 0;

    copy_processes(p, original, n);

    while (completed < n) {
        update_new_and_io(p, n, time);
        enqueue_ready(p, n, q, &rear, front, current);

        if (current == -1 && front < rear) {
            current = q[front];
            front++;
            p[current].state = RUNNING;
            used_quantum = 0;
        }

        if (current == -1) {
            add_gantt(g, &gcnt, 0, time, time + 1);
            add_waiting_time(p, n, -1);
            time++;
            continue;
        }

        add_gantt(g, &gcnt, p[current].pid, time, time + 1);
        add_waiting_time(p, n, current);
        execute_one_time_unit(p, current, time, &completed);
        used_quantum++;
        time++;

        update_new_and_io(p, n, time);
        enqueue_ready(p, n, q, &rear, front, current);

        if (p[current].state == WAITING || p[current].state == DONE) {
            current = -1;
            used_quantum = 0;
        } else if (used_quantum == quantum) {
            p[current].state = READY;
            p[current].ready_since = time;
            q[rear] = current;
            rear++;
            current = -1;
            used_quantum = 0;
        }
    }

    print_result("Round Robin", p, n, g, gcnt, r);
}

void print_comparison(Result r[], int count) {
    int best_wait = 0;
    int best_turn = 0;

    printf("\n\n==================== Performance Comparison ====================\n");
    printf("-----------------------------------------------------------------------\n");
    printf("Algorithm\t\t\tAverage Waiting\tAverage Turnaround\n");
    printf("-----------------------------------------------------------------------\n");

    for (int i = 0; i < count; i++) {
        printf("%-25s\t%.2f\t\t%.2f\n",
               r[i].name,
               r[i].avg_waiting,
               r[i].avg_turnaround);

        if (r[i].avg_waiting < r[best_wait].avg_waiting) best_wait = i;
        if (r[i].avg_turnaround < r[best_turn].avg_turnaround) best_turn = i;
    }

    printf("-----------------------------------------------------------------------\n");
    printf("Best average waiting time: %s (%.2f)\n",
           r[best_wait].name,
           r[best_wait].avg_waiting);
    printf("Best average turnaround time: %s (%.2f)\n",
           r[best_turn].name,
           r[best_turn].avg_turnaround);
}

int main() {
    Process processes[MAX_PROCESS];
    Result results[6];

    int n;
    int quantum;

    printf("CPU Scheduling Simulator\n");
    printf("Enter number of processes (1-%d): ", MAX_PROCESS);
    scanf("%d", &n);

    if (n < 1 || n > MAX_PROCESS) {
        printf("Invalid number of processes.\n");
        return -1;
    }

    printf("Enter time quantum for Round Robin: ");
    scanf("%d", &quantum);

    if (quantum <= 0) {
        printf("Invalid time quantum.\n");
        return -1;
    }

    create_processes(processes, n);
    print_process_table(processes, n);

    run_normal_algorithm(processes, n, FCFS, "FCFS", &results[0]);
    run_normal_algorithm(processes, n, SJF_NP, "Non-Preemptive SJF", &results[1]);
    run_normal_algorithm(processes, n, SJF_P, "Preemptive SJF", &results[2]);
    run_normal_algorithm(processes, n, PRIORITY_NP, "Non-Preemptive Priority", &results[3]);
    run_normal_algorithm(processes, n, PRIORITY_P, "Preemptive Priority", &results[4]);
    run_round_robin(processes, n, quantum, &results[5]);

    print_comparison(results, 6);

    return 0;
}