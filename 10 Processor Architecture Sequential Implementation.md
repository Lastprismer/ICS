# 10 Processor Architecture SEQ: Sequential Implementation



Y86-64 的顺序实现



## 一、将处理组织成阶段

* **取指**（fetch）：从内存中读取指令字节，地址为程序计数器（PC）的值。读取：

  * icode指令代码和ifun指令功能
  * rA、rB寄存器操作数指示符
  * 四字节常数字valC
  * 按照顺序方式计算当前指令的下一条指令的地址valP：valP等于PC的值加上已取出指令的长度
* **译码**（decode）：从寄存器文件读入最多两个操作数，得到值valA和（或）valB
* **执行**（execute）：
  1. 执行指令（ifun）指明的操作、计算内存引用的有效地址、增加或减少栈指针等，得到valE
  2. 设置条件码、检验条件码：条件转移或跳转
* **访存**（）：将数据写入内存或从内存读出数据，值为valM
* **写回**（write back）：最多写两个结果到寄存器文件
  * 这里有两个写口：`M`和`E`，一个是写`valM`的，一个是写`valE`的
  * 说明其他的`val`是**不能直接写**的
* **更新PC**（PC update）：将PC设置成下一条指令的地址

处理器无限循环，执行这些阶段；发生任何异常时，处理器就会停止：执行`halt`指令或非法指令、试图读或写非法地址



