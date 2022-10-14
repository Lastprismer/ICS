/* 
 * CS:APP Data Lab 
 * 
 * <Please put your name and userid here>
 * 
 * bits.c - Source file with your solutions to the Lab.
 *          This is the file you will hand in to your instructor.
 *
 * WARNING: Do not include the <stdio.h> header; it confuses the dlc
 * compiler. You can still use printf for debugging without including
 * <stdio.h>, although you might get a compiler warning. In general,
 * it's not good practice to ignore compiler warnings, but in this
 * case it's OK.  
 */

#if 0
/*
 * Instructions to Students:
 *
 * STEP 1: Read the following instructions carefully.
 */

You will provide your solution to the Data Lab by
editing the collection of functions in this source file.

INTEGER CODING RULES:
 
  Replace the "return" statement in each function with one
  or more lines of C code that implements the function. Your code 
  must conform to the following style:
 
  int Funct(arg1, arg2, ...) {
	  /* brief description of how your implementation works */
	  int var1 = Expr1;
	  ...
	  int varM = ExprM;

	  varJ = ExprJ;
	  ...
	  varN = ExprN;
	  return ExprR;
  }

  Each "Expr" is an expression using ONLY the following:
  1. Integer constants 0 through 255 (0xFF), inclusive. You are
	  not allowed to use big constants such as 0xffffffff.
  2. Function arguments and local variables (no global variables).
  3. Unary integer operations ! ~
  4. Binary integer operations & ^ | + << >>
	
  Some of the problems restrict the set of allowed operators even further.
  Each "Expr" may consist of multiple operators. You are not restricted to
  one operator per line.

  You are expressly forbidden to:
  1. Use any control constructs such as if, do, while, for, switch, etc.
  2. Define or use any macros.
  3. Define any additional functions in this file.
  4. Call any functions.
  5. Use any other operations, such as &&, ||, -, or ?:
  6. Use any form of casting.
  7. Use any data type other than int.  This implies that you
	 cannot use arrays, structs, or unions.

 
  You may assume that your machine:
  1. Uses 2s complement, 32-bit representations of integers.
  2. Performs right shifts arithmetically.
  3. Has unpredictable behavior when shifting an integer by more
	 than the word size.

EXAMPLES OF ACCEPTABLE CODING STYLE:
  /*
   * pow2plus1 - returns 2^x + 1, where 0 <= x <= 31
   */
  int pow2plus1(int x) {
	 /* exploit ability of shifts to compute powers of 2 */
	 return (1 << x) + 1;
  }

  /*
   * pow2plus4 - returns 2^x + 4, where 0 <= x <= 31
   */
  int pow2plus4(int x) {
	 /* exploit ability of shifts to compute powers of 2 */
	 int result = (1 << x);
	 result += 4;
	 return result;
  }

FLOATING POINT CODING RULES

For the problems that require you to implent floating-point operations,
the coding rules are less strict.  You are allowed to use looping and
conditional control.  You are allowed to use both ints and unsigneds.
You can use arbitrary integer and unsigned constants.

You are expressly forbidden to:
  1. Define or use any macros.
  2. Define any additional functions in this file.
  3. Call any functions.
  4. Use any form of casting.
  5. Use any data type other than int or unsigned.  This means that you
	 cannot use arrays, structs, or unions.
  6. Use any floating point data types, operations, or constants.


NOTES:
  1. Use the dlc (data lab checker) compiler (described in the handout) to 
	 check the legality of your solutions.
  2. Each function has a maximum number of operators (! ~ & ^ | + << >>)
	 that you are allowed to use for your implementation of the function. 
	 The max operator count is checked by dlc. Note that '=' is not 
	 counted; you may use as many of these as you want without penalty.
  3. Use the btest test harness to check your functions for correctness.
  4. Use the BDD checker to formally verify your functions
  5. The maximum number of ops for each function is given in the
	 header comment for each function. If there are any inconsistencies 
	 between the maximum ops in the writeup and in this file, consider
	 this file the authoritative source.

/*
 * STEP 2: Modify the following functions according the coding rules.
 * 
 *   IMPORTANT. TO AVOID GRADING SURPRISES:
 *   1. Use the dlc compiler to check that your solutions conform
 *      to the coding rules.
 *   2. Use the BDD checker to formally verify that your solutions produce 
 *      the correct answers.
 */


#endif
/* Copyright (C) 1991-2022 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */
/* This header is separate from features.h so that the compiler can
   include it implicitly at the start of every compilation.  It must
   not itself include <features.h> or any other header that includes
   <features.h> because the implicit include comes before any feature
   test macros that may be defined in a source file before it first
   explicitly includes a system header.  GCC knows the name of this
   header in order to preinclude it.  */
