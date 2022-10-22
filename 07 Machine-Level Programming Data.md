# Machine-Level Programming IV Data



## 数组

### 一、一维数组

基本原则：

* 数组声明：数据类型`T`，整型常数`L`：`T a[L]`

  效果：

  1. 在内存中分配一个`L * sizeof(T)`的连续空间
  2. 标识符`a`：可以用`a`来作为指向数组开头的指针，`Type T*`

指针运算

* `*&a`和`a`等价 $\Leftrightarrow$ `a[i]`和`*(a + i)`等价
* 返回数组值的操作类型为`int`，涉及4字节操作；返回指针操作类型为`int*`，涉及8字节操作



### 二、嵌套数组

声明：`T a[r][c]`，二维数组，r行c列

* 占用空间：`r * c * sizeof(T)`

* 顺序：行优先存储（同一行中的连续元素位于连续地址）

* 数组元素`a[i][j]`地址：`&a[i][j] = a + sizeof(T) * (i * c + j)`

  ```assembly
  # a in %rdi, i in %rsi, j in %rdx, T is int, column is 3
  leaq (%rsi, %rsi, 2), %rax # 3i
  leaq (%rdi, %rax, 4), %rax # a+12i
  movl (%rax, %rdx, 4), %eax # M[a+12i+4j]
  
  ```

编译器优化：利用访问模式的规律性，优化索引计算

源代码：计算两矩阵乘积得到的新矩阵的`(i,j)`元的值：

```c
#define N 16
typedef int matrix[N][N];
int fix_prod_ele (matrix A, matrix B, long i, long k)
{
    long j;
    int result = 0;
    for (j = 0; j < N; j++)
        result += A[i][j] * B[j][k];
    return result;
}
```

```c
// 优化后：
int fix_prod_ele_opt (matrix A, matrix B, long i, long k)
{
    int *Aptr = &A[i][0];	// 指向A的第i行
    int *Bptr = &B[0][k];	// 指向B的第k列
    int *Bend = &B[N][k];	// Bptr的结尾
    int result = 0;
    do
    {
        result += *Aptr * *Bptr;
        Aptr++;
        Bptr += N;
    } while (Bptr != Bend);
    return result;
}
```





### 三、变长数组

可以将数组声明为：`T a[expr1][expr2]`

例：

```c
int var_ele (long n, int A[n][n], long i, long j)
{
    return A[i][j];
}
```

```assembly
imulq	%rdx, %rdi				# n*i
leaq	(%rsi, %rdi, 4), %rax	 # xA+4(n*i)
movl	(%rax, %rcx, 4), %eax    # 读地址 
ret
```

动态的版本必须用乘法指令对`i`伸缩`n`倍，而不能用一系列移位和加法，在一些处理器中乘法会带来严重的性能处罚。





## 结构体

* 存放在内存中一段**连续**的区域里，指向区域的指针就是结构第一个字节的地址。
* 根据字段声明进行排序（内存中）
* 编译器维护关于每个结构类型的信息，指示每个字段的字节偏移；以这些偏移作为内存引用指令中的位移，产生对结构元素的引用。
* 结构的各个字段的选取完全是在编译时处理的。机器代码不包含关于字段声明或字段名字的信息。

例：

```c
struct s1
{
    char c;
    int i[2];
    double v;
} *p;

/*
结构体内存占用示意——数据不对齐
------------	高地址
| v      8 | <- p+9
| i[2]   4 | <- p+5
| i[0]   4 | <- p+1
| c      1 | <- p
------------	低地址
*/
```





## 数据对齐

* 某种类型对象的地址必须是某个值 K （通常是2，4或8）的倍数，结构体总占用必须是 K 的倍数

* 原始的数据（表项）必须占 K 个字节

* K 为任意字段的最大字节数

* 可能进行末尾填充

* 某些机器上必须使用（某些型号的Intel和AMD处理器对于实现多媒体操作的SSE指令，对16字节的数据块进行操作，内存地址必须是16的倍数）；建议在x86-64上使用

  如：

  ```c
  struct s2
  {
      char c;
      int i[2];
      double v;
  } *p;
  
  /*
  结构体内存占用示意——数据对齐
  .align 4
  ---------------	高地址
  | v         8 | <- p+16
  | nothing   4 | 为了让v的地址为8的倍数，空置的内存
  | i[1]      4 | <- p+8
  | i[0]      4 | <- p+4, 4为4的倍数
  | nothing   3 | 为了让i[0]的地址为4的倍数，空置的内存
  | c         1 | <- p
  ---------------	低地址
  */
  ```



作用：

* 以4或8字节为单位读取内存更加容易（系统特性），提高效率

  缺点：占用内存更大、存储某些类型时不便

例：在x86-64中，地址：

* `char`没有特殊要求
* `short`地址低1位必须为 $0_2$
* `int, float`地址低2位必须为 $00_2$
* `double, long, void*`地址低3位必须为 $000_2$
* `long double`（gcc on linux）地址低4位必须为 $0000_2$



为了节省内存，应该把大的数据定义在前面：

```c
struct s3
{
    char c;
    int i;
    char d;
} *p3; // 补充6个字节，共12

struct s4
{
    int i;
    char c;
    char d;
} *p4; // 补充2个字节，共8
```

栈指针：和16对齐







## 浮点数

处理器的 **浮点体系结构**：

* 通过某种寄存器，存储和访问浮点数值
* 对浮点数据操作
* 向函数传递浮点数参数，从函数返回浮点数结果
* 函数调用过程中保存寄存器的规则



### 一、浮点寄存器和指令类型

<img src=".\Images\07-FP Registers.png" style="zoom:50%;" />

以 AVX 为例，AVX 浮点体系结构允许数据存储在16个YMM寄存器中，名称为 `%ymm0 ~ %ymm15`，每个YMM寄存器都是32字节。

对**标量数据**（用`s`表示）操作时，寄存器只保存浮点数，且只使用低32位（float）或低64位（double），汇编代码用寄存器的 SSE XMM 寄存器名字 `%xmm0 ~ %xmm15`来引用它们，每个 XMM 寄存器都是对应的 YMM 寄存器的低16字节。

向量操作：**SIMD**（用`p`表示）：对整个寄存器（4个float，2个double）同时操作

例：`vmovapd`：对齐的向量双精度传送

<img src=".\Images\07-FP Mov.png" style="zoom:75%;" />

传送指令中内存引用方式和整数MOV的指令一样

* `addss`：标量单精度加法



### 二、内存引用

* 整型和指针在传统寄存器中存储

* 浮点型在 XMM 寄存器中存储传送

  例：

  ```c
  double dincr (double *p, double v)
  {
      double x = *p;
      *p = x + v;
      return x;
  }
  ```

  ```assembly
  # p in %rdi, v in %xmm0
  movapd		%xmm0, %xmm1	# 复制v
  movsd		(%rdi), %xmm0	# x = *p
  addsd		%xmm0, %xmm1	# t = x + v
  movsd		%xmm1, (%rsi)	# *p = t
  ret
  ```

  

### 三、希望不会考

* 更多的指令
* 浮点比较：
  * `ucomiss`和`ucomisd`指令
  * 设置标志：ZF、CF、PF
    * PF，确认NaN
* 常量使用
  * 将 `%xmm0`设置为0：`xorpd %xmm0, %xmm0`
  * 其他的，从内存中读取