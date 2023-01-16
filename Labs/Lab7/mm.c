/*
 * lastprismer 1145141919810
 * seg fix + 去脚 + 单调 + realloc
 * 97/100, 开摆
 * Simple, 32-bit and 64-bit clean allocator based on implicit free
 * lists, first-fit placement, and boundary tag coalescing, as described
 * in the CS:APP3e text. Blocks must be aligned to doubleword (8 byte)
 * boundaries. Minimum block size is 16 bytes.
 */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memlib.h"
#include "mm.h"

/* do not change the following! */
#ifdef DRIVER
/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#endif /* def DRIVER */

/*
 * If NEXT_FIT defined use next fit search, else use first-fit search
 */
#define NEXT_FITx

/* Basic constants and macros */
#define UNALLOCATED 0                 /* 未分配 */
#define ALLOCATED 1                   /* 已分配 */
#define WSIZE 4                       /* 字长和头部 / 脚部的大小 */
#define DSIZE 8                       /* 双字长 */
#define CHUNKSIZE ((1 << 11) * 3 / 2) /* 扩展堆时的默认大小 */

#define MINUNALLOCATED 16 /* 最小的空块:头部4+脚部4+两个相对指针4= */
#define MINBLOCKSIZE 16 /* 最小的块的大小:头部4+双字8+补齐4=16 */

#define MAX(x, y) ((x) > (y) ? (x) : (y))

/* 将大小和已分配位组合起来,放入一个字中 */
#define PACK(size, prevalloc, alloc) ((size) | (prevalloc << 1) | (alloc))

/* 读取和返回参数p引用的字 */
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

/* 对块的头部或脚部操作,读取块的大小 */
#define GET_SIZE(p) (GET(p) & ~0x7)
/* 对块的头部或脚部操作,读取块的分配位 */
#define GET_ALLOC(p) (GET(p) & 0x1)
/* 对块的头部或脚部操作,读取前一个块的分配位 */
#define GET_PALLOC(p) ((GET(p) & 0x2) >> 1)

/* 给定指向块的指针,计算指向块的头部的指针 */
#define HDRP(bp) ((char *)(bp)-WSIZE)
/* 给定指向块的指针,计算指向块的脚部的指针 */
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* 给定指向块的指针,计算指向下一个块的指针 */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp)-WSIZE)))
/* 给定指向块的指针,计算指向上一个块的指针 */
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE(((char *)(bp)-DSIZE)))

/* 输入unsigned int,计算实际的指针 */
#define ABS_ADDR(value) (list_table + (unsigned long)value)
/* 输入指针,计算unsigned int */
#define RELE_ADDR(bp) (unsigned int)((char *)bp - list_table)

/* 给定指向空块的指针,计算指向PREV位置的指针 */
#define PREV(bp) ((char *)(bp))

/* 给定指向空块的指针,计算指向NEXT位置的指针 */
#define NEXT(bp) ((char *)(bp) + 4)

/* 给定指向空块的指针,指向上一空块 */
#define PREV_LKBLKP(bp) ((char *)ABS_ADDR(GET(PREV(bp))))

/* 给定指向空块的指针,指向下一空块 */
#define NEXT_LKBLKP(bp) ((char *)ABS_ADDR(GET(NEXT(bp))))

/* 迭代链表时到达终点的标志 */
#define LINKLIST_TAIL list_table

/* Global variables */
static char *heap_listp = 0; /* 指向第一个块的指针 */
static char *list_table = 0; /* 表指针 */
static char *list_table_end = 0;

#ifdef NEXT_FIT
static char *rover; /* Next fit rover */
#endif

/* Function prototypes for internal helper routines */

/*
 * extend_heap - 在堆尾部申请新空间,并将其维护成一个空块
 *
 * 参数:
 *     words:     新空间占用的字数
 *
 * 返回:
 *     void*:     指向指向新空块的指针.若发生合并,指向合并后的堆尾空块
 */
