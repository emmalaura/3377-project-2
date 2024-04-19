/*
Emma Gonzalez NetID: elg210004
Mateo Estrada NetID: mxe210022
*/

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
    void* arr = mmap(NULL, fileSize, PROT_READ, MAP_PRIVATE, fd, 0);
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
  uint32_t hash;
  uint32_t n = (uint32_t)(uintptr_t)ptr;
  pthread_t thread1, thread2;

  bool one_child= false, two_children = false;
  if (2 * n + 1 < num_threads) {
      pthread_create(&thread1, NULL, child, (void *)(uintptr_t)(2 * n + 1));
      one_child = true;
      if (2 * n + 2 < num_threads) {
          pthread_create(&thread2, NULL, child, (void *)(uintptr_t)(2 * n + 2));
          two_children = true;
      }
  }

  char temp_buffer[1000];

  hash = jenkins_one_at_a_time_hash((uint8_t *)&buffer[n * chunk_size], chunk_size);

  uint32_t ptr1, ptr2;

  // Join child threads and concatenate hash values
  if (one_child) {
      pthread_join(thread1, (void *)&ptr1);
  }
  if (two_children) {
      pthread_join(thread2, (void *)&ptr2);
  }

  // Concatenate hash values and calculate hash of the concatenated string
  if (two_children) {
      sprintf(temp_buffer, "%u%u%u", hash, ptr1, ptr2);
      hash = (uint32_t)jenkins_one_at_a_time_hash((uint8_t *)temp_buffer, strlen(temp_buffer));
  } else if (one_child) {
      sprintf(temp_buffer, "%u%u", hash, ptr1);
      hash = jenkins_one_at_a_time_hash((uint8_t *)temp_buffer, strlen(temp_buffer));
  }


  // Exit the thread and return the hash value
  return (void *)(uintptr_t)hash;
}

void usage(char *s) {
    fprintf(stderr, "Usage: %s filename num_threads\n", s);
    exit(EXIT_FAILURE);
}
