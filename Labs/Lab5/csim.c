/*

姓名 学号

*/
#include "cachelab.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TRUE 1
#define FALSE 0
#define FILE_PATH_LENGTH 20
#define CLEAR 0xffffffffffffffff
#define INF 0x3f3f3f3f

typedef int BOOL;
typedef unsigned long ul;
typedef struct {
  ul addr;
  int size;
  char op;
} OpCode;
typedef struct {
  ul line_tag;
  int last_usage;
  BOOL valid_tag;
} CacheLine;
typedef CacheLine *CacheSet;
typedef CacheSet *Cache;

int S, s;
int E;
int b;
int t;
BOOL v;
char test_file_path[FILE_PATH_LENGTH];

int hit_count = 0;
int miss_count = 0;
int eviction_count = 0;
int cycle_count = 0;
Cache cache = NULL;

void input(int _argc, char **_argv);
void init();
void init_cache();
void free_cache();
void run_cache();
void update_cache(ul addr);

void update_cache(ul addr) {
  unsigned int set_id = (unsigned)((addr << t) >> (b + t));
  ul this_tag = addr >> (s + b);
  CacheSet workingSet = cache[set_id];
  cycle_count++;

  // 1. 寻找是否匹配现有行
  for (int i = 0; i < E; i++) {
    if (workingSet[i].line_tag == this_tag) {
      workingSet[i].last_usage = cycle_count;
      hit_count++;
      if (v)
        printf("hit\n");
      return;
    }
  }

  // 2. 寻找是否有空行
  for (int i = 0; i < E; i++) {
    if (!workingSet[i].valid_tag) {
      workingSet[i].valid_tag = TRUE;
      workingSet[i].last_usage = cycle_count;
      workingSet[i].line_tag = this_tag;
      miss_count++;
      if (v)
        printf("miss\n");
      return;
    }
  }

  // 3. 驱逐现有行
  miss_count++;
  eviction_count++;
  int lru_num = INF;
  int lru_id = -1;
  for (int i = 0; i < E; i++) {
    if (workingSet[i].last_usage >= 0 && workingSet[i].valid_tag &&
        workingSet[i].last_usage < lru_num) {
      lru_id = i;
      lru_num = workingSet[i].last_usage;
    }
  }
  workingSet[lru_id].line_tag = this_tag;
  workingSet[lru_id].last_usage = cycle_count;
  if (v)
    printf("miss eviction\n");
}

void input(int _argc, char **_argv) {
  // 处理命令行参数
  int opt;
  while ((opt = getopt(_argc, _argv, "v::E:s:b:t:")) != -1) {
    switch (opt) {
    case 'E':
      E = atoi(optarg);
      break;
    case 's':
      s = atoi(optarg);
      break;
    case 'b':
      b = atoi(optarg);
      break;
    case 't':
      strncpy(test_file_path, optarg, FILE_PATH_LENGTH);
      break;
    case 'v':
      v = TRUE;
      break;
    }
  }
}

void init() {
  // 参数初始化
  t = 64 - b - s;
  S = 1 << s;

  // 检查文件是否可用
  FILE *fp;
  fp = fopen(test_file_path, "r");
  if (fp == NULL)
    exit(1);
  fclose(fp);

  init_cache();
}

void init_cache() {
  // cache初始化
  cache = (CacheSet *)malloc(sizeof(CacheSet) * S);

  for (int i = 0; i < S; i++) {
    cache[i] = (CacheSet)malloc(sizeof(CacheLine) * E);
    for (int j = 0; j < E; j++) {
      cache[i][j].valid_tag = FALSE;
      cache[i][j].line_tag = CLEAR;
      cache[i][j].last_usage = -1;
    }
  }
}

void free_cache() {
  // 释放cache空间
  for (int i = 0; i < S; ++i)
    free(cache[i]);
  free(cache);
}

void run_cache() {
  // 循环模拟
  char op;
  ul addr;
  int size;

  FILE *fp;
  fp = fopen(test_file_path, "r");
  if (fp == NULL)
    exit(1);

  while (fscanf(fp, " %c %lx,%d\n", &op, &addr, &size) != -1) {
    if (v)
      printf("%c %lx,%d ", op, addr, size);
    if (op == 'L' || op == 'S')
      update_cache(addr);
    else if (op == 'M') {
      update_cache(addr);
      update_cache(addr);
    }
  }
  fclose(fp);
}

int main(int argc, char *argv[]) {
  input(argc, argv);
  init();
  run_cache();
  free_cache();
  printSummary(hit_count, miss_count, eviction_count);
  return 0;
}