<table>
    <tr style = "text-align:center">
        <th rowspan=2>指令</th>
        <th>取指</th>
        <th>译码</th>
        <th>执行</th>
    </tr>
    <tr style = "text-align:center">
        <th>访存</th>
        <th>写回</th>
        <th>更新PC</th>
    </tr>
    <tr>
        <td rowspan=2 style = "text-align:center; vertical-align:middle;">OPq rA, rB</td>
        <td>
            icode:ifun &#8592 M1[PC] <br>
            rA:rB &#8592 M1[PC+1] <br>
            <br>
            valP &#8592 PC+2
        </td>
        <td>
            valA &#8592 R[rA] <br>
            valB &#8592 R[rB]
        </td>
        <td>
            valE &#8592 valB OP valA <br>
            Set CC
        </td>
    </tr>
    <tr>
        <td></td>
        <td>
            R[rB] &#8592 valE
        </td>
        <td>PC &#8592 valP</td>
    </tr>
    <tr>
        <td rowspan=2 style = "text-align:center; vertical-align:middle;">rrmovq rA, rB</td>
        <td>
            icode:ifun &#8592 M1[PC] <br>
            rA:rB &#8592 M1[PC+1] <br>
            <br>
            valP &#8592 PC+2
        </td>
        <td>
            valA &#8592 R[rA]
        </td>
        <td>
            valE &#8592 0+R[rA]
        </td>
    </tr>
    <tr>
        <td></td>
        <td>
            R[rB] &#8592 valE
        </td>
        <td>PC &#8592 valP</td>
    </tr>
    <tr>
        <td rowspan=2 style = "text-align:center; vertical-align:middle;">irmovq V, rB</td>
        <td>
            icode:ifun &#8592 M1[PC] <br>
            rA:rB &#8592 M1[PC+1] <br>
            valC &#8592 M8[PC+2] <br>
            valP &#8592 PC+10
        </td>
        <td></td>
        <td>
            valE &#8592 0+valC
        </td>
    </tr>
    <tr>
        <td></td>
        <td>
            R[rB] &#8592 valE
        </td>
        <td>PC &#8592 valP</td>
    </tr>
    <tr>
        <td rowspan=2 style = "text-align:center; vertical-align:middle;">rmmovq rA, D(rB)</td>
        <td>
            icode:ifun &#8592 M1[PC] <br>
            rA:rB &#8592 M1[PC+1] <br>
            valC &#8592 M8[PC+2] <br>
            valP &#8592 PC+10
        </td>
        <td>
            valA &#8592 R[rA] <br>
            valB &#8592 R[rB]
        </td>
        <td>
            valE &#8592 valC + valB
        </td>
    </tr>
    <tr>
        <td>
            M8[valE] &#8592 valA
        </td>
        <td></td>
        <td>PC &#8592 valP</td>
    </tr>
    <tr>
        <td rowspan=2 style = "text-align:center; vertical-align:middle;">mrmovq D(rB), rA</td>
        <td>
            icode:ifun &#8592 M1[PC] <br>
            rA:rB &#8592 M1[PC+1] <br>
            valC &#8592 M8[PC+2] <br>
            valP &#8592 PC+10
        </td>
        <td>
            valB &#8592 R[rB]
        </td>
        <td>
            valE &#8592 valC + valB
        </td>
    </tr>
    <tr>
        <td>
            valM &#8592 M8[valE]
        </td>
        <td>
            R[rA] &#8592 valM
        </td>
        <td>PC &#8592 valP</td>
    </tr>
    <td rowspan=2 style = "text-align:center; vertical-align:middle;">pushq rA</td>
        <td>
            icode:ifun &#8592 M1[PC] <br>
            rA:rB &#8592 M1[PC+1] <br>
            <br>
            valP &#8592 PC+2
        </td>
        <td>
            valA &#8592 R[rA] <br>
            valB &#8592 R[%rsp]
        </td>
        <td>
            valE &#8592 valB + (-8)
        </td>
    </tr>
    <tr>
        <td>
            M8[valE] &#8592 valA
        </td>
        <td>
            R[%rsp] &#8592 valE
        </td>
        <td>PC &#8592 valP</td>
    </tr>
	<tr>
		<td rowspan=2 style = "text-align:center; vertical-align:middle;">popq rA</td>
        <td>
            icode:ifun &#8592 M1[PC] <br>
            rA:rB &#8592 M1[PC+1] <br>
            <br>
            valP &#8592 PC+2
        </td>
        <td>
            valA &#8592 R[%rsp] <br>
            valB &#8592 R[%rsp]
        </td>
        <td>
            valE &#8592 valB + 8
        </td>
    </tr>
    <tr>
        <td>
            valM &#8592 M8[valA]
        </td>
        <td>
            R[%rsp] &#8592 valE <br>
            R[rA] &#8592 valM
        </td>
        <td>PC &#8592 valP</td>
    </tr>
	<tr>
		<td rowspan=2 style = "text-align:center; vertical-align:middle;">jXX Dest</td>
        <td>
            icode:ifun &#8592 M1[PC] <br>
            <br>
            valC &#8592 M8[PC+1]<br>
            valP &#8592 PC+9
        </td>
        <td></td>
        <td>
            Cnd &#8592 Cond(ifun, CC)
        </td>
    </tr>
    <tr>
        <td></td>
        <td></td>
        <td>PC &#8592 Cnd ? valC : valP</td>
    </tr>
	<tr>
		<td rowspan=2 style = "text-align:center; vertical-align:middle;">call Dest</td>
        <td>
            icode:ifun &#8592 M1[PC] <br>
            <br>
            valC &#8592 M8[PC+1]<br>
            valP &#8592 PC+9
        </td>
        <td>
            valB &#8592 R[%rsp]
        </td>
        <td>
            valE &#8592 valB + (-8)
        </td>
    </tr>
    <tr>
        <td>
            M8[valE] &#8592 valP
        </td>
        <td>
            R[%rsp] &#8592 valE
        </td>
        <td>PC &#8592 valC</td>
    </tr>
	<tr>
		<td rowspan=2 style = "text-align:center; vertical-align:middle;">ret</td>
        <td>
            icode:ifun &#8592 M1[PC] <br>
        </td>
        <td>
            valA &#8592 R[%rsp] <br>
            valB &#8592 R[%rsp]
        </td>
        <td>
            valE &#8592 valB + 8
        </td>
    </tr>
    <tr>
        <td>
            valM &#8592 M8[valA]
        </td>
        <td>
            R[%rsp] &#8592 valE
        </td>
        <td>PC &#8592 valM</td>
    </tr>
	<tr>
		<td rowspan=2 style = "text-align:center; vertical-align:middle;">cmovXX rA, rB</td>
        <td>
            icode:ifun &#8592 M1[PC] <br>
            rA:rB &#8592 M1[PC+1] <br>
            <br>
            valP &#8592 PC+2
        </td>
        <td>
            valA &#8592 R[rA] <br>
            valB &#8592 R[rB]
        </td>
        <td>
            Cnd &#8592 Cond(CC, ifun) <br>
            valE &#8592 valA + 0
        </td>
    </tr>
    <tr>
        <td></td>
        <td>
            if (Cnd) <br>
            R[rB] &#8592 valE
        </td>
        <td>PC &#8592 valP</td>
    </tr>
