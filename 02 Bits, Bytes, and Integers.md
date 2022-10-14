# Bits, Bytes and Integers

## Representing information as bits

**Base 2 Number Representations**

**Why Computer Use Binary**：Binary is the most practical system to use!

### Encoding Byte Values

Byte = 8 Bits

二进制：$00000000_2$ 到 $11111111_2$

十进制：$0_{10}$ 到 $255_{10}$

十六进制：$00_{16}$ 到 $FF_{16}$

* 十六进制表示法：在C中用0x开头
* 内存：以Byte为单位的数组

<img src=".\Images\02-Data Representations.png" style="zoom:50%;"/>



## Bit-level manipulations

### Boolean Algebra

与，或，非，异或，Operate on **Bit Vectors（位向量）**

### Bit-Level Operations in C

左移，右移（逻辑右移：开头补0；数学右移，开头补最高位），true / false，!



## Integers

### Representation: unsigned and signed

Unsigned：
$$
\sum_{i=0}^{n-1}2^ix_i
$$
Singed: (Two's complement, 补码)
$$
-2^{n-1}x_{n-1}+\sum_{i=0}^{n-2}2^ix_i
$$
Uniqueness: Every bit pattern represents  unique integer value, and each representable integer has  unique bit encoding

范围：$-2^{w-1} \sim 2^{w-1}-1$

$|TMin| = TMax + 1,UMax = 2 \times TMax + 1$

相互转换：隐含的强制转换：有符号数与无符号数计算，会被转换为无符号数

### Conversion, casting

Mappings between unsigned and two’s complement numbers: **keep bit representations and reinterpret**

Can have unexpected effects: **adding or subtracting $2 ^ w$**

* 2’s Comp. $\rightarrow$ Unsigned

  Ordering Inversion

  Negative $\rightarrow$ Big Positive

Implicit casting also occurs via assignments and procedure calls

* **Casting Surprises**: Expression Evaluation

  If there is a mix of unsigned and signed in single expression,  **signed values implicitly cast to unsigned**

  e.g.: (-1 > 0u) return true

### Expanding, truncating

扩展和截取：Converting from smaller to larger integer data type, C automatically performs sign extension

扩展：unsigned: 0s added; signed: sign extension. Both yield expected result

截取： (e.g., unsigned to unsigned short)

* Unsigned/signed: bits are truncated, result reinterpreted
* Unsigned: mod operation
* Signed: similar to mod

### Addition, Negation, Multiplication, Shifting

#### Addition

* Unsigned: $s=(u+v)mod\ 2^w$

* 2's: cast first, having identical bit-level behavior with unsigned

* Overflow: 

  <img src=".\Images\02-UAdd Overflow.png" style="zoom:50%;" /><img src=".\Images\02-TAdd Overflow.png" style="zoom:50%;" />

* Negation: same

#### Multiplication

Would need to keep expanding word size with each product computed

Standard Multiplication Function: Ignores high order $w$ bits

* Unsigned: $(u \times v)mod \ 2^w$
* Signed: Lower bits are the same
* Shift e.g.: $u \times 24 =(u << 5) - (u << 3)$

#### Unsigned Power-of-2 Divide with Shift

* Quotient of Unsigned by Power of 2: $u >> k$ gives $\frac{u}{2^k}$
* Uses logical shift
* Signed Power-of-2 Divide: Special condition for negative number: **round**



### Summary

Data type `size_t` defined as unsigned value with `length = word size`

**Why Should I Use Unsigned?**

* Do Use When Performing **Modular Arithmetic** （模运算）:  Multiprecision arithmetic
* Do Use When **Using Bits to Represent Sets**（位向量）：Logical right shift, no sign extension





## Representations in memory, pointers, strings

### Byte-Oriented Memory Organization

Programs refer to data by address, system provides private address spaces to each “process”.

Any given computer has a “Word Size”. Increasingly, machines have 64-bit word size (Potentially, could have 18 EB of addressable memory).

### Word-Oriented Memory Organization

* Addresses Specify Byte  Locations

* Byte Ordering: **多字节值的哪一端存储在该值的起始地址处**

  * 大端存储：(Big Endian): Sun, PPC Mac, Internet: Least significant byte has highest address

  * 小端存储：(Little Endian): x86, ARM processors running Android, iOS, and  Windows: Least significant byte has lowest address

    <img src=".\Images\02-Byte Ordering.png" style="zoom:50%;" />
Representing Pointers: Different compilers & machines assign different locations to objects

### Representing Strings

* Represented by array of characters
* Each character encoded in ASCII format
* String should be null-terminated: Final character = 0
* Byte ordering not an issue