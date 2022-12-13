#include  <stdio.h>
#include  <stdlib.h>
#include  <sys/types.h>
#include  <sys/ipc.h>
#include  <sys/shm.h>
#include <unistd.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <fcntl.h>

void DadProcess(int [], sem_t *);
void ChildProcess(int [], sem_t *, int);
void MomProcess(int [], sem_t *);

int  main(int  argc, char *argv[]) {
  if (argc < 3) {
    printf("usage:  shm_proc <parents> <children> (where parent = [1 | 2], children = [N > 0]\n");
    exit(1);
  }
  
  int parents = atoi(argv[1]);
  if (parents != 1 && parents != 2) {
    printf("parents should be either 1 or 2\n");
    exit(1);
  }
  
  int childs = atoi(argv[2]);
  if (childs <= 0) {
    printf("childs should be > 0 i.e., at least 1\n");
    exit(1);
  }
  
  int shmID;
  int *shmPTR;
  int total = childs + parents;
  pid_t pid[total];
  sem_t *mutex;

  shmID = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
  if (shmID < 0) {
    printf("*** shmget error ***\n");
    exit(1);
  }
  shmPTR = (int *) shmat(shmID, NULL, 0);
  if (*shmPTR == -1) {
    printf("*** shmat error ***\n");
    exit(1);
  }
  shmPTR[0] = 0;
  
  if ((mutex = sem_open("banksemaphore", O_CREAT, 0644, 1)) == SEM_FAILED) {
    perror("semaphore initilization");
    exit(1);
  }

  for (int i=0; i<total; i++) {
    pid[i] = fork();
    if (pid[i] < 0) {
      printf("fork failed!");
      exit(1);
    } else if (pid[i] == 0) {
      if (i == 0) {
        DadProcess(shmPTR, mutex);
      } else if (parents == 2 && i == 2) {
        MomProcess(shmPTR, mutex);
      } else {
        ChildProcess(shmPTR, mutex, i);
      }
      exit(0);
    }
  }
  
  for (int i=0; i<total; i++) {
    wait(NULL);
  }
  
  shmdt((void *) shmPTR);
  shmctl(shmID, IPC_RMID, NULL);
}

void  ChildProcess(int sharedMem[], sem_t* mutex, int id) {
  int account, randBal;
  srand(getpid());

  for (int i=0; i<5; i++) {
    sleep(rand()%6);
    printf("Poor Student #%d: Attempting to Check Balance\n", id);
    sem_wait(mutex);
		account = sharedMem[0];
    randBal = rand() % 51;
    printf("Poor Student #%d needs $%d\n", id, randBal);
    if (rand()%2 == 0) {
      if (randBal <= account) {
        account -= randBal;
        printf("Poor Student #%d: Withdraws $%d / Balance = $%d\n", id, randBal, account);
        sharedMem[0] = account;
      } else {
        printf("Poor Student #%d: Not Enough Cash ($%d)\n", id, account);
      }
    }
    sem_post(mutex);
  }
}

void DadProcess(int sharedMem[], sem_t* mutex) {
  int account, randBal;
  srand(getpid());

  for (int i=0; i<5; i++) {
    sleep(rand()%6);
    printf("Dear Old Dad: Attempting to Check Balance\n");
    sem_wait(mutex);
    account = sharedMem[0];
    if (account <= 100) {
      randBal = rand()%101;
      if (randBal % 2) {
        account += randBal;
        printf("Dear old Dad: Deposits $%d / Balance = $%d\n", randBal, account);
        sharedMem[0] = account;
      } else {
        printf("Dear old Dad: Doesn't have any money to give\n");
      }
    } else {
      printf("Dear old Dad: Thinks Student has enough Cash ($%d)\n", account);
    }
    sem_post(mutex);
  }
}

void MomProcess(int sharedMem[], sem_t* mutex) {
  int account, randBal;
  srand(getpid());

  for (int i=0; i<5; i++) {
    sleep(rand()%10);
    printf("Loveable Mom: Attempting to Check Balance\n");
    sem_wait(mutex);
    account = sharedMem[0];
    if (account <= 100) {
      randBal = rand()%126;
      account += randBal;
      printf("Lovable Mom: Deposits $%d / Balance = $%d\n", randBal, account);
      sharedMem[0] = account;
    } else {
      printf("Lovable Mom: Thinks Student has enough Cash ($%d)\n", account);
    }
    sem_post(mutex);
  }
}