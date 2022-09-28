# Floating Point

## Background: Fractional binary numbers

e.g.: What is $1011.101_2$ ? $8+2+1+\frac{1}{2}+\frac{1}{8}$

Representation: 

* Bits to right of “binary point” represent fractional powers of 2

* $$
  \sum ^i _{k=-j}b_k \times 2^k
  $$

Representable Numbers: Limitations

1. Can only exactly represent numbers of the form $\frac{x}{2^k}$, other rational numbers have repeating bit representations.
   $$
   \begin{align}
   \frac{1}{3} = &\ 0.010101010101[01]\dots _2 \\
   \frac{1}{5} = &\ 0.001100110011[0011]\dots _2 \\
   \frac{1}{10} = &\ 0.000110011001[0011]\dots _2
   \end{align}
   $$

2. Just one setting of binary point within the $w$ bits (very small values? very large?)



## IEEE floating point standard: Definition

**IEEE Floating Point: IEEE Standard 754**（**科学计数法**）

* Numerical Form:
  $$
  (-1)^s M\times 2^E
  $$

  * **Sign bit**

  * **Signifivand M**: normally a fractional value in range $[1.0, 2.0]$

  * **Exponent E**: weights value by power of two

* Encoding
  * <img src=".\Images\Float Encoding.png" style="zoom:33%;" />
  * s 符号位
  * exp 阶码，e
  * frac 尾码，f



### 四种表示方式

定义：**偏置**：$Bias = 2^{k-1}-1,\ k$ 为阶码的bit数

例：float偏置为 $2^7-1=127$

1. **规格化**

   阶码的位不全为0、不全为1时：

   * 阶码的值：$E=e-Bias$，单精度float为 $-126 \sim 127$

   * 尾码的值：$f :=0.f_1f_2f_3\dots f_n$，定义 **尾数** $M = 1+f$

     （隐含的1开头：通过调整阶码 $E$，使得尾数在范围 $1\leqslant M<2$ 之中（假设没有溢出），那么这种表示方法能轻松获得1个额外精度位——既然第一位总是等于1，那么就不需要显式地表示它）

   $$
   (-1)^s \times (1+f) \times 2^{e-Bias}
   $$

2. **非规格化**

   阶码的位全为0时：用来 （**以尾码为重点**） 表示很接近0的数

   * 阶码的值： $E = 1-Bias$，尾数 $M = f$

   $$
   (-1)^s \times f \times 2^{1-Bias}
   $$

   * 不取 $-Bias$：为了表示形如 $0.f_1f_2\dots$ 的数，尾数开头不为1（没有隐含的1开头），这样取能平滑的从非规格化值转换到规格化值。

3. **特殊值**

   * **NaN (Not a Number)**：阶码全为1， 小数域不全为0
   * $\infty$ **无穷**：阶码全为1，小数域全为0.

表格图片：<img src=".\Images\Float Representation.png" style="zoom:50%;" />

特殊补充：$0^-<0^+$，所以比较时需要 $\epsilon$

浮点数：代表一段空间



## Rounding, addition, multiplication

### Rounding：**四舍六入，五向偶数舍入**

* 假设：向小数点后两位舍入：

  | Binary |   Rounded   |
  | --- | --- |
  | 10.00 011 | 10.00 |
  | 10.00 110 | 10.01 |
  | 10.11 100 | 11.00 |
  | 10.10 100 | 10.10 |
  
  1. 下一位为0，舍去
  2. 下一位为1，分类：
     * 若下一位之后直到结尾，有任何一位为1，则向上舍入
     * 若之后没有1，则**向偶数舍入**
  
* 目的：（以十进制为例）（从统计的角度而言）保证0.5舍入到1和0之间的概率相同

### FP Multiplication

* $(-1)^{s_1}M_1\ 2^{E_1} \times (-1)^{s_2}M_2\ 2^{E_2}$
* Exact Result: $(-1)^sM\ 2^E$
  * Sign s: `s1 ^ s2`
  * M: `M1 * M2`
  * E: `E1 + E2`
* 修正
  * $M \geqslant 2$，M 右移，E++；
  * E 有溢出问题
  * M 的舍入问题：浮点型舍入
* 结合律：No（举一个到0的例子就行）；交换律：Yes

### FP Addition

和乘法大致同理：

* 满足单调性：$\text{如果} x,a,b \not= NaN/\infty, \text{则} a \geqslant b \Leftrightarrow x+a \geqslant x+b$
* 额外修正
  * $M < 1$，M 左移，E--；
  * 精度丢失：float类型可以用于数值计算的位数只有23位（尾码）
* 结合律：No（精度丢失）；交换律：Yes



## Floating Point in C

* float单精度，double双精度
* 转换：
  * float/double $\rightarrow$ int：截尾，NaN转换为TMin
  * int $\rightarrow$ double：不越界则正常，遵循舍入原则



## Summary

* IEEE Floating Point has clear mathematical properties
* Represents numbers of form $M \times 2^E$
* One can reason about operations independent of  implementation
  * As if computed with perfect precision and then rounded
* **Not the same as real arithmetic**
  * Violates associativity/distributivity
  * Makes life difficult for compilers & serious numerical applications  programmers



## Additional Content: Creating Floating Point Number

* Steps: 
  1. Normalize to have leading 1
  2. Round to fit within fraction
  3. Postnormalize to deal with effects of rounding