/* glibc's intent is to support the IEC 559 math functionality, real
   and complex.  If the GCC (4.9 and later) predefined macros
   specifying compiler intent are available, use them to determine
   whether the overall intent is to support these features; otherwise,
   presume an older compiler has intent to support these features and
   define these macros by default.  */
/* wchar_t uses Unicode 10.0.0.  Version 10.0 of the Unicode Standard is
   synchronized with ISO/IEC 10646:2017, fifth edition, plus
   the following additions from Amendment 1 to the fifth edition:
   - 56 emoji characters
   - 285 hentaigana
   - 3 additional Zanabazar Square characters */
/* 
 * bitAnd - x&y using only ~ and | 
 *   Example: bitAnd(6, 5) = 4
 *   Legal ops: ~ |
 *   Max ops: 8
 *   Rating: 1
 */
int bitAnd(int x, int y) {
  /* 真值表 */
  return ~((~x) | (~y));
}
/* 
 * bitConditional - x ? y : z for each bit respectively
 *   Example: bitConditional(0b00110011, 0b01010101, 0b00001111) = 0b00011101
 *   Legal ops: & | ^ ~
 *   Max ops: 8
 *   Rating: 1
 */
int bitConditional(int x, int y, int z) {
  /* 模拟半加器 */
  // 好家伙3ops是什么鬼
  // 好家伙不如搜索
  // return (x&y)^(z&(~x));
  return z^(x&(y^z));
  // nb
}
/* 
 * byteSwap - swaps the nth byte and the mth byte
 *  Examples: byteSwap(0x12345678, 1, 3) = 0x56341278
 *            byteSwap(0xDEADBEEF, 0, 2) = 0xDEEFBEAD
 *  You may assume that 0 <= n <= 3, 0 <= m <= 3
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 25
 *  Rating: 2
 */
int byteSwap(int x, int n, int m) {
  /* 简单的异或 */
  int n_move = n << 3;
  int m_move = m << 3;
  int n_byte_of_x = x >> m_move;
  int m_byte_of_x = x >> n_move;
  int t = (n_byte_of_x ^ m_byte_of_x) & 0xff;
  int another_x = (t << m_move) | (t << n_move);
  x = x ^ another_x;
  return x;
}
/* 
 * logicalShift - shift x to the right by n, using a logical shift
 *   Can assume that 0 <= n <= 31
 *   Examples: logicalShift(0x87654321,4) = 0x08765432
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 20
 *   Rating: 3 
 */
int logicalShift(int x, int n) {
  /* mask的构造 */
  int shifted = x >> n;
  // int mask = ~(((~0 << 1) << (32 + ~n)));  
  int mask = ~(((1 << 31) >> n) << 1);
  return shifted & mask;
}
/* 
 * cleanConsecutive1 - change any consecutive 1 to zeros in the binary form of x.
 *   Consecutive 1 means a set of 1 that contains more than one 1.
 *   Examples cleanConsecutive1(0x10) = 0x10
 *            cleanConsecutive1(0xF0) = 0x0
 *            cleanConsecutive1(0xFFFF0001) = 0x1
 *            cleanConsecutive1(0x4F4F4F4F) = 0x40404040
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 25
 *   Rating: 4
 */
int cleanConsecutive1(int x){
  /* 寻找到010结构并保存，其他的全部删掉欸嘿嘿，向左抖一下再向右抖一下，写个真值表，把1匀出来 */
  int rightshifted = x >> 1;
  int rightmask = ~((~0 << 1) << 30);
  int rightshift = rightshifted & rightmask;
  int leftshift = x << 1;
  return ~(~x | leftshift | rightshift);
}
/* 
 * countTrailingZero - return the number of consecutive 0 from the lowest bit of 
 *   the binary form of x.
 *   YOU MAY USE BIG CONST IN THIS PROBLEM, LIKE 0xFFFF0000
 *   YOU MAY USE BIG CONST IN THIS PROBLEM, LIKE 0xFFFF0000
 *   YOU MAY USE BIG CONST IN THIS PROBLEM, LIKE 0xFFFF0000
 *   Examples countTrailingZero(0x0) = 32, countTrailingZero(0x1) = 0,
 *            countTrailingZero(0xFFFF0000) = 16,
 *            countTrailingZero(0xFFFFFFF0) = 8,
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 40
 *   Rating: 4
 */
