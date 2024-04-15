/*
Emma Gonzalez NetID: elg210004
Mateo Estrada NetID: mxe210022
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <errno.h>     // for EINTR
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>

struct mThread{
    int start;
    int end;
    uint32_t hash;
};


// Print out the usage of the program and exit.
void Usage(char*);
uint32_t jenkins_one_at_a_time_hash(const uint8_t* , uint64_t );
void *tree(void *);
double GetTime();

// block size
#define BSIZE 4096

int
main(int argc, char** argv)
{
  int32_t fd;
  uint32_t nblocks;

  // input checking
  if (argc != 3)
    Usage(argv[0]);

  // open input file
  fd = open(argv[1], O_RDWR);
  if (fd == -1) {
    perror("open failed");
    exit(EXIT_FAILURE);
  }
  // use fstat to get file size
  // calculate nblocks
  struct stat statbuf;
  // use fstat to get file size
  if(fstat(fd, &statbuf) == -1){
    perror("fstat failed");
    exit(EXIT_FAILURE);
  }
  // a file can be divided into n blocks of size BSIZE
  size_t size = statbuf.st_size;
  nblocks = size / BSIZE;
  printf(" no. of blocks = %u \n", nblocks);

  double start = GetTime();

  // calculate hash value of the input file
  void* arr = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (arr == MAP_FAILED) {
    perror("mmap failed");
    exit(EXIT_FAILURE);
  }

    double start = 0;

    // create m threads
    pthread_t rootThread;
    struct mThread args;
    args.start = 0;
    args.end = nblocks - 1;

  double end = GetTime();
  printf("hash value = %u \n", args.hash);
  printf("time taken = %f \n", (end - start));
  close(fd);
  return EXIT_SUCCESS;
}

uint32_t
jenkins_one_at_a_time_hash(const uint8_t* key, uint64_t length)
{
  uint64_t i = 0;
  uint32_t hash = 0;

  while (i != length) {
    hash += key[i++];
    hash += hash << 10;
    hash ^= hash >> 6;
  }
  hash += hash << 3;
  hash ^= hash >> 11;
  hash += hash << 15;
  return hash;
}


void
Usage(char* s)
{
  fprintf(stderr, "Usage: %s filename num_threads \n", s);
  exit(EXIT_FAILURE);
}


void *tree(void *arg) {
    struct mThread *args = (struct mThread *)arg;
    int ht = args -> end - args -> start + 1;

    // if leaf node
    if(ht == 0){
        // compute hash value of n/m consecutive blocks assigned to it 
    }
    // if 
    else{
        pthread_t p1, p2;
        struct mThread leftThread, rightThread;
        leftThread.start = args -> start;
        leftThread.end = args -> start + ht / 2 - 1;

        rightThread.start = args -> start + ht / 2;
        rightThread.end = args -> end;

        pthread_create(&p1, NULL, tree, (void *) &ht);
        pthread_create(&p2, NULL, tree, &ht);

        pthread_join(p1, NULL);
        pthread_join(p2, NULL);

        // return hash value to its parent thread usings pthread_exit() call
       /* char combinedHash[33];
        fprintf(combinedHash,sizeof(combinedHash), "%u%u%u", args->hash, leftThread.hash, rightThread.hash);
        args->hash = jenkins_one_at_a_time_hash((uint8_t*)combinedHash, sizeof(combinedHash));
*/
        pthread_exit(NULL);
    }
    return NULL;
}

double GetTime() {
    struct timespec threadTime;
    clock_gettime(CLOCK_MONOTONIC, &threadTime);
    // whole seconds plus nanoseconds
    return threadTime.tv_sec + threadTime.tv_nsec / 1e9;
}
