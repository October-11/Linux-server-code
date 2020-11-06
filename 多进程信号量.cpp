#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

//semctl()的第四个参数，需要手动声明
union semun {
    int val;
    struct semid_ds* buf;
    unsigned short int* array;
    struct seminfo* _buf;
}

//op = -1时执行P操作，op = 1时执行V操作,semop成功返回0，失败返回1；
void pv(int sem_id, int op) {
    struct sembuf sem_b;
    sem_b.sem_num = 0;
    sem_b.sem_op = op;
    sem_b.sem_flg = SEM_UNDO;
    semop(sem_id, &sem_b, 1);
}

int main(int argc, char* argv[]) {
    int sem_id = semget(IPC_PRIVATE, 1, 0666);
    
    union semun sem_un;
    sem_un.val = 1;
    semctl(sem_id, 0, SETVAL, sem_un);

    pid_t id = fork();
    if (id < 0) {
        printf("process create failed\n")
        return 1;
    }
    else if (id == 0){
        printf("Child try to get binary sem\n");
        pv(sem_id, -1);         //加信号
        printf("child get a sem and would release after 5 seconds\n");
        sleep(5);
        pv(sem_id, 1);
        exit(0);
    }
    else {
        printf("parents try to get a sem\n");
        pv(sem_id, -1);
        printf("parent get a sem and release after 5 sec");
        pv(sem_id, 1);
    }
    waitpid(id, NULL, 0);
    semctl(sem_id, 0, IPC_RMID, sem_un);
    return 0;
}