static void *extend_heap(size_t words);

/*
 * place - 在空块中进行分割,并实际进行
 *
 * 参数:
 *     bp:     指向空块的指针
 *
 *     asize:  将分配块的大小
 *
 * 返回: 无
 */
static void place(void *bp, size_t asize);

/*
 * find_fit - 寻找能放下大小为asize的块的空块
 *
 * 参数:
 *     asize:   查询块的大小
 *
 * 返回:
 *     void*:   NULL为未找到,其他为指向找到空块的指针
 */
static void *find_fit(size_t asize);

/*
 * coalesce - 带边界标记的合并.返回指向合并后得到的空块的指针,并进行链表更新
 *
 * 参数:
 *     bp:     指向空块的指针
 *
 * 返回:
 *     void*:  指向合并后的大空块的指针
 */
static void *coalesce(void *bp);

/*
 * insert_block - 插入一个块,采用LIFO策略
 *
 * 参数:
 *     bp:     指向空块的指针
 *
 * 返回: 无
 */
static void insert_block(void *bp);

/*
 * delete_block - 删除一个块,采用LIFO策略
 *
 * 参数:
 *     bp:     指向空块的指针
 *
 * 返回: 无
 */
static void delete_block(void *bp);

/*
 * search_list_head - 输入块的大小,返回对应链表的表头
 *
 * 参数:
 *     block_size:     块的大小,以字节为单位
 *
 * 返回: void*: 指向序言块前的链表head
 */
static void *search_list_head(size_t block_size);

static void printblock(int i, void *bp);
static void printlink(int line);

/*
 * mm_init - Initialize the memory manager
 */
int mm_init(void) {
  /* 创建初始化的空堆 */
  if ((heap_listp = mem_sbrk(14 * WSIZE)) == (void *)-1)
    return -1;
  // 以双字为单位
  PUT(heap_listp, 0);                // 1
  PUT(heap_listp + (1 * WSIZE), 0);  //  2-3
  PUT(heap_listp + (2 * WSIZE), 0);  //  4-7
  PUT(heap_listp + (3 * WSIZE), 0);  //  8-15
  PUT(heap_listp + (4 * WSIZE), 0);  //  16-31
  PUT(heap_listp + (5 * WSIZE), 0);  //  32-63
  PUT(heap_listp + (6 * WSIZE), 0);  //  64-127
  PUT(heap_listp + (7 * WSIZE), 0);  //  128-255
  PUT(heap_listp + (8 * WSIZE), 0);  //  256-511
  PUT(heap_listp + (9 * WSIZE), 0);  //  512-1023
  PUT(heap_listp + (10 * WSIZE), 0); //  1024--

  PUT(heap_listp + (11 * WSIZE),
      PACK(DSIZE, ALLOCATED, ALLOCATED)); /* 序言块头部 */
  PUT(heap_listp + (12 * WSIZE),
      PACK(DSIZE, ALLOCATED, ALLOCATED)); /* 序言块脚部 */
  PUT(heap_listp + (13 * WSIZE),
      PACK(0, ALLOCATED, ALLOCATED)); /* 结尾块,只有头部,已分配 */
  list_table = heap_listp;
  list_table_end = heap_listp + (11 * WSIZE);
  heap_listp += (12 * WSIZE);

  /* 将堆扩展一个块的空间 */
  if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
    return -1;
  return 0;
}

/*
 * malloc - Allocate a block with at least size bytes of payload
 */
