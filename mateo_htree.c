#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <sys/mman.h>
#include "common_threads.h"
#include "common.h"

// Function prototypes
void usage(char *);
uint32_t jenkins_one_at_a_time_hash(const uint8_t *, uint64_t);
void *child(void *);

// Global variables
int num_threads = 0;
int chunk_size = 0;
int32_t fd = 0;
char *buffer = NULL;
uint64_t fileSize = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // Initialize mutex

#define BSIZE 4096

int main(int argc, char *argv[]) {
    // Input checking
    if (argc != 3) {
        usage(argv[0]);
    }

    // Open input file
    fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("open failed");
        exit(EXIT_FAILURE);
    }

    // Use fstat to get file size
    struct stat fileInfo;
    if (fstat(fd, &fileInfo)) {
        perror("file not found.");
        exit(EXIT_FAILURE);
    }

    uint64_t fileSize = fileInfo.st_size;

    // Map file to memory
    void* arr = mmap(NULL, fileSize, PROT_READ, MAP_SHARED, fd, 0);
    if(arr == MAP_FAILED) {
      perror("mmap failed.");
      exit(EXIT_FAILURE);
    }

    // Set buffer to the mapped memory
    buffer = (char *)arr;

    double start = GetTime();

    // Calculate hash value using multithreading
    num_threads = atoi(argv[2]);
    chunk_size = fileSize / num_threads;

    pthread_t rootThread;
    pthread_create(&rootThread, NULL, child, (void *)0);

    uint32_t hash;
    pthread_join(rootThread, (void **)&hash);

    double end = GetTime();

    // Print results

    printf("num Threads = %d\n", num_threads);
    printf("hash value = %u\n", hash);
    printf("time taken = %f\n", (end - start));

    // Clean up and close file
    munmap(buffer, fileSize);
    close(fd);

    return EXIT_SUCCESS;
}

uint32_t jenkins_one_at_a_time_hash(const uint8_t *key, uint64_t length) {
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

void *child(void *ptr) {
  uint32_t n = (uint32_t)(uintptr_t)ptr;
  uint32_t hash = jenkins_one_at_a_time_hash((uint8_t *)&buffer[n * chunk_size], chunk_size);
  pthread_t thread1, thread2;
  uint32_t ptr1, ptr2;
  bool one_child= false, two_children = false;


  if (2 * n + 1 < num_threads) {
      pthread_create(&thread1, NULL, child, (void *)(uintptr_t)(2 * n + 1));
      pthread_join(thread1, (void **)&ptr1);
         one_child = true;
      if (2 * n + 2 < num_threads) {
      pthread_create(&thread2, NULL, child, (void *)(uintptr_t)(2 * n + 2));
      pthread_join(thread2, (void **)&ptr2);
      two_children = true;
      }
  }

  char temp_buffer[1000];

    pthread_mutex_lock(&mutex);

    // Concatenate hash values and calculate hash of the concatenated string
    if (two_children) {
        sprintf(temp_buffer, "%u%u%u", hash, ptr1, ptr2);
        hash = (uint32_t)jenkins_one_at_a_time_hash((uint8_t *)temp_buffer, strlen(temp_buffer));
    } else if (one_child) {
        sprintf(temp_buffer, "%u%u", hash, ptr1);
        hash = jenkins_one_at_a_time_hash((uint8_t *)temp_buffer, strlen(temp_buffer));
    }

    // Unlock mutex after modifying shared variables
    pthread_mutex_unlock(&mutex);

  // Exit the thread and return the hash value
  pthread_exit((void *)(uintptr_t)hash);
}

void usage(char *s) {
    fprintf(stderr, "Usage: %s filename num_threads\n", s);
    exit(EXIT_FAILURE);
}
