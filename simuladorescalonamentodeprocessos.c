#include <stdio.h>
#include <stdlib.h>

typedef enum { NEW, READY, RUNNING, WAITING, TERMINATED } ProcessState;

// Estrutura que representa um processo com suas informações e métricas
typedef struct Process {
    int pid;
    int burst_time;
    int arrival_time;
    int priority;
    int remaining_time;
    ProcessState state;
    int start_time;
    int completion_time;
    int waiting_time;
    int turnaround_time;
    int response_time;
} Process;

// Cria um novo processo inicializando seus campos e definindo estado NEW
Process* create_process(int pid, int burst_time, int arrival_time, int priority) {
    Process* proc = (Process*) malloc(sizeof(Process));
    if (!proc) {
        printf("Erro ao alocar memória para processo.\n");
        exit(EXIT_FAILURE);
    }
    proc->pid = pid;
    proc->burst_time = burst_time;
    proc->arrival_time = arrival_time;
    proc->priority = priority;
    proc->remaining_time = burst_time;
    proc->state = NEW;
    proc->start_time = -1;     // Ainda não executou
    proc->completion_time = 0;
    proc->waiting_time = 0;
    proc->turnaround_time = 0;
    proc->response_time = -1;  // Ainda não respondeu
    return proc;
}

void free_process(Process *p) {
    free(p);
}

// Imprime as métricas após a simulação para análise e comparação
void print_metrics(Process **processes, int n) {
    float total_wait = 0, total_turnaround = 0, total_response = 0;
    printf("\nPID\tBurst\tArrival\tWait\tTurnaround\tResponse\n");
    for (int i = 0; i < n; i++) {
        printf("%d\t%d\t%d\t%d\t%d\t\t%d\n",
            processes[i]->pid, processes[i]->burst_time, processes[i]->arrival_time,
            processes[i]->waiting_time, processes[i]->turnaround_time, processes[i]->response_time);
        total_wait += processes[i]->waiting_time;
        total_turnaround += processes[i]->turnaround_time;
        total_response += (processes[i]->response_time == -1) ? 0 : processes[i]->response_time;
    }
    printf("\nMédias:\nEspera: %.2f\nTurnaround: %.2f\nResposta: %.2f\n",
           total_wait / n, total_turnaround / n, total_response / n);
}

// Escalonamento FCFS (First-Come, First-Served)
void fcfs(Process **processes, int n) {
    int time = 0;
    printf("\nExecutando Escalonamento FCFS:\n");
    for (int i = 0; i < n; i++) {
        Process *p = processes[i];
        if (time < p->arrival_time) time = p->arrival_time; // Espera o processo chegar
        p->start_time = time;
        p->waiting_time = time - p->arrival_time;
        p->response_time = p->waiting_time;
        p->completion_time = time + p->burst_time;
        p->turnaround_time = p->completion_time - p->arrival_time;
        printf("Processo %d executando de %d a %d\n", p->pid, p->start_time, p->completion_time);
        time += p->burst_time;
    }
    print_metrics(processes, n);
}

// Escalonamento SJF preemptivo (Shortest Remaining Time First)
void sjf_preemptive(Process **processes, int n) {
    int time = 0, completed = 0;
    int min_remain;
    int shortest = -1;
    int *first_response = (int*)malloc(n * sizeof(int));
    int *remaining = (int*)malloc(n * sizeof(int));

    for (int i = 0; i < n; i++) {
        remaining[i] = processes[i]->burst_time;
        first_response[i] = -1;
    }

    printf("\nExecutando Escalonamento SJF Preemptivo:\n");

    while (completed < n) {
        min_remain = 1e9;
        shortest = -1;
        // Escolhe processo com menor tempo restante e que já chegou
        for (int i = 0; i < n; i++) {
            if (processes[i]->arrival_time <= time && remaining[i] > 0 && remaining[i] < min_remain) {
                min_remain = remaining[i];
                shortest = i;
            }
        }
        if (shortest == -1) {
            time++;
            continue;
        }

        if (first_response[shortest] == -1) {
            first_response[shortest] = time - processes[shortest]->arrival_time;
            processes[shortest]->start_time = time;
        }

        printf("Tempo %d executando processo %d\n", time, processes[shortest]->pid);
        remaining[shortest]--;
        time++;

        if (remaining[shortest] == 0) {
            completed++;
            int finish_time = time;
            processes[shortest]->completion_time = finish_time;
            processes[shortest]->turnaround_time = finish_time - processes[shortest]->arrival_time;
            processes[shortest]->waiting_time = processes[shortest]->turnaround_time - processes[shortest]->burst_time;
            processes[shortest]->response_time = first_response[shortest];
            printf("Processo %d finalizado em %d\n", processes[shortest]->pid, finish_time);
        }
    }
    print_metrics(processes, n);
    free(first_response);
    free(remaining);
}

