# PingCap-Homework

### 1. 文件结构

`include`: 头文件

`src`: C++源代码

`util`:  (1) python脚本，用于生成随机URL文件供测试使用 (2) 生成的url文件 (3) 预处理之后的url文件



### 2. 大致思路

首先，我想到了外部排序，比如经典的TPMMS。但是这个问题其实并不需要排序，排序的代价太大。

然后，考虑数据库、操作系统中常用的page置换算法，想到了LRU。

然后，为了提高效率，我选择为置换出去的每个URL都创建一个文件来存放它的中间过程计数。相当于一个hash表，可以根据文件名直接找到相应URL的计数。而且这样每个文件都很小，操作相对较快。最后只需要扫描一遍这些小文件就可以得到top K 的URL。

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