int countTrailingZero(int x){
  /* 分治法 */
  int masked = !(x & 0x0000ffff) << 4;
  int ans = 0;
  ans = ans + masked;
  x = x >> masked;

  masked = !(x & 0x000000ff) << 3;
  ans = ans + masked;
  x = x >> masked;

  masked = !(x & 0x0000000f) << 2;
  ans = ans + masked;
  x = x >> masked;

  masked = !(x & 0x00000003) << 1;
  ans = ans + masked;
  x = x >> masked;

  masked = !(x & 0x00000001);
  ans = ans + masked;
  x = x >> masked;
  return ans + !x;
}
/* 
 * divpwr2 - Compute x/(2^n), for 0 <= n <= 30
 *  Round toward zero
 *   Examples: divpwr2(15,1) = 7, divpwr2(-33,4) = -2
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 2
 */
int divpwr2(int x, int n) {
  /* 判断符号 */
  /* 异或，好 */
  int sig = (x >> 31) & 1;
  // int odd = (x << (33 + ~n));
  // return (x >> n) + ((!!(n & odd)) & sig);
  int rightshift = x >> n;
  int another_x = rightshift << n;
  int leftnums = another_x ^ x;
  return rightshift + (sig & !!leftnums); 
}
/* 
 * oneMoreThan - return 1 if y is one more than x, and 0 otherwise
 *   Examples oneMoreThan(0, 1) = 1, oneMoreThan(-1, 1) = 0
 *   Legal ops: ~ & ! ^ | + << >>
 *   Max ops: 15
 *   Rating: 2
 */
int oneMoreThan(int x, int y) {
  /* 溢出？y=x+1
  分符号讨论：
	  y+ x- ~y-：只有0=-1+1
	  y- x+ ~y+：不可能
	  y+ x+ ~y-：考虑最大成立时，0x7fffffff=0x7ffffffe+1，0x7fffffff+~0x7ffffffe=0，不溢出
	  y- x- ~y+：同理，0x80000001=0x80000000+1，0x80000001+~0x80000000=0，不溢出
	  同号时成立必不溢出（y+~x即可判定），异号时不存在溢出成立的情况
	  ~y和x
  */
  int may_overflow = !(y + ~x);
  int if_overflow_then_x_and_y_cannot_reach_the_condition_so_its_COMMON_SIGN = (~y | x) >> 31;
  return may_overflow & if_overflow_then_x_and_y_cannot_reach_the_condition_so_its_COMMON_SIGN;
}
/*
 * satMul3 - multiplies by 3, saturating to Tmin or Tmax if overflow
 *  Examples: satMul3(0x10000000) = 0x30000000
 *            satMul3(0x30000000) = 0x7FFFFFFF (Saturate to TMax)
 *            satMul3(0x70000000) = 0x7FFFFFFF (Saturate to TMax)
 *            satMul3(0xD0000000) = 0x80000000 (Saturate to TMin)
 *            satMul3(0xA0000000) = 0x80000000 (Saturate to TMin)
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 25
 *  Rating: 3
 */
int satMul3(int x) {
  /* 多判断几次就行，先判是否同号，再判正负 */  
  int x2 = x << 1;
  int x3 = x + x2;
  int full_sign_x = x >> 31;
  int full_sign_x2 = x2 >> 31;
  int full_sign_x3 = x3 >> 31;
  int valid_sign = ((full_sign_x ^ full_sign_x2) | (full_sign_x ^ full_sign_x3)) >> 31; // all 0 is valid
  int int_min = 1 << 31;
  int ans = ((~valid_sign) & x3) | (valid_sign & ((full_sign_x & int_min) | ((~full_sign_x) & (~int_min))));
	return ans;
}
/* 
 * subOK - Determine if can compute x-y without overflow
 *   Example: subOK(0x80000000,0x80000000) = 1,
 *            subOK(0x80000000,0x70000000) = 0, 
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 20
 *   Rating: 3
 */
int subOK(int x, int y) {
  /* 判断符号 */
  // x+ y+ ok
  // x+ y- sign is +
  // x- y+ sign is -
  // x- y- ok
  // int has_the_same_sign = ((x ^ y) >> 31) & 1; // 0 is same
  // int minus = x + ~y + 1;
  // int sign_minus = (minus >> 31) & 1;
  // int valid_sign = (sign_minus ^ ((x >> 31) & 1)); // 0 is valid
  // return !(has_the_same_sign & valid_sign);
  int has_the_same_sign = x ^ y; // 0 is same
  int minus = x + ~y + 1;
  int sign_minus = minus ^ x;
  return !((has_the_same_sign & sign_minus) >> 31);
}
/* 
 * isLessOrEqual - if x <= y  then return 1, else return 0 
 *   Example: isLessOrEqual(4,5) = 1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
int isLessOrEqual(int x, int y) {
  /* 同上题 */  
  // x+ y+ y-x
  // x+ y- no
  // x- y+ yes
  // x- y- y-x
  int minus = y + (~x + 1);
  int minus_sign = minus >> 31;
  int x_is_neg_and_y_is_pos = (~x | y) >> 31; // 0 is true
  int same_sign = (x ^ y) >> 31; // 0 is same
  return (!x_is_neg_and_y_is_pos) | (!same_sign & !minus_sign);
}
/*
 * trueThreeFourths - multiplies by 3/4 rounding toward 0,
 *   avoiding errors due to overflow
 *   Examples: trueThreeFourths(11) = 8
 *             trueThreeFourths(-9) = -6
 *             trueThreeFourths(1073741824) = 805306368 (no overflow)
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 20
 *   Rating: 4
 */