// Escalonamento Round Robin com quantum configurável e preempção
void round_robin(Process **processes, int n, int quantum) {
    int time = 0, completed = 0;
    int *remaining = (int*) malloc(n * sizeof(int));
    int *response_time = (int*) malloc(n * sizeof(int));
    int *started = (int*) calloc(n, sizeof(int));
    int *queue = (int*) malloc(n * n * sizeof(int)); // fila circular 
    int front=0, rear=0;

    for (int i=0; i<n; i++) {
        remaining[i] = processes[i]->burst_time;
        response_time[i] = -1;
    }

    // Enfileira processos que já chegaram
    for (int i=0; i<n; i++) {
        if (processes[i]->arrival_time <= time) {
            queue[rear++] = i;
            started[i] = 1;
            response_time[i] = 0;
            processes[i]->start_time = time;
        }
    }

    printf("\nExecutando Escalonamento Round Robin (quantum = %d):\n", quantum);

    while (completed < n) {
        if (front == rear) { // fila vazia, avança tempo
            time++;
            for (int i=0; i<n; i++) {
                if (!started[i] && processes[i]->arrival_time <= time) {
                    queue[rear++] = i;
                    started[i] = 1;
                    response_time[i] = time - processes[i]->arrival_time;
                    processes[i]->start_time = time;
                }
            }
            continue;
        }

        int idx = queue[front++];
        int exec_time = (remaining[idx] > quantum) ? quantum : remaining[idx];
        printf("Tempo %d executando processo %d por %d unidades\n", time, processes[idx]->pid, exec_time);
        remaining[idx] -= exec_time;
        time += exec_time;

        // Enfileira processos que chegaram durante execução
        for (int i=0; i<n; i++) {
            if (!started[i] && processes[i]->arrival_time <= time) {
                queue[rear++] = i;
                started[i] = 1;
                response_time[i] = time - processes[i]->arrival_time;
                processes[i]->start_time = time;
            }
        }

        if (remaining[idx] > 0) {
            queue[rear++] = idx; // Reinsere caso não tenha terminado
        }
        else {
            completed++;
            processes[idx]->completion_time = time;
            processes[idx]->turnaround_time = time - processes[idx]->arrival_time;
            processes[idx]->waiting_time = processes[idx]->turnaround_time - processes[idx]->burst_time;
            processes[idx]->response_time = response_time[idx];
            printf("Processo %d finalizado em %d\n", processes[idx]->pid, time);
        }
    }
    print_metrics(processes, n);
    free(remaining);
    free(response_time);
    free(started);
    free(queue);
}

// Função para limpar buffer stdin
void flush_input() {
    while (getchar() != '\n');
}

// Função para obter processos do usuário
int input_processes(Process **processes, int max_processes) {
    int n;
    printf("Digite o número de processos (máximo %d): ", max_processes);
    scanf("%d", &n);
    flush_input();

    if (n <= 0 || n > max_processes) {
        printf("Número inválido de processos.\n");
        return 0;
    }

    for (int i = 0; i < n; i++) {
        int at, bt, pr;
        printf("\nProcesso %d\n", i + 1);
        printf("Tempo de chegada: ");
        scanf("%d", &at);
        flush_input();
        printf("Tempo de execução (burst): ");
        scanf("%d", &bt);
        flush_input();
        printf("Prioridade (0 a 10): ");
        scanf("%d", &pr);
        flush_input();

        processes[i] = create_process(i+1, bt, at, pr);
    }
    return n;
}

// Menu principal para o usuário escolher algoritmo e execução
int main() {
    const int MAX_PROCESS = 100;
    Process *processes[MAX_PROCESS];
    int n_process = 0;
    int quantum;

    while (1) {
        printf("\n--- Simulador de Escalonamento de Processos ---\n");
        printf("1. First-Come, First-Served (FCFS)\n");
        printf("2. Shortest Job First (Preemptivo)\n");
        printf("3. Round Robin\n");
        printf("4. Sair\n");
        printf("Escolha uma opção: ");
        int option;
        scanf("%d", &option);
        flush_input();

        if (option == 4) {
            printf("Encerrando...\n");
            break;
        }

        n_process = input_processes(processes, MAX_PROCESS);
        if (n_process == 0) continue;

        switch(option) {
            case 1:
                fcfs(processes, n_process);
                break;
            case 2:
                sjf_preemptive(processes, n_process);
                break;
            case 3:
                printf("Digite o valor do quantum: ");
                scanf("%d", &quantum);
                flush_input();
                if (quantum <= 0) {
                    quantum = 2;
                    printf("Quantum inválido. Usando valor padrão 2.\n");
                }
                round_robin(processes, n_process, quantum);
                break;
            default:
                printf("Opção inválida.\n");
                break;
        }

        // Libera memória para os processos criados
        for (int i = 0; i < n_process; i++) {
            free_process(processes[i]);
        }
    }
    return 0;
}