void *malloc(size_t size) {
  size_t asize; /* 实际要分配的块的大小 */
  size_t extendsize; /* 如果没有适合的空块,需要额外扩展的堆空间的大小 */
  char *bp;

  if (heap_listp == 0) {
    mm_init();
  }
  if (size == 0)
    return NULL;

  if (size <= DSIZE)
    asize = MINBLOCKSIZE;
  else
    asize = DSIZE * ((size + (WSIZE) + (DSIZE - 1)) / DSIZE);

  if ((bp = find_fit(asize)) != NULL) {
    place(bp, asize);
    return bp;
  }
  /* 没有找到合适的空块,尝试扩展堆空间,并在新空间中分配 */
  /* 分配时大小至少是1<<13 */
  extendsize = MAX(asize, CHUNKSIZE);
  if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
    return NULL;
  place(bp, asize);
  return bp;
}

/*
 * free - Free a block
 */
void free(void *bp) {
  if (bp == 0)
    return;

  size_t size = GET_SIZE(HDRP(bp));
  size_t prevalloc = GET_PALLOC(HDRP(bp));
  size_t next_size = GET_SIZE(HDRP(NEXT_BLKP(bp)));
  size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));

  if (heap_listp == 0) {
    mm_init();
  }

  /* 设置头部和脚部 */
  PUT(HDRP(bp), PACK(size, prevalloc, UNALLOCATED));
  PUT(FTRP(bp), PACK(size, prevalloc, UNALLOCATED));

  PUT(HDRP(NEXT_BLKP(bp)), PACK(next_size, UNALLOCATED, next_alloc));
  /* 合并空闲块 */
  coalesce(bp);
}

/*
 * realloc - Naive implementation of realloc
 * 对着几千万行的checkheap看了半年,累了,结束吧
 */
void *realloc(void *ptr, size_t size) {
  if (ptr == NULL) {
    return malloc(size);
  }
  if (size == 0) {
    free(ptr);
    return 0;
  }

  size_t asize;
  if (size <= DSIZE)
    asize = MINBLOCKSIZE;
  else
    asize = DSIZE * ((size + (WSIZE) + (DSIZE - 1)) / DSIZE);

  size_t oldsize = GET_SIZE(HDRP(ptr));

  if (oldsize == asize) /* 不变 */
    return ptr;

  else if (oldsize > asize) { /* 新比旧小 */
    int prevallo = GET_PALLOC(HDRP(ptr));

    if (oldsize - asize >= MINBLOCKSIZE) { /* 能切一块出来 */
      PUT(HDRP(ptr), PACK(asize, prevallo, ALLOCATED));

      void *next = NEXT_BLKP(ptr); /* 空块 */

      PUT(HDRP(next), PACK(oldsize - asize, ALLOCATED, UNALLOCATED));
      PUT(FTRP(next), PACK(oldsize - asize, ALLOCATED, UNALLOCATED));
      void *merge = next;

      next = NEXT_BLKP(next);
      size_t next_size = GET_SIZE(HDRP(next));
      int next_alloc = GET_ALLOC(HDRP(next));
      PUT(HDRP(next), PACK(next_size, UNALLOCATED, next_alloc));
      PUT(FTRP(next), PACK(next_size, UNALLOCATED, next_alloc));

      coalesce(merge);
    } else {
      return ptr;
    }
    return ptr;
  } else {
    void *next_block = NEXT_BLKP(ptr);
    int next_alloc = GET_ALLOC(HDRP(next_block));

    if (!next_alloc && GET_SIZE(HDRP(next_block)) + oldsize > asize) {
      /* 下一个非空,且能从下一个中分出来 */
      delete_block(next_block);
      size_t ssize = GET_SIZE(HDRP(next_block)) + oldsize;
      size_t left = ssize - asize;
      int prevallo = GET_PALLOC(HDRP(ptr));

      if (left >= MINBLOCKSIZE) {
        PUT(HDRP(ptr), PACK(asize, prevallo, ALLOCATED));
        void *bp = NEXT_BLKP(ptr);
        PUT(HDRP(bp), PACK(left, ALLOCATED, 0));
        PUT(FTRP(bp), PACK(left, ALLOCATED, 0));
        insert_block(bp);
      } else {
        PUT(HDRP(ptr), PACK(ssize, prevallo, 1));
        PUT(FTRP(ptr), PACK(ssize, prevallo, 1));
        void *bp = NEXT_BLKP(ptr);
        size_t next_size = GET_SIZE(HDRP(bp));
        PUT(HDRP(bp), PACK(next_size, ALLOCATED, ALLOCATED));
      }

      return ptr;

    } else {
      char *newptr = malloc(asize);
      if (newptr == 0)
        return 0;
      memcpy(newptr, ptr, oldsize);
      free(ptr);
      return newptr;
      // }
    }
  }
}

