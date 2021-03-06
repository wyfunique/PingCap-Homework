# PingCap-Homework
Author: Yifan Wang

### 1. 文件结构

`include`: 头文件

`src`: C++源代码

`util`:  (1) python脚本，用于生成随机URL文件供测试使用 (2) 生成的url文件 (3) 预处理之后的url文件



### 2. 大致思路

首先，我想到了外部排序，比如经典的TPMMS。但是这个问题其实并不需要排序，排序的代价太大。

然后，考虑数据库、操作系统中常用的page置换算法，想到了LRU。

然后，为了提高效率，我选择为置换出去的每个URL都创建一个文件来存放它的中间过程计数。相当于一个hash表，可以根据文件名直接找到相应URL的计数，从而在每次该URL被置换出去时进行累加，最后这些文件里存放的就是每个URL的最终计数结果。而且这样每个文件都很小，操作相对较快。最后只需要扫描一遍这些小文件就可以得到top K 的URL。

因此，我的大致思路是 **LRU + 类Hash表文件系统**。



### 3. 数据分布的影响 

（1）在测试过程中，我发现数据分布对性能影响很大。由于我的测试数据接近于均匀分布，换句话说，不够集中，存在较多偶然数据。这就导致LRU效率受限。

（2）然后我引入了MRU算法来代替LRU，来测试数据是否过于分散，是否出现周期性重复的情况。如果存在这些情况，MRU性能应该会明显好于LRU。不过实验结果表明二者相差不大。

（3）于是可以推断，均匀分布的数据是介于LRU（集中分布）和MRU（周期分布）之间的。因此我引入预处理，先将url文件分批载入内存，进行初步聚合，将每一批内部的相同URL合并并计数，然后写入临时文件。经测试，这一步骤可以将原始URL文件大小压缩至2/3。之后再用LRU就会高效很多。



### 4. 最终方案

最终，我使用 **预聚合处理 + LRU + 类Hash表文件系统** 的方案解决这一问题。



### 5. 代码简介

由于缺乏开发环境和设备等原因，我没有进行完全的系统编程，而是选择模拟主要过程。

（1）设计了一个Memory类来模拟内存，包括设置内存大小、利用Windows系统API检查本程序占用内存、将数据从硬盘载入内存（读文件）、从内存写回硬盘（写文件）、当本程序实际占用内存接近设定总内存时进行LRU置换等等。

（2）设计Logger类来进行日志记录和管理

（3）编写测试用例

### +++++++++++++++++++++++++++++++++++++++++++++++++++++

### 6. 更新与改进

**（1）异步IO + 多线程**

为了使读写磁盘不要阻塞等待（异步IO），我加入了write buffer，使得写磁盘不再直接阻塞式读出内存核心运算区域，而是从buffer读出。这样也可以利用多线程，同时写入多个文件，可以提高存储效率。

**注：** 只有写文件用到了多线程，读文件仍然用单线程。因为输入文件是单个大文件，最好的读取方式就是顺序读。这时用多线程的话只会导致磁盘频繁随机读写，反而降低IO性能。

**（2）避免大量小文件读写**
**主要思路：** 利用Hash函数将每个URL映射为一个32位整数，由于Hash函数的结果大约呈现均匀分布，我们可以将这些值所在的空间均分成若干hash bucket，也就是若干临时文件。这样每个临时文件将对应多个URL，而不再是原来的一个文件只存一个URL，大大减少中间临时文件的数目。比如我们想让临时文件个数为200（也就是大小为每个最多500MB），那么就可以将这些整数的集合分为200份，第一份为[0, 2^32/200)，第二份为[2^32/200, 2 * 2^32/200)，…… 最后一份为[199 * 2^32/200, 2^32)。每一份对应一个临时文件，然后将相应整数对应的原始URL的中间计数结果存放在该文件中。

**注：** 假定每个URL平均长50个字符，那么100G大约是100 * 1000 * 1000 * 1000 / 50 = 10^11 / (0.5 * 10^2) = 2 * 10^9, 约为32bit整数个数（2^32）的1/5，因此可以忽略碰撞。当然如果使用64位Hash会更好的避免碰撞。由于我选用的C++ std::hash 是32位Hash函数，因此这里并没有使用64位Hash。在以后的改进中可以考虑换成64位。

**（3）** 相应的，在write buffer端的线程会持续读出buffer里属于同一整数子集（也就是要被写入同一临时文件）的连续的URL，直到遇见第一个属于另外子集的URL为止。然后它就释放write buffer的锁、开始写入相应文件，这样可以保证一次读写写入最多的URL，尽可能减少IO次数。
**注：** 

	1）这样的话每个写线程都要有一个自己的buffer，从总的write buffer先读入自己的buffer，当遇到另外子集的URL时再开始从自己的buffer里写入disk，这样才能保证一个线程写入disk不影响另一个线程从总的write buffer读。
	
	2）每个写线程还需要有一个临时文件buffer，因为修改临时文件必须要全部读入内存。所以临时文件不可太大，否则这里会耗用太多内存（这一部分一共需要 写线程数*临时文件buffer size 这么多的内存）
	
	3）总write buffer + 所有写线程自己的write buffer和临时文件buffer + 核心运算 <= 内存大小
	
	4）在具体实现中，我们设定临时文件大小相对内存来说很小（设定其大小为内存的1%）。而写线程不会很多。因此我们认为临时文件buffer占用内存不大，可以忽略。

**（4）数据分布问题：**

我着重考虑了最坏情况下的数据分布，也就是每个URL都出现很少的次数。这种情况下目前的方案应该是比较有效的。
