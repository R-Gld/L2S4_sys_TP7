#define _DEFAULT_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <wait.h>
#include <limits.h>

#define N_fix 100000000

/**
 * Retourne un nombre aléatoire entre 0 et 1.
 * @return un nombre aléatoire entre 0 et 1.
 */
double get_random_btwn_0_1(void);
void seq_pi(double *pi);
void biprocess_pi(double *pi);
void interrupted_pi(double *pi);
void parallel_pi(double *pi, int q);

void close_all_tubes(int tubes[][2], int count);

volatile sig_atomic_t keep_running = 1;
void sigint_handler(int signal) {
    keep_running = 0;
}

int main(int argc, char **argv) {

    if(argc != 2) {
        fprintf(stderr, "Usage: %s nb_process\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int q = atoi(argv[1]);
    if(q < 1) {
        fprintf(stderr, "Erreur, nb_process doit être un nombre supérieur à 1.");
        exit(EXIT_FAILURE);
    }

    double pi;
    parallel_pi(&pi, q);
    printf("\n\npi = %f\n\n\n", pi);
    return EXIT_SUCCESS;
}

double get_random_btwn_0_1(void) {
    return (double) rand() / RAND_MAX;
}
void close_all_tubes(int tubes[][2], int count) {
    for (int i = 0; i < count; i++) {
        close(tubes[i][0]);
        close(tubes[i][1]);
    }
}

void seq_pi(double *pi) {
    srand(time(NULL));
    *pi = 0;
    double p = 0;
    for (int i = 0; i < N_fix; i++) {
        double x = get_random_btwn_0_1();
        double y = get_random_btwn_0_1();
        if ((x * x) + (y * y) < 1) {
            p += 1;
        }
    }
    *pi = 4 * p / N_fix;
}

void biprocess_pi(double *pi) {
    int tube[2];
    pipe(tube);

    pid_t pid1 = fork();
    if(pid1 == -1) { perror("fork");exit(EXIT_FAILURE); }
    if(pid1 == 0) {
        srand(getpid());
        close(tube[0]);
        double p = 0;
        for(int i = 0; i < N_fix / 2; i++) {
            double x = get_random_btwn_0_1();
            double y = get_random_btwn_0_1();
            if ((x * x) + (y * y) < 1) {
                p += 1;
            }
        }
        write(tube[1], &p, sizeof(double));
        close(tube[1]);
        exit(EXIT_SUCCESS);
    }

    pid_t pid2 = fork();
    if(pid2 == -1) { perror("fork");exit(EXIT_FAILURE); }
    if(pid2 == 0) {
        srand(getpid());
        close(tube[0]);

        double p = 0;
        for (int i = 0; i < N_fix / 2; i++) {
            double x = get_random_btwn_0_1();
            double y = get_random_btwn_0_1();
            if ((x * x) + (y * y) < 1) {
                p += 1;
            }
        }
        write(tube[1], &p, sizeof(p));
        close(tube[1]);
        exit(EXIT_SUCCESS);
    }

    close(tube[1]);
    double p1, p2;
    read(tube[0], &p1, sizeof(p1));
    read(tube[0], &p2, sizeof(p2));
    close(tube[0]);

    if( wait(NULL) == -1 ) { perror("Waiting child"); exit(EXIT_FAILURE); }
    if( wait(NULL) == -1 ) { perror("Waiting child"); exit(EXIT_FAILURE); }
    *pi = 4 * (p1 + p2) / N_fix;
}

void interrupted_pi(double *pi) {
    struct sigaction old_sa; // ancien handler,
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = sigint_handler;
    sigaction(SIGINT, &sa, &old_sa);

    int tube1[2];
    int tube2[2];
    if(pipe(tube1) == -1) {
        perror("pipe for process 1 failed");
        exit(EXIT_FAILURE);
    }

    if(pipe(tube2) == -1) {
        perror("pipe for process 2 failed");
        exit(EXIT_FAILURE);
    }

    pid_t pid1 = fork();
    if(pid1 == -1) { perror("fork");exit(EXIT_FAILURE); }
    if(pid1 == 0) {
        srand(getpid());
        close(tube1[0]);
        double p1 = 0;
        int n1 = 0;

        while(keep_running && n1 < INT_MAX) {
            n1++;
            double x = get_random_btwn_0_1();
            double y = get_random_btwn_0_1();
            if ((x * x) + (y * y) < 1) {
                p1 += 1;
            }
        }

        if(write(tube1[1], &p1, sizeof(p1)) == -1) { perror("write p1"); exit(EXIT_FAILURE); }
        if(write(tube1[1], &n1, sizeof(n1)) == -1) { perror("write n1"); exit(EXIT_FAILURE); }
        close(tube1[1]);
        exit(EXIT_SUCCESS);
    }

    pid_t pid2 = fork();
    if(pid2 == -1) { perror("fork");exit(EXIT_FAILURE); }
    if(pid2 == 0) {
        srand(getpid());
        close(tube2[0]);

        double p2 = 0;
        int n2 = 0;

        while(keep_running && n2 < INT_MAX) {
            n2++;
            double x = get_random_btwn_0_1();
            double y = get_random_btwn_0_1();
            if ((x * x) + (y * y) < 1) {
                p2 += 1;
            }
        }

        if( write(tube2[1], &p2, sizeof(p2)) == -1 ) { perror("write p2"); exit(EXIT_FAILURE); }
        if( write(tube2[1], &n2, sizeof(n2)) == -1 ) { perror("write n2"); exit(EXIT_FAILURE); }
        close(tube2[1]);
        exit(EXIT_SUCCESS);
    }
    struct sigaction empty_sa;
    sigemptyset(&empty_sa.sa_mask);
    empty_sa.sa_flags = 0;
    empty_sa.sa_handler = SIG_IGN;
    sigaction(SIGINT, &empty_sa, NULL);


    double p1, p2;
    int n1, n2;
    close(tube1[1]); close(tube2[1]);

    if( read(tube1[0], &p1, sizeof(p1)) == -1 ) { perror("read p1 failed"); exit(1); }
    if( read(tube1[0], &n1, sizeof(n1)) == -1 ) { perror("read n1 failed"); exit(1); }
    if( read(tube2[0], &p2, sizeof(p2)) == -1 ) { perror("read p2 failed"); exit(1); }
    if( read(tube2[0], &n2, sizeof(n2)) == -1 ) { perror("read n2 failed"); exit(1); }

    close(tube1[0]); close(tube2[0]);
    if( wait(NULL) == -1 ) { perror("Waiting child"); exit(EXIT_FAILURE); }
    if( wait(NULL) == -1 ) { perror("Waiting child"); exit(EXIT_FAILURE); }

    *pi = 4.0 * (p1 + p2) / (n1 + n2);

    sigaction(SIGINT, &old_sa, NULL);
}

void parallel_pi(double *pi, int q) {
    struct sigaction old_sa; // ancien sigaction
    struct sigaction sa; // nouveau sigaction, utilisé avec sigint_handler(int signal); pour mettre keep_running à 0 lors d'un Ctrl+c
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = sigint_handler;
    sigaction(SIGINT, &sa, &old_sa);

    int tube[q][2];
    for(int i = 0; i < q; ++i) {
        if(pipe(tube[i]) == -1) {
            perror("Pipe for process failed");
            close_all_tubes(tube, q);
            exit(EXIT_FAILURE);
        }
        pid_t pid = fork();
        if(pid == -1) { perror("Creating new child"); close_all_tubes(tube, q); exit(EXIT_FAILURE); }
        if(pid == 0) {
            srand(time(NULL) ^ getpid());
            close(tube[i][0]); // on ferme l'extrémité de lecture, car on ne l'utilise pas.
            double p = 0;
            unsigned long n = 0; // Ici, on utilise un unsigned long pour avoir une capacité d'itération maximale.
            while(keep_running && n < ULONG_MAX) { // Ici, on vérifie que n < ULONG_MAX pour que si on ne fait pas de Ctrl+c, n n'ai pas d'overflow.
                n++;
                double x = get_random_btwn_0_1();
                double y = get_random_btwn_0_1();
                if ((x * x) + (y * y) < 1) p++;
            }
            if( write(tube[i][1], &p, sizeof(p)) == -1 ) { perror("write p"); close_all_tubes(tube, q); exit(EXIT_FAILURE); }
            if( write(tube[i][1], &n, sizeof(n)) == -1 ) { perror("write n"); close_all_tubes(tube, q); exit(EXIT_FAILURE); }
            close(tube[i][1]); // fermeture de l'écriture.
            exit(EXIT_SUCCESS);
        }
    }

    struct sigaction empty_sa;
    sigemptyset(&empty_sa.sa_mask);
    empty_sa.sa_flags = 0;
    empty_sa.sa_handler = SIG_IGN;
    sigaction(SIGINT, &empty_sa, NULL); // on ignore le SIG_INT (Ctrl+c) pour ne pas affecter le processus principal lors du Ctrl+c d'arret de calcul.

    double p_sum = 0;
    unsigned long n_sum = 0;
    for(int i = 0; i < q; ++i) {
        close(tube[i][1]);
        double p;
        unsigned long n;
        if( read(tube[i][0], &p, sizeof(double)) == -1 ) { perror("Read p threw tube"); close_all_tubes(tube, q); exit(EXIT_FAILURE); }
        if( read(tube[i][0], &n, sizeof(unsigned long)) == -1 ) { perror("Readn  threw tube"); close_all_tubes(tube, q); exit(EXIT_FAILURE); }
        close(tube[i][0]);
        p_sum += p;
        n_sum += n;
        if( wait(NULL) == -1 ) { perror("Waiting child"); exit(EXIT_FAILURE); }
    }

    *pi = 4 * p_sum / n_sum;
    printf("Nombre d'itération (n): %lu\n", n_sum);

    sigaction(SIGINT, &old_sa, NULL); // Ici, on remet le sigaction d'avant le calcul.
}
