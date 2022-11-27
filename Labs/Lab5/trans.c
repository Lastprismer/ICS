/*
 * 姓名 学号
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */
#include "cachelab.h"
#include "contracts.h"
#include <stdio.h>
#include <stdlib.h>

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/*
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. The REQUIRES and ENSURES from 15-122 are included
 *     for your convenience. They can be removed if you like.
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N]) {
  int a0, a1, a2, a3, a4, a5, a6, a7;
  int i, j, k, l; // 用来循环
  if (N == 32) {
    for (i = 0; i < N; i += 8) {
      for (j = 0; j < M; j += 8) {
        // A[0][0],A[8][0],A[16][0],A[24][0]占用同一cache行,B[0][0]驱逐A[0][0]
        // 以8*8的小方块分块处理
        // 非对角线位置,A中miss1行8次,B中一次性miss8行1次
        // 每次对角线位置必定被驱逐,对角线位置只出现在对角线块中,对角线块额外miss7次(第一次与miss8行第一次重合)
        // 预测miss:12*(8+8)+4*(8+8+7)=284，正确
        /*
      for (k = i; k < i + 8; k++) {
        a0 = A[k][j];
        a1 = A[k][j + 1];
        a2 = A[k][j + 2];
        a3 = A[k][j + 3];
        a4 = A[k][j + 4];
        a5 = A[k][j + 5];
        a6 = A[k][j + 6];
        a7 = A[k][j + 7];

        B[j][k] = a0;
        B[j + 1][k] = a1;
        B[j + 2][k] = a2;
        B[j + 3][k] = a3;
        B[j + 4][k] = a4;
        B[j + 5][k] = a5;
        B[j + 6][k] = a6;
        B[j + 7][k] = a7;
      }
        */
        // 做完64再回来优化:对角线块原地转置
        // 排除对角线块内的额外miss,应该是16*(8+8)=256次
        if (i != j) {
          for (k = i; k < i + 8; k++) {
            a0 = A[k][j];
            a1 = A[k][j + 1];
            a2 = A[k][j + 2];
            a3 = A[k][j + 3];
            a4 = A[k][j + 4];
            a5 = A[k][j + 5];
            a6 = A[k][j + 6];
            a7 = A[k][j + 7];

            B[j][k] = a0;
            B[j + 1][k] = a1;
            B[j + 2][k] = a2;
            B[j + 3][k] = a3;
            B[j + 4][k] = a4;
            B[j + 5][k] = a5;
            B[j + 6][k] = a6;
            B[j + 7][k] = a7;
          }
        } else {
          for (k = i; k < i + 8; k++) {
            a0 = A[k][j];
            a1 = A[k][j + 1];
            a2 = A[k][j + 2];
            a3 = A[k][j + 3];
            a4 = A[k][j + 4];
            a5 = A[k][j + 5];
            a6 = A[k][j + 6];
            a7 = A[k][j + 7];

            B[k][j] = a0;
            B[k][j + 1] = a1;
            B[k][j + 2] = a2;
            B[k][j + 3] = a3;
            B[k][j + 4] = a4;
            B[k][j + 5] = a5;
            B[k][j + 6] = a6;
            B[k][j + 7] = a7;
            // 之后全命中
          }
          for (k = i; k < i + 8; k++) {
            // 斜三角
            for (l = j + k - i + 1; l < j + 8; l++) {
              a0 = B[k][l];
              B[k][l] = B[l][k];
              B[l][k] = a0;
            }
          }
        }
      }
    }
  } else if (N == 64) {
    // A[0][0],A[4][0],A[8][0]...占用同一cache行,B[0][0]驱逐A[0][0]
    // 那就一次传8*8，但以4行分小块，两次转置最后单独处理
    // 懒得誊草稿纸了
    for (i = 0; i < 64; i += 8) {
      for (j = 0; j < 64; j += 8) {
        // 第一块
        for (k = j; k < j + 4; k++) {
          // 8*1为单位
          a0 = A[k][i];
          a1 = A[k][i + 1];
          a2 = A[k][i + 2];
          a3 = A[k][i + 3];
          a4 = A[k][i + 4];
          a5 = A[k][i + 5];
          a6 = A[k][i + 6];
          a7 = A[k][i + 7];
          // 转进两个1*4位空,卡住cache
          // 原地转置，因为必定miss
          B[i][k] = a0;
          B[i + 1][k] = a1;
          B[i + 2][k] = a2;
          B[i + 3][k] = a3;
          // 这里先不一步到位,就不会miss
          B[i][k + 4] = a4;
          B[i + 1][k + 4] = a5;
          B[i + 2][k + 4] = a6;
          B[i + 3][k + 4] = a7;
        }

        for (k = i; k < i + 4; k++) {
          // 处理左下块，借助左下块的置换正位亏4miss但赚4miss
          a0 = A[j + 4][k];
          a1 = A[j + 5][k];
          a2 = A[j + 6][k];
          a3 = A[j + 7][k];
          // 相同的方法,先卡cache以处理上一层

          a4 = B[k][j + 4];
          a5 = B[k][j + 5];
          a6 = B[k][j + 6];
          a7 = B[k][j + 7];
          // 左下块置换
          B[k][j + 4] = a0;
          B[k][j + 5] = a1;
          B[k][j + 6] = a2;
          B[k][j + 7] = a3;

          B[k + 4][j] = a4;
          B[k + 4][j + 1] = a5;
          B[k + 4][j + 2] = a6;
          B[k + 4][j + 3] = a7;
        }
        for (k = i + 4; k < i + 8; k++) {
          // 最简单的
          a0 = A[j + 4][k];
          a1 = A[j + 5][k];
          a2 = A[j + 6][k];
          a3 = A[j + 7][k];

          B[k][j + 4] = a0;
          B[k][j + 5] = a1;
          B[k][j + 6] = a2;
          B[k][j + 7] = a3;
        }
      }
    }
  } else if (N == 68) {
    // 草，4*4没过，猜到16*4就最低了，有意思捏
    for (i = 0; i < N; i += 16) {
      for (j = 0; j < M; j += 4) {
        for (k = i; k < i + 16 && k < N; k++) {
          a0 = A[k][j];
          a1 = A[k][j + 1];
          a2 = A[k][j + 2];
          a3 = A[k][j + 3];

          B[j][k] = a0;
          B[j + 1][k] = a1;
          B[j + 2][k] = a2;
          B[j + 3][k] = a3;
        }
      }
    }
  }
}

/*
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started.
 */

/*
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N]) {
  int i, j, tmp;

  REQUIRES(M > 0);
  REQUIRES(N > 0);

  for (i = 0; i < N; i++) {
    for (j = 0; j < M; j++) {
      tmp = A[i][j];
      B[j][i] = tmp;
    }
  }

  ENSURES(is_transpose(M, N, A, B));
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions() {
  /* Register your solution function */
  registerTransFunction(transpose_submit, transpose_submit_desc);

  /* Register any additional transpose functions */
  registerTransFunction(trans, trans_desc);
}

/*
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N]) {
  int i, j;

  for (i = 0; i < N; i++) {
    for (j = 0; j < M; ++j) {
      if (A[i][j] != B[j][i]) {
        return 0;
      }
    }
  }
  return 1;
}
