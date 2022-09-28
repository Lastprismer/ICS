# Machine-Level Programming I: Basics

## History of Intel processors and architectures

### Intel 处理器 x86 的发展历程

* 起源：1978年8086；
* 现在的“x86"：x86-64的简称，“Intel 32位体系结构（Intel Architexture 32-bit, IA32）” 的64位扩展
* **摩尔定律**：单个处理器中，晶体管数量每26个月就会翻一番（在超过50年中，每18个月就会翻一番）
* 并驾齐驱的公司：AMD



## C, assembly, machine code

### 概念定义

* **ISA**：Instruction Set Architecture，指令系统架构（指令系统体系结构）（抽象）
  * **一个处理器支持的指令和指令的字节级编码**
  * 内容：**基本数据类型**，**指令集**，寄存器，寻址模式，存储体系，中断，异常处理以及外部I/O
    * 定义了机器级程序的格式和行为——处理器状态、指令的格式、每条指令对状态的影响
    * 通过采取某些措施，让实际上在处理器硬件上并发执行的指令，它们的整体的执行行为与按照ISA指定的顺序来执行产生的行为完全一致
    * 可视为软硬件的接口
  * 内容例：机器级代码（处理器执行的字节级程序）、汇编代码（机器码的文本表示）
  * 例：Intel 的 x86, IA32, Itanium, x86-64等

* **Microarchitexture**：ISA的具体硬件实现，使 ISA 可以在处理器上被运行：寄存器大小、核心频率



### 汇编与机器代码

<img src=".\Images\Programmer-Visible State.png" style="zoom:50%;" />

通过机器代码可以看到的处理器状态：

* PC **程序计数器**：给出将要执行的下一条指令在内存中的地址
* Register file **寄存器文件**（寄存器堆）：存储数据
  * Condition Codes **条件码寄存器**：保存着近执行的算术或逻辑指令的状态信息，实现控制或数据流中的条件变化
* Memory **内存**：Byte addressable array, stack to support procedures



#### 将C代码转化为目标码（Object Code，基本就是机器码）

已有一个多文件工程：包含 p1.c 和 p2.c 文件

1. 使用 **编译器**（Compiler，包含预处理器），将 **源程序**（C Program） 翻译为 **汇编程序**（Asm Program）p1.s, p2.s
2. 使用 **汇编器**（Assembler），将 **汇编程序** 翻译为 **目标程序**（Object Program）p1.o, p2.o
3. 使用 **链接器**（Linker），将 多个 **目标程序**与 **库** 合并，得到 **可执行目标文件**（可执行文件）p

* 即机器执行的程序只是一个字节序列（如此处的 p1.o 文件），它是对一系列指令的编码
* 查看机器代码：使用 **反汇编器**（Disassembler）处理机器代码，可以反向得到类似于汇编代码的文本
  * `objdump -d [filename]`，可用于`.out`（完全可执行文件）或`.o`文件
* **汇编器**：将指令进行二进制编码处理，翻译为几乎完整、可以执行的代码，但是不同文件代码之间缺少链接
* **链接器**：处理文件间的引用，链接静态 / 动态库，链接发生在程序开始时



#### 汇编语言的特征

* 数据：1，2，4，8 字节的整型，用以存储数据或地址

  4，8，（10，不要用）的浮点，存储数据

  <img src=".\Images\Assembly Data Types.png"  />

  代码：用字节序列编码的一系列指令
  
  没有聚合数据类型，（数组、结构体等），只是在内存中连续分配对应需要的空间
  
* 操作：

  * 对寄存器或内存数据执行算术功能
  * 在内存和寄存器之间传输数据
  * 实现流控制



#### 汇编、机器码实例

**C代码**：`*dest = t`：将 `dest`指向的对象赋值`t`

汇编：`movq %rax, (%rbx)`：

* 将8字节值移动到内存中（x86-64中的“四字”）
* 操作：
  * `t`： `Register %rax`
  * dest: `Register %rbx`
  * *dest: `Memory M[%rbx]`

机器码：`0x40059e:  48 89 03`

* 三个字节的指令，储存在地址0x40059e处

**可反汇编**：任何可以被解释为可执行代码的东西都可反汇编，反汇编器会检查字节并重建汇编源代码。

* 使用 gdb Debugger 分析反汇编码



## Assembly Basics: Registers, operands, move

### Registers 寄存器

<img src=".\Images\Registers in x86.png" style="zoom:50%;" />

（右边那些小字在“运行时栈”块，之后学到再说）

所有16个寄存器的低位部分都可以作为字节字（16位）、双字（32位）和四字（64位）数字来访问，不同的操作会访问不同的字节。

生成小于8字节结果的指令：

* 生成1、2字节数字的指令：保持剩下的字节不变
* 生成4字节数字的指令：高位4个字节置为0



### Operand 操作数类型

