# Machine-Level Programming II: Control

机器代码实现有条件的行为：

1. 测试数据值
2. 根据测试结果改变控制流或数据流

机器代码中的指令都是顺序进行的



## Control: Condition codes 条件码

**条件码寄存器**：描述最近的算数或逻辑操作的属性，可以检测条件码寄存器来执行条件分支指令



### 一、隐含设置

* **CF**：进位标志：最近的操作使最高位产生了进位，可用来检测无符号操作的溢出

* **ZF**：零标志：最近的操作得到了0

* **SF**：符号标志：最近操作得到的结果为负数

* **OF**：溢出标志：最近操作导致一个补码溢出（正/负）
  * 都**只有一个位**，表述的是**最近**的算数或逻辑运算的结果
  
  * `leaq`不会改变任何条件码
  * 逻辑操作中，CF和OF会被设置为0
  * 移位操作中，CF会被设置为最后一个被移出的位，OF设置为0
  * INC（++）和DEC（--）指令会设置OF和ZF，但是不会改变CF

例：t, a, b为整型，用ADD指令完成`t = a + b`：

* CF：`(unsigned)t < (unsigned)a`
* ZF：`t == 0`
* SF：`t < 0`
* OF：`(a < 0 && b < 0) && (a < 0 != t < 0)`



### 二、显式设置

1. CMP指令
   * 行为和SUB指令一样，但是只改变条件码寄存器
     * 如：`cmpb, cmpw, cmpl, cmpq`；`cmpq S1, S2`会去计算`S2-S1`并改变条件码寄存器（比较S2？S1）
2. TEST指令
   * 行为和AND指令一样，但是只改变条件码寄存器
     * 如：`testb, testw, testl, testq`；`testq S1, S2`会去计算`S2&S1`并改变条件码寄存器

3. 其他的（书上和ppt都没有的）
   * `CLC`：CF置0
   * `SLC`：CF置1
   * `CMC`：CF取反



### 三、访问条件码

条件码通常不会直接读取，常用的使用方法有：

1. 根据条件码的某种组合，将一个字节设置为0或者1
2. 跳转到程序的某个其他的部分
3. 有条件的传送数据

#### SET指令

![](.\Images\Set istc.png)

例子：

```c
int gt (long x, long y) { return x > y; }
```

```assembly
cmpq	%rsi, %rdi
setq	%al
movzbl	%al, %eax
ret
```

例：`setg	D`：不为0，为负但是溢出（大整数） / 不溢出且为正



## Conditional branches 流控制：分支

### 一、跳转指令

* **直接跳转**：跳转目标作为指令的一部分编码
  * 条件跳转只能是直接跳转
* **间接跳转**：跳转目标从寄存器或内存位置中读出，如：
  * `jmp *%rax`，用`%rax`中的值作为跳转目标
  * `jmp *(%rax)`，`%rax`中的值作为读地址，从内存中读出跳转目标

![](.\Images\Jump istc.png)

注：

* 跳转指令的编码：
  1. **PC相对**：会将目标指令的地址与**紧跟在跳转指令后面**那条指令的地址之间的差作为编码，差可以编码为1、2或4字节
  2. **绝对地址**：用4字节直接指定目标



### 二、用条件控制实现条件分支

例子：

```c
if (test-expr)
{
    then-statement
}
else
{
    else-statement
}
```

**两个分支语句中只能执行一个**，此时汇编通常会使用这样解释——

```c
t = test-expr;
if (!t)
    goto false;
then-statement
    goto done;
false:
	else-statement;
done:
```

汇编器为`then-statement`和`else-statement`产生各自的代码块。它会插入条件和无条件分支，以保证能执行正确的代码块。



### 三、用条件传送实现条件分支

用控制实现条件转移简单而通用，但是在现代处理器上，它 **可能** 会非常低效。替代的策略是使用**数据**的条件转移：**先计算出条件操作的两种结果，然后再根据条件是否满足从中选取一个**。

```c
long absdiff (long x, long y)
{
    long result;
    if (x < y)
        result = y - x;
    else
        result = x - y;
    return result;
}
```

使用条件赋值的实现：

```c
long cmovdiff (long x, long y)
{
    long rval = y - x;
    long wval = x - y;
    long ntest = x >= y;
    /* Line below requires single instruction: */
    if (ntest)
        rval = eval;
    return rval;
}
```

性能更高原因：**流水线**（pipelining）中，每一条指令的处理会经过一系列的阶段，每个阶段执行所需操作的一小部分（从内存中取指令、确定指令类型、从内存读数据、执行算术运算、向内存写数据、更新程序计数器）；处理器通过重叠连续指令的步骤来获得高性能，程序会预测要跳转到哪个分支以提前重叠连续指令（**分支预测逻辑**）；预测错误会招致惩罚：多花时间运行本不用运行的指令、丢掉它为这条跳转指令后所有已用指令所做的工作、从正确位置处开始填充流水线

预测错误概率为 $p$，没有预测错误执行时间是 $T_{OK}$，预测错误处罚是 $T_{MP}$。

执行平均时间 $T_{avg}(p)=T_{OK}+pT_{MP}$；模式随机时平均时间 $T_{ran}=T_{avg}(0.5)=T_{OK}+0.5T_{MP}$，最终关系 $T_{MP}=2(T_{ran}-T_{OK})$



#### **CMOV**条件传送指令：

![](.\Images\cmov istc.png)

与条件跳转不同，处理器无需预测测试的结果就可以执行条件传送。处理器只是读源值（可能是从内存中），检查条件码，然后要么更新目的寄存器，要么保持不变。

**不能传送单字节**，单字，双字，四字都可。



#### 条件数据传送的问题

例如：`v = test-expr ? then-expr : else-expr`

条件控制转移：

```c
if (!test-expr)
    goto false;
v = then-expr;
goto done;
false: else-expr;
done:
```

条件传送：

```c
v = then-expr;
ve = else-expr;
t = test-expr;
if (!t) v = ve;
```

此时：

```c
long cread (long *xp)
{
    return (xp ? *xp : 0);
}
```

非法：测试为假时，对`*xp`的间接引用还是发生了

编译器会在一定程度上避免这种错误，但不能完全避免

**条件数据传送不是总可以提高代码的执行效率**



## Loops 流控制：循环

条件测试 + 跳转组合 = 循环

### 一、do-while 循环

```c
do
    body-statement
    while(test-expr);
```

to

```c
loop:
body statement;
t = test-expr;
if (t)
    goto loop;
```



### 二、while 循环

```c
while (test-expr)
    body-statement
```

to

1. ```c
   // “跳转到中间”
   goto test:
   loop:
   body-statement
   test:
   t = test-expr;
   if (t) goto loop;
   // 缺点：无论有没有符合条件，都会跳转
   ```
   
2. ```c
   // 转化成do-while guarded-do
   t = test-expr;
   if (!t)
       goto done;
   do
       body-statement
   // 可能快一点（开局不直接跳转）
   ```



### 三、for 循环

```c
for (init-expr; test-expr; update-expr)
    body-statement
```

to

```c
init-expr;
while (test-expr)
{
    body-statement
    update-expr;
}
```

`continue`：跳转到`update-expr`



## Switch 语句

`switch`语句可以根据一个整数索引值进行**多重分支** (multiway branching) 。`switch`通过使用跳转表 (jump table) 数据结构使得实现加高效 。

**跳转表**是一个数组， 表项 $i$ 是一个代码段的地址，这个代码段实现当开关索引值等于 $i$ 时程序应该采取的动作。

和使用一组很长的`if-else`语句相比，使用跳转表的优点是执行开关语句的时间与开关情况的数量无关。 

开关情况较多、值的跨度范围较小时，使用跳转表

gcc会根据 **开关情况的数量** 和 **开关情况值的稀疏程度** 来翻译开关语句。开关情况数量比较多（例如4个以上），并且值的范围跨度比较小时，就会使用跳转表。

例：

```c
void switch_eg (long x, long n, long *dest)
{
    long val = x;
    switch (n)
    {
        case 100:
            val *= 114514;
            break;
        case 102:
            val += 1919810;
           	// 继续向下
        case 103:
            val += 114;
            break;
        case 104:
        case 106:
            val *= val;
            break;
        default:
            val = 0;
    }
    *dest = val;
}
```

```c
void switch_eg_impl (long x, long n, long *dest)
{
    /* Table of code pointers*/
    static void *jt[7] = 
    {
        &&loc_A, &&loc_def, &&loc_B, &&loc_C, &&loc_D, &&loc_def, &&loc_D
    }; // 指向代码位置的指针
    unsigned long index = n - 100; // 无符号
    /* 此处例：
    编译器：将n减去100后，取值范围移到0-6之间
    补码表示的负数变为无符号大整数，进一步简化分支
    */
    long val;
    if (index > 6)
        goto loc_def;
    goto *jt[index];
    
    loc_A:
    val = x * 114514;
    goto done;
    
    loc_B:
    x = x + 1919810;
    
    loc_C:
    val = x + 114;
    goto done;
    
    loc_D:
    val *= val;
    goto done;
    
    loc_def:
    val = 0;
    
    done:
    *dest = val;
}
```

```assembly
.LFB23:
	.cfi_startproc
	subq	$100, %rsi
	cmpq	$6, %rsi
	ja	.L8			# n>6，进入loc_def（负数变为大整数，也>6）
	leaq	.L4(%rip), %rcx
	movslq	(%rcx,%rsi,4), %rax
	addq	%rcx, %rax
	jmp	*%rax
	.section	.rodata # 只读数据，Read-Only Data，存放若干qword（x86-64的long）
	# jmp	*.L4(,%rsi,8) 为课本上给出的示例跳转
	.align 4	# 将地址对齐到4的倍数
	.align 4
.L4:
	.long	.L3-.L4	# case 100: loc_A
	.long	.L8-.L4	# case 101: loc_def，范围内缺失的情况：默认情况
	.long	.L5-.L4	# case 102: loc_B
	.long	.L6-.L4	# case 103: loc_C
	.long	.L7-.L4	# case 104: loc_D
	.long	.L8-.L4	# case 105: loc_def
	.long	.L7-.L4	# case 106: loc_D
	.text
	.p2align 4,,10
	.p2align 3
.L5:                             # case 102
	addq	$1919810, %rdi      # 没有写break，继续运行下去
.L6:                             # case 103
	addq	$114, %rdi
	movq	%rdi, (%rdx)
	ret
	.p2align 4,,10
	.p2align 3
.L3:                             # case 100
	imulq	$114514, %rdi, %rdi
	movq	%rdi, (%rdx)
	ret
	.p2align 4,,10
	.p2align 3
.L7:  		                    # case 104, 106，某种情况对应多种标号（跳转表中相应数组元素相同）
	imulq	%rdi, %rdi
	movq	%rdi, (%rdx)
	ret
	.p2align 4,,10
	.p2align 3
.L8:                             # default
	xorl	%edi, %edi
	movq	%rdi, (%rdx)
	ret
	.cfi_endproc
```

此例中，程序可以只用一次跳转表引用就分支到5个不同的位置。