</table>







## 二、SEQ硬件结构：

SEQ：sequential，顺序的

### 1. 抽象表示

<img src="./Images/10-SEQ View.png" style="zoom: 67%;" />

* PC放在寄存器中，信息沿着线流动（先向上，再向右），同各个阶段相关的 **硬件单元** 负责执行这些处理。
* 右边反馈线路向下，包括写到寄存器文件的更新值和更新的PC值
* 在SEQ内，所有硬件单元的处理都在一个时钟周期内完成。

硬件单元与各个处理阶段相关联：

* **取指**：将程序计数器寄存器作为地址，指令内存读取指令的字节；PC增加器计算valP

* **译码**：寄存器文件有两个读端口A和B，从两个端口同时读valA和valB

* **执行**：根据指令的类型，将ALU用于不同目的（包括加0）

  * ALU也负责计算条件码的新值。

    执行条件传送指令时，根据条件码和传送条件决定是否更新目标寄存器

    执行跳转指令时，根据条件码和跳转类型计算分支信号`Cnd`

* **访存**：数据内存读出或写入一个内存字

* **写回**：端口E写ALU计算出的值（valE），端口M写数据内存中读出的值（valM）

* **PC更新**：新值选择自valP、valC（跳转）、valM（ret）





### 2. 硬件结构

<img src="./Images/10-SEQ Hardware.png" style="zoom:67%;" />

* *时钟寄存器*：PC是SEQ中唯一的时钟寄存器
* *硬件单元*：蓝色框，包括内存、ALU等
* *逻辑控制块*：圆角矩形，后文重点
* 粗线为字长，细线为字节或更小，虚线为单个位
  * 白圈不是单元，是线名







## 三、SEQ 的时序

SEQ实现中，组合逻辑的输入变化了，值就通过逻辑门网络传播；**将读RAM看成和组合逻辑一样的操作**

状态单元：

* PC：每个时钟周期，装载新的指令地址
* CC：只有执行整数运算指令时才装载
* 数据内存：只有执行`rmmovq, pushq, call`时才写数据内存
* 寄存器文件：两个写端口允许每个时钟周期更新至多两个程序寄存器

计算原则：**从不回读**：处理器从来不需要为了完成一条指令的执行而去读由该指令更新了的状态

* 为什么`pushq %rsp`不是将更改后的`%rsp`推入栈中：那需要写回后再进行一次访存，孬
* 没有指令既设置条件码又读取条件码



**读操作沿着状态单元传播，写操作由时间控制**

时钟周期开始时：读入指令，准备更新组合逻辑（组合逻辑、状态单元未更新）

时钟周期结束时，组合逻辑更新完毕（状态单元未更新）

下一个时钟周期开始时，时钟上升，更新状态单元，读入下一个指令（组合逻辑未更新，循环）







## 四、SEQ阶段的实现

<img src="./Images/10-HCL Consts.png" style="zoom:50%;" />



### 1. 取指阶段

<img src="./Images/10-Fetch Logic.png" style="zoom:67%;" />

* *PC*：程序计数器

* *指令内存*：读10个字节，如果指令地址不合法，产生`imem_error`信号

* *Split*：将第一个字节分为两个四位的数

* *icode、ifun逻辑控制块*：输出`icode`、`ifun`：如果接收到了`imem_error`信号，输出`nop`；

  ```
  int icode = 
  {
  	imem_error: INOP;
  	1: imem_icode;
  }
  
  int ifun = 
  {
  	imem_error: FNONE;
  	1: imem_ifun;
  }
  ```

  根据`icode`，计算出三个信号：

  1. `instr_valid`：这个字节是不是合法指令
  2. `need_regids`：指令是否包含一个寄存器指示符字节
  3. `need_valC`：指令是否包含常数字

  * `instr_valid`和`imem_error`在访存阶段用来产生状态码

  例：

  ```
  bool need_valC = icode in {	IIRMOVQ, IRMMOVQ, IMRMOVQ, IJXX, ICALL };
  ```