/*
 * calloc - Allocate the block and set it to zero.
 */
void *calloc(size_t nmemb, size_t size) {
  size_t bytes = nmemb * size;
  void *newptr;

  newptr = malloc(bytes);
  memset(newptr, 0, bytes);

  return newptr;
}

static void printlink(int line) {
  printf("\n");
  if (line)
    printf("At line %d\n", line);
  printf("Linklist Information:\n");
  void *header = list_table;
  void *bp;
  for (int i = 0; i < 11; i++) {
    printf("Linklist Header: %p\tNext: %x\tsize: %d\n", header, GET(header),
           (1 << (i + 3)));
    int j = 0;
    for (bp = ABS_ADDR(GET(header)); bp != LINKLIST_TAIL;
         bp = ABS_ADDR(GET(NEXT(bp)))) {
      printf("  Link Node %d:\t %p\tNext: %x\t\tPrev: %x\t\tSize: %d\n", j++,
             bp, GET(NEXT(bp)), GET(PREV(bp)), GET_SIZE(HDRP(bp)));
    }
    header += WSIZE;
  }
  printf("\n");
}

static void printblock(int i, void *bp) {
  size_t hsize, halloc, hpalloc, fsize, falloc, fpalloc;

  // mm_checkheap(0);
  hsize = GET_SIZE(HDRP(bp));
  halloc = GET_ALLOC(HDRP(bp));
  hpalloc = GET_PALLOC(HDRP(bp));

  printf("Block %d:\n", i);
  printf("%p: header: [%ld:%c:%c] ", bp, hsize, (hpalloc ? 'a' : 'f'),
         (halloc ? 'a' : 'f'));
  // if (hsize == 24)
  //   for (int i = 0; i < 4; i++) {
  //     printf("%hhx ", *(char *)(HDRP(bp) + i));
  //   }
  if (!halloc) {
    fsize = GET_SIZE(FTRP(bp));
    falloc = GET_ALLOC(FTRP(bp));
    fpalloc = GET_PALLOC(FTRP(bp));
    printf(" footer: [%ld:%c:%c]\n", fsize, (fpalloc ? 'a' : 'f'),
           (falloc ? 'a' : 'f'));
    if ((fsize != hsize || halloc != falloc || hpalloc != fpalloc))
      printf("ERROR: header doesn't match footer!\n");
  } else
    printf("\n");
}
/*
 * mm_checkheap - Check the heap for correctness. Helpful hint: You
 *                can call this function using mm_checkheap(__LINE__);
 *                to identify the line number of the call site.
 */
void mm_checkheap(int lineno) {
  char *bp = heap_listp;
  if (lineno) {
    printf("at line:%d\n", lineno);
    printf("Heap (%p):\n", heap_listp);
  }

  if ((GET_SIZE(HDRP(heap_listp)) != DSIZE) || !GET_ALLOC(HDRP(heap_listp))) {
    printf("Bad prologue header, prologue header size:%d\n",
           GET_SIZE(HDRP(heap_listp)));
    printf("Heap (%p):\n", heap_listp);
    for (int i = 0; i < 8; i++) {
      printf("%hhx ", *(bp - WSIZE + i));
    }
    printf("\n");
  }

  if ((size_t)bp % 8)
    printf("Error: %p is not doubleword aligned\n", bp);

  int i = 0;
  for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
    // if (lineno)
    printblock(i++, bp);
    if ((size_t)bp % 8)
      printf("Error: %p is not doubleword aligned\n", bp);
  }

  if (lineno)
    printblock(i, bp);
  if ((GET_SIZE(HDRP(bp)) != 0) || !(GET_ALLOC(HDRP(bp))))
    printf("Bad epilogue header\n");
  printlink(0);
  printf("\n");
}