int trueThreeFourths(int x)
{
  /* x-x/4，向0取整=>x/4向inf取整 */
  int divpw2_but_round_to_inf_for_negs = x >> 2;
  int sign = x >> 31;
  int minus_one_more =  ~sign & x & 3; // neg and not divisible
  return (x + ~(divpw2_but_round_to_inf_for_negs + !!minus_one_more) + 1);
}
/* 
 * float_twice - Return bit-level equivalent of expression 2*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned float_twice(unsigned uf) {
  /* 拆分，然后分情况讨论 */
  unsigned mask_nan = 0x7F800000;
  unsigned e = uf & mask_nan; // 保留阶码
  unsigned sign = uf & 0x80000000;
  unsigned f = uf & 0x007FFFFF;
  unsigned is_nan = e ^ mask_nan; // 0 is nan
  if (!is_nan)
    return uf;
  if (!e) // 为非规格
    return (uf << 1) | sign;
  e = e + 0x00800000; // 规格
  if (!(e ^ mask_nan)) // become inf
    return 0x7F800000 | sign;
  return sign | e | f;
}
/* 
 * float_i2f - Return bit-level equivalent of expression (float) x
 *   Result is returned as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point values.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned float_i2f(int x) {  
  /* 四舍六入五成双，修了半天五的bug，发现原来是人家都成双了，我还没有，怪不得修不好，我是废物 */
  unsigned sign = (x >> 31) & 1;
  unsigned E = 0;
  unsigned f = 0;
  unsigned e = 0;

  int bias = 127;
  int first_one_count_from_left = 0;
  int to_be_deleted;
  int bad;
  unsigned temp_x = x;

  if (x == 0)
      return 0;
  if (x < 0)
  {
      sign = 1;
      temp_x = ~x + 1;
  }
  while (temp_x < 0x80000000)
  {
      first_one_count_from_left++;
      temp_x <<= 1;
  }

  // turn x to 2^(E)*(1+f), where E=x^(30-first_one_count_from_left) and f is the left part of temp_x
  E = 31 - first_one_count_from_left;
  e = E + bias;
  f = temp_x << 1;

  f >>= 1; // 防溢出
  to_be_deleted = f & 0xff;
  bad = f & 0x100;
  if (to_be_deleted > 0x80) // 进位
      f += 0x100;
  else if (to_be_deleted == 0x80)
  {
    if (bad & 0x100) // 五，偶
      f += 0x100;
  }

  e <<= 23;
  sign <<= 31;
  f >>= 8;
  return sign + e + f;
}
/* 
 * float_f2i - Return bit-level equivalent of expression (int) f
 *   for floating point argument f.
 *   Argument is passed as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point value.
 *   Anything out of range (including NaN and infinity) should return
 *   0x80000000u.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
int float_f2i(unsigned uf) {
  /* 还好.jpg */
  unsigned sign = uf & 0x80000000;
  unsigned e = (uf & 0x7f800000) >> 23;
  unsigned f = uf & 0x007fffff;
  int bias = 127;
  if (e < bias) return 0; // 小
  e -= bias;
  if (e > 30) return 0x80000000; // 大或inf或nan
  f = ((f << 7) >> (30 - e)) + (1 << e);
  if (sign) return -f;
  else return f;
  // 0000 0001 0111 1111 1111 1111 1111 1111
}
/* 
 * float_pwr2 - Return bit-level equivalent of the expression 2.0^x
 *   (2.0 raised to the power x) for any 32-bit integer x.
 *
 *   The unsigned value that is returned should have the identical bit
 *   representation as the single-precision floating-point number 2.0^x.
 *   If the result is too small to be represented as a denorm, return
 *   0. If too large, return +INF.
 * 
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. Also if, while 
 *   Max ops: 30 
 *   Rating: 4
 */
unsigned float_pwr2(int x) {
  /* 一般 */
  if (x > 127)
  {
    return 0x7f800000;
  }
  else if (x < -149)
  {
    return 0;
  }
  else if (x < -126) // 尾码中
  {
    return (1 << (149 + x));
  }
  else return (x + 127) << 23; // 阶码中
}