* *Align*：根据信号，处理剩下的9个字节，产生rA, rB, valC`：如果指令没有指明寄存器，则将`rA, rB`设为0xF（RNONE）；

* *PC增加器*：根据信号，产生`valP`





### 2. 译码和写回阶段

<img src="./Images/10-Decode And Write Back Logic.png" style="zoom:67%;" />

* *寄存器文件*：读口AB，写口EM，地址为程序寄存器的ID或0xF
* *控制逻辑*：读口地址`srcA, srcB`，写口地址`dstE, dstM`
* *信号*：根据执行阶段算出的`Cnd`，确定是否进行条件传送

```
int srcA = 
[
	icode in { IRRMOVQ, IRMMOVQ, IOPQ, IPUSHQ } : rA;
	icode in { IPOPQ, IRET } : RRSP;
	1 : RNONE
]

int srcB = 
[
	icode in { IOPQ, IRMMOVQ, IMRMOVQ } : rB
	icode in { IPUSHQ, IPOPQ, ICALL, IRET } : RRSP
	1 : RNONE
]

int dstE = 
[
	icode in { IRRMOVQ } && Cnd : rB	# 条件传送
	icode in { IIRMOVQ, IOPQ } : rB;
	icode in { IPOPQ, IPUSHQ, ICALL, IRET } : RRSP;
	1 : RNONE
]

int dstM = 
[
	icode in { IMRMOVQ, IPOPQ } : rA
	1 : RNONE;
]
```

同一周期内，两个写端口尝试对同一个地址进行写入时，**M端口优先级更高**：仅M端口上的写会进行。





### 3. 执行阶段

<img src="./Images/10-Execute Logic.png" style="zoom: 80%;" />

* *ALU*：根据`alufun`信号的设置，对输入`aluA, aluB`执行四种运算之一，输出`valE`；
  * 只在执行`OPq`指令时才设置条件码
* *Cond*：根据条件码和功能码，确定是否进行条件分支或者条件传送，产生的`Cnd`信号用于设置`dstE`，也用于下一个PC逻辑中

```
int aluA = 
[
	icode in { IRRMOVQ, IOPQ } : valA;
	icode in { IIRMOVQ, IRMMOVQ, IMRMOVQ} : valC;
	icode in { ICALL, IPUSHQ } : -8;
	icode in { IRET, IPOPQ } : 8;
]
# 其他情况不需要ALU

int aluB = 
[
	icode in { IRMMOVQ, IMRMOVQ, IOPQ, ICALL, IPUSHQ, IRET, IPOPQ } : valB;
	icode in { IRRMOVQ, IIRMOVQ } : 0
]

int alufun = 
[
	icode == IOPQ : ifun:
	1 : ALUADD;
]

bool set_cc = icode in { IOPQ };
```





### 4. 访存阶段

<img src="./Images/10-Memory Logic.png" style="zoom:67%;" />

* `Mem.read, Mem.write`：数据应该读还是写，读时产生`valM`
* `Mem.addr, Mem.data`：选中的地址和数据
  * 读和写的地址总是`valE`或`valA`
* `Stat`：访存阶段最后才设置，根据取指阶段的`icode, imem_error, instr_valid`和数据内存产生的`dmem_error`，计算状态码

```
bool mem_read = icode in { IMRMOVQ, IPOPQ, IRET };
bool mem_write = icode in {	IRMMOVQ, IPUSHQ, ICALL };

int mem_addr = 
[
	icode in { IRMMOVQ, IPOSHQ, ICALL, IMRMOVQ } : valE;
	icode in { IPOPQ, IRET } : valA;
]

int mem_data = 
[
	icode in { IRMMOVQ, IPUSHQ } : valA;
	icode == ICALL : valP;
]

int Stat = 
[
	imem_error || dmem_error : SADR; # 地址异常
	icode == IHALT : SHLT; # halt状态
	!instr_valid : SINS; # 非法指令异常状态
	1 : SAOK; # 正常操作
]
```





### 5. 更新PC阶段

<img src="./Images/10-Update PC Logic.png" style="zoom:67%;" />

```
int new_pc = 
[
	icode == ICALL : valC;
	icode == IJXX && Cnd : valC;
	icode == IRET : valM;
	1 : valP;
]
```







## 五、总结

缺点：太慢了，时钟必须非常慢，以使信号能在一个周期内传播所有的阶段。