/*
 * The remaining routines are internal helper routines
 */

static void *extend_heap(size_t words) {
  char *bp;
  size_t size;
  size_t prev_allo;

  /* 根据传入字节数奇偶, 考虑对齐 */
  size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
  /* 分配 */
  if ((long)(bp = mem_sbrk(size)) == -1)
    return NULL;
  /* bp指向结尾块,也就是新空块 */

  prev_allo = GET_PALLOC(HDRP(bp));
  /* 设置空块的头部、脚部; */
  /* 结尾块为新空块的头部,故prev_alloc为结尾块前一个块的分配情况 */
  /* 若前块不为空,不影响;前块为空,合并时会更新空块数据,直接用宏即可 */
  PUT(HDRP(bp), PACK(size, prev_allo, UNALLOCATED));
  PUT(FTRP(bp), PACK(size, prev_allo, UNALLOCATED));
  /* 设置新的结尾块 */
  PUT(HDRP(NEXT_BLKP(bp)), PACK(0, UNALLOCATED, ALLOCATED));

  /* 如果前一个块为空,则进行合并 */
  return coalesce(bp);
}

static void *coalesce(void *bp) {
  size_t prev_alloc = GET_PALLOC(HDRP(bp));
  size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
  size_t size = GET_SIZE(HDRP(bp));
  size_t next_size;
  if (prev_alloc && next_alloc) {
    /* Case 1:前后都已分配 */

    /*     |      */
    /* |分|空|分| */

    /* 不用改后面的块 */
    insert_block(bp);
  }

  else if (prev_alloc && !next_alloc) {
    /* Case 2:前面已分配,后面是空块 */

    /*     |        */
    /* |分|空|空|分| */
    delete_block(NEXT_BLKP(bp));
    size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
    PUT(HDRP(bp), PACK(size, ALLOCATED, UNALLOCATED)); // 新头:当前的头
    PUT(FTRP(bp), PACK(size, ALLOCATED, UNALLOCATED)); // 新脚:新头+新空间的脚
    /* 大空块的后面 */
    next_size = GET_SIZE(
        HDRP(NEXT_BLKP(bp))); // 用的是新头的大小,得到的就是大空块下一个
    PUT(HDRP(NEXT_BLKP(bp)), PACK(next_size, UNALLOCATED, ALLOCATED));
    insert_block(bp);
  }

  else if (!prev_alloc && next_alloc) {
    /* Case 3:前面是空块,后面已分配 */

    /*        |     */
    /* |分|空|空|分| */
    delete_block(PREV_BLKP(bp));

    size += GET_SIZE(HDRP(PREV_BLKP(bp)));             // 新空块总大小
    PUT(FTRP(bp), PACK(size, ALLOCATED, UNALLOCATED)); // 新脚:当前的脚
    PUT(HDRP(PREV_BLKP(bp)),
        PACK(size, ALLOCATED, UNALLOCATED)); // 新头:上一个空块的头
    bp = PREV_BLKP(bp);                      // 指向这个空块
    insert_block(bp);
  }

  else {
    /* Case 4:前后都是空块 */

    /*        |        */
    /* |分|空|空|空|分| */

    delete_block(PREV_BLKP(bp));
    delete_block(NEXT_BLKP(bp));
    size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
            GET_SIZE(FTRP(NEXT_BLKP(bp))); // 新空块总大小
    PUT(HDRP(PREV_BLKP(bp)),
        PACK(size, ALLOCATED, UNALLOCATED)); // 新头:上一个的头
    PUT(FTRP(NEXT_BLKP(bp)),
        PACK(size, ALLOCATED, UNALLOCATED)); // 新脚:上一个的脚
    bp = PREV_BLKP(bp);                      // 指向这个空块
    insert_block(bp);
  }

#ifdef NEXT_FIT
  /* Make sure the rover isn't pointing into the free block */
  /* that we just coalesced */
  if ((rover > (char *)bp) && (rover < NEXT_BLKP(bp)))
    rover = bp;
#endif
  // mm_checkheap(__LINE__);
  return bp;
}

