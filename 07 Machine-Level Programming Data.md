# Machine-Level Programming Data



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
imulq	%rdx, %rdi
leaq	(%rsi, %rdi, 4), %rax
movl	(%rax, %rcx, 4), %eax
ret
```

动态的版本必须用乘法指令对`i`伸缩`n`倍，而不能用一系列移位和加法，在一些处理器中乘法会带来严重的性能处罚。



编译器优化：利用访问模式的规律性，优化索引计算



## 结构体







## 浮点数

xmm寄存器：全部是调用者保存