1. **立即数**，表示常数值，`$mm`，以1，2，4字节编码，如`$0x400, $-533`

2. **寄存器**，表示16个寄存器的内容

   * 用不同的字节作为操作数，可以对应寄存器中不同的位
   * 1，2，4，8字节的操作数对应8，16，32，64位的低位
   * 用符号 $r_a$ 表示寄存器 $a$，用 $R[r_a]$ 表示它的值（寄存器引用）
   * 如 `%rax, %r10d, %bx, %dil`
     * `%rsp` 有特殊用途，其他的寄存器在特定指令中由特殊用途

3. **内存**引用

   * 将内存看成一个很大的字节数组，用 $M_b[Addr]$ 表示对存储在内存中从地址 $Addr$ 开始的 $b$ 个字节值的引用

   * 地址数据以8个字节（x86-64）存在某个寄存器中

   * **完整的寻址模式**：

     **通用形式**：语法：$Imm(r_b, r_i,s)$ 对应计算出的有效地址：
     $$
     Imm+R[r_b]+R[r_i]\cdot s
     $$
     $Imm$：**立即数偏移**；$r_b$：**基址寄存器**；$r_i$：**变址寄存器**；$s$：比例因子（必须为1，2，4或8）
     
     此处寄存器中64位的内容被解释为一个内存地址
     
     
     
   * 例子：
   
     * `(rb, ri)` $M_b[R[r_b]+R[r_i]]$
     * `Imm(, ri, s)` $M_b[Imm+R[r_i]\cdot s]$
     * `(rb, ri, s)` $M_b[R[r_b]+R[r_i]\cdot s]$



### Moving Data 最频繁使用的指令——数据传送指令

作用：将数据从一个位置复制到另一个位置

**指令类**：同一类的指令执行相同的操作，但是操作的数据大小不同

**MOV类**：

<table style="text-align: center;">
    <tr>
        <th>指令</th>
        <th>效果</th>
        <th>描述</th>
    </tr>
    <tr>
        <td>MOV S, D</td>
        <td>D &#8592 S</td>
        <td>传送</td>
    </tr>
    <tr>
        <td>movb<br>movw<br>movl<br>movq<br></td>
        <td></td>
        <td>传送字节（1 Byte）<br>传送字（2 Byte）<br>传送双字（4 Byte）<br>传送四字（8 Byte）</td>
    </tr>
    <tr>
        <td>movabsq I, R</td>
        <td>R &#8592 I</td>
        <td>传送绝对的四字</td>
    </tr>
</table>

* `movabsq`仅用于将立即数传送给寄存器

实际例子：

C代码：

```c++
void swap(long *xp, long *yp)
{
    long t0 = *xp;
    long t1 = *yp;
    *xp = t1;
    *yp = t0;
}
```

汇编码：

```assembly
swap:
	movq	(%rdi), %rax	# to = *xp，%rdi为第一个参数，存入%rax返回值
	movq	(%rsi), %rdx	# t1 = *yp，%rsi为第二个参数
	movq	%rdx, (%rdi)	# *xp = t1
	movq	%rax, (%rsi)	# *yp = t2

```

* 传送指令的两个操作数**不能都指向内存位置**，需要先把源值加载到寄存器中，再将寄存器值写入目的位置（x86）
* 寄存器部分的大小必须与指令最后一个字符（“b-byte”, “w-word”, “l-long word”，“q-quad word”）匹配
* `movb`，`movw`以寄存器为目的时，只会改变指定的寄存器字节；`movl`以寄存器为目的、设置寄存器的低位4字节时，**会同时将寄存器的高位4字节置为0**（x86）
* **为什么要有`movabsq`？**常规的`movq`以立即数作为源操作数时，最大只能使用表示为 **32位补码数字** 的立即数。移动时，会将这个值 **符号扩展** 为64位的值，放到目的位置。（源是立即数时才会发生，寄存器和内存地址时不会发生，为了向下兼容32位）`movabsq`能够以 **任意64位立即数数值** 作为源操作数，并且 **只能以寄存器为目的**。



#### 附录：更多的传送指令

1. 两类类似于MOV的指令，都是 **将较小的源值复制到较大的目的中去** ，都只能以 **寄存器** 作为目的，以 **寄存器或内存地址** 作为源；
   
   1.1 **MOVZ类**，“**零扩展**数据传送指令”，把目的中剩余的字节填充为0
   
   * `movz#&`为将#零扩展到&后传送到目的（如`movzbw`为“将做了零扩展的字节传送到字”）——`movzbw, movzbl, movzbq, movzwl, movzwq`
   * 没有“`movzlq`”——寄存器为目的的`movl`会自动将高位4字节置零，避免重复实现
   
   1.2 **MOVS类**，“**符号扩展**数据传送指令”，需要填充的字节，通过符号扩展规则进行填充
   * `movs#&`为将#符号扩展到&后传送到目的（如`movsbw`为“将做了符号扩展的字节传送到字”）——`movsbw, movsbl, movsbq, movswl, movswq, movslq`
   * 还有一个 `cltq`，没有操作数：它总是以寄存器`%eax`（双字）作为源，`%rax`（四字）作为目的，默认效果等价于`movsql %eax, %rax`

2. 基于 **程序栈** 的数据传送
   * 程序栈：先进后出，用push操作压入栈，用pop操作推出栈
   
   * 程序栈存放在内存中某个区域，栈向下增长，故栈顶元素地址是所有栈中元素地址中最低的
     * **栈指针** `%rsp`保存着栈顶元素的地址
     
   * `pushq S`等价于两条指令：
   
     `subq $8, %rsp`
   
     `movq S, (%rsp)`
   
     而`pop D`等价于两条指令：
   
     `movq (%rsp) D`
   
     `addq $8 %rsp`
<table>
<table style="text-align:center">
    <tr>
        <th>指令</th>
    	<th>效果</th>
    	<th>描述</th>
    </tr>
    <tr>
        <td>pushq S</td>
        <td>R[%rsp] &#8592 R[%rsp]-8<br>M[R[%rsp]] &#8592 S </td>
        <td>将四字压入栈</td>
    </tr>
    <tr>
        <td>popq D</td>
        <td>D &#8592 M[R[%rsp]]<br>R[%rsp] &#8592 R[%rsp]+8</td>
        <td>将四字弹出栈</td>
    </tr>
</table>




## Arithmetic & logical operations

### 加载有效地址 leaq

* `movq`的变形，将内存有效地址写入**寄存器**，为后面的内存引用产生指针
* 也可以简洁的描述普通的算术操作
  * 例如：`%rdx`值为x，`%rsi`值为k，则 `leaq imm(%rdx,%rsi,9), %rax`会将寄存器`%rax`的值设置为`imm+x+9*k`
  * 可用于简化乘法和加法指令
  * 也有`leal`，用于32位地址计算和传送
* 没有溢出（在流控制中不产生溢出标志）



### 更多算数和逻辑指令

![](.\Images\Arithmetic Expressions.png)

* 有b, w, l, q四种，操作不同的大小



### 特殊的算术操作

x86-64对128位（16字节）数的操作提供有限的支持，为 **八字**（oct word）

<table>
    <tr>
        <th>指令</th>
        <th>效果</th>
        <th>描述</th>
    </tr>
    <tr>
        <td>imulq S<br>mulq S</td>
        <td>R[%rdx]:R[%rdx] &#8592 S * R[%rax] <br> R[%rdx]:R[%rax] &#8592 S * R[%rax]</td>
        <td>有符号全乘法 <br> 无符号全乘法</td>
    </tr>
    <tr>
        <td>clto</td>
        <td>R[%rdx]:R[%rax] &#8592 符号扩展（R[%rax]）</td>
        <td>转换为八字</td>
    </tr>
    <tr>
        <td>idivq S</td>
        <td>R[%rdx] &#8592 R[%rdx]:R[%rax] mod S <br> R[%rdx] &#8592 R[%rdx]:R[%rax] / S</td>
        <td>有符号除法</td>
    </tr>
    <tr>
        <td>idivq S</td>
        <td>R[%rdx] &#8592 R[%rdx]:R[%rax] mod S <br> R[%rdx] &#8592 R[%rdx]:R[%rax] / S</td>
        <td>无符号除法</td>
    </tr>
</table>

* imulq有两种：双操作数时，为从两个64位操作数中产生一个64位乘积；单操作数时（以及mulq，无符号乘法），一个参数必须在寄存器 %rax 中，而另一个作为指令的源操作数给出。乘积存放在寄存器%rdx（高64位）和%rax（低64位）中。
* 其他同理
* 去实际试一下下面两个：

```c
#include <inttypes.h>
typedef unsigned ___int128 uint128_t;
void store_uprod (uint128_t *dest, uint64_t x, uint64_t y)
{
    *dest = x * (uint128_t) y;
}
```

（注意大小端带来的区别）

<img src=".\Images\Int128 Asm.png" style="zoom:50%;" />



算术运算表中没有除法或取模操作，这些操作是由单操作数除法指令来提供的。

* 有符号除法指令过idivl将寄存器%rdx（高64 位）、%rax（低64位）中的128位数作为被除数，而除数作为指令的操作数给出
* 商存储在%rax中，余数存储在%rdx中
* %rdx的位应该设置为全0（无符号运算）或者%rax 的符号位（有符号运算）

```c
void remdiv (long x, long y, long *qp, long *rp)
{
    long q = x / y;
    long r = x % y;
    *qp = q;
    *rp = r;
}
```

<img src=".\Images\64div Asm.png" style="zoom:50%;" />