static void place(void *bp, size_t asize) {

  /* 获取当前空闲块的大小 */
  size_t csize = GET_SIZE(HDRP(bp));
  size_t next_size;
  size_t next_allo;
  delete_block(bp);
  if ((csize - asize) >= (MINBLOCKSIZE)) {
    /* 当前空闲块被分割后,剩余空间还能放下其它块 */
    /* 处理这个块 */
    PUT(HDRP(bp), PACK(asize, ALLOCATED, ALLOCATED));
    // PUT(FTRP(bp), PACK(asize, 1));

    /* 处理剩余空块 */
    bp = NEXT_BLKP(bp);
    PUT(HDRP(bp), PACK(csize - asize, ALLOCATED, UNALLOCATED));
    PUT(FTRP(bp), PACK(csize - asize, ALLOCATED, UNALLOCATED));

    insert_block(bp);
  } else {
    /* 不够分割,处理这个块 */
    PUT(HDRP(bp), PACK(csize, ALLOCATED, ALLOCATED));
    /* 处理下一个占用块 */
    next_size = GET_SIZE(HDRP(NEXT_BLKP(bp)));
    next_allo = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(next_size, ALLOCATED, next_allo));
  }
}

static void *find_fit(size_t asize) {
#ifdef NEXT_FIT
  /* Next fit search */
  char *oldrover = rover;

  /* Search from the rover to the end of list */
  for (; GET_SIZE(HDRP(rover)) > 0; rover = NEXT_BLKP(rover))
    if (!GET_ALLOC(HDRP(rover)) && (asize <= GET_SIZE(HDRP(rover))))
      return rover;

  /* search from start of list to old rover */
  for (rover = heap_listp; rover < oldrover; rover = NEXT_BLKP(rover))
    if (!GET_ALLOC(HDRP(rover)) && (asize <= GET_SIZE(HDRP(rover))))
      return rover;

  return NULL; /* no fit found */
#else
  /* 首次适配搜索策略 */
  char *head = search_list_head(asize);
  void *bp;

  /* 结束条件:结尾块是一个大小为0的块 */
  while ((char *)head < list_table_end) {
    for (bp = ABS_ADDR(GET(head)); bp != LINKLIST_TAIL;
         bp = ABS_ADDR(GET(NEXT(bp)))) {
      if (asize <= GET_SIZE(HDRP(bp))) {
        return bp;
      }
    }
    head += WSIZE;
  }

  return NULL; /* 无适配 */
#endif
}

static void insert_block(void *bp) {

  /* 维护空块单调 */
  size_t block_size = GET_SIZE(HDRP(bp));
  void *list_head = search_list_head(block_size);
  void *insert_place = list_head;
  for (void *tmp = ABS_ADDR(GET(list_head));
       tmp != LINKLIST_TAIL && block_size > GET_SIZE(HDRP(tmp));
       tmp = ABS_ADDR(GET(NEXT(tmp)))) {
    insert_place = tmp;
  };
  if (insert_place == list_head) {
    if (GET(list_head) == 0) {
      /* 链表为空 */
      PUT(PREV(bp), 0);
      PUT(NEXT(bp), 0);
      PUT(list_head, RELE_ADDR(bp));
    } else {
      PUT(PREV(bp), 0);
      PUT(NEXT(bp), GET(list_head));
      PUT(list_head, RELE_ADDR(bp));
      PUT(PREV(NEXT_LKBLKP(bp)), RELE_ADDR(bp));
    }
  } else if (ABS_ADDR(GET(NEXT(insert_place))) == LINKLIST_TAIL) {
    PUT(NEXT(insert_place), RELE_ADDR(bp));
    PUT(PREV(bp), RELE_ADDR(insert_place));
    PUT(NEXT(bp), 0);
  } else {
    PUT(NEXT(bp), GET(NEXT(insert_place)));
    PUT(PREV(bp), RELE_ADDR(insert_place));
    PUT(PREV(NEXT_LKBLKP(bp)), RELE_ADDR(bp));
    PUT(NEXT(insert_place), RELE_ADDR(bp));
  }
}

static void delete_block(void *bp) {
  size_t block_size = GET_SIZE(HDRP(bp));
  void *list_head = search_list_head(block_size);
  if (GET(PREV(bp)) == 0 && GET(NEXT(bp)) == 0) {
    /* 唯一节点 */
    PUT(list_head, 0);
  } else if (GET(PREV(bp)) != 0 && GET(NEXT(bp)) == 0) {
    /* 最后节点 */
    PUT(NEXT(PREV_LKBLKP(bp)), 0);
  } else if (GET(PREV(bp)) == 0 && GET(NEXT(bp)) != 0) {
    /* 首个节点 */
    PUT(list_head, GET(NEXT(bp)));
    PUT(PREV(NEXT_LKBLKP(bp)), 0);
  } else {
    PUT(NEXT(PREV_LKBLKP(bp)), GET(NEXT(bp)));
    PUT(PREV(NEXT_LKBLKP(bp)), GET(PREV(bp)));
  }
}

static void *search_list_head(size_t block_size) {
  /*
          3
      1        7
    0   2   5     9
           4 6   8 10
  */
  /*
   int dword_num = block_size / 8;
   if (dword_num > 15) {
     if (dword_num < 128) {
       if (dword_num < 32)
         return list_table + (4 * WSIZE); // 4
       else if (dword_num > 63)
         return list_table + (6 * WSIZE); // 6
       else
         return list_table + (5 * WSIZE); // 5
     } else if (dword_num > 255) {
       if (dword_num < 512)
         return list_table + (8 * WSIZE); // 8
       else if (dword_num > 1023)
         return list_table + (10 * WSIZE); // 10
       else
         return list_table + (9 * WSIZE); // 9
     } else
       return list_table + (7 * WSIZE); // 7
   } else if (dword_num < 8) {
     if (dword_num > 3)
       return list_table + (2 * WSIZE); // 2
     else if (dword_num == 1)
       return list_table; // 0
     else
       return list_table + (1 * WSIZE); // 1
   }
   return list_table + (3 * WSIZE); // 3
   */
  /*


  */
  int dword_num = block_size / 16; // 不是dword,懒了
  if (dword_num > 15) {
    if (dword_num < 128) {
      if (dword_num < 32)
        return list_table + (4 * WSIZE); // 4
      else if (dword_num > 63)
        return list_table + (6 * WSIZE); // 6
      else
        return list_table + (5 * WSIZE); // 5
    } else if (dword_num > 255) {
      if (dword_num < 512)
        return list_table + (8 * WSIZE); // 8
      else if (dword_num > 1023)
        return list_table + (10 * WSIZE); // 10
      else
        return list_table + (9 * WSIZE); // 9
    } else
      return list_table + (7 * WSIZE); // 7
  } else if (dword_num < 8) {
    if (dword_num > 3)
      return list_table + (2 * WSIZE); // 2
    else if (dword_num == 1)
      return list_table; // 0
    else
      return list_table + (1 * WSIZE); // 1
  }
  return list_table + (3 * WSIZE); // 3
}