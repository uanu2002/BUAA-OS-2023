## 思考题

### Thinking 2.1

>   请根据上述说明，回答问题：在编写的 C 程序中，指针变量中存储的地址是虚拟地址，还是物理地址？MIPS 汇编程序中 lw 和 sw 使用的是虚拟地址，还是物理地址？ 



### Thinking 2.2

>   请思考下述两个问题：
>
>   • 从可重用性的角度，阐述用宏来实现链表的好处。
>
>   • 查看实验环境中的 /usr/include/sys/queue.h，了解其中单向链表与循环链表的实现，比较它们与本实验中使用的双向链表，分析三者在插入与删除操作上的性能差异。



### Thinking 2.3

>   请阅读 include/queue.h 以及 include/pmap.h, 将 Page_list 的结构梳理清楚，选择正确的展开结构。
>
>   ![image-20230508015721475](C:\Users\DELL\AppData\Roaming\Typora\typora-user-images\image-20230508015721475.png)



### Thinking 2.4

>   请思考下面两个问题：
>
>   • 请阅读上面有关 R3000-TLB 的描述，从虚拟内存的实现角度，阐述 ASID 的必要性。
>
>   • 请阅读《IDT R30xx Family Software Reference Manual》的 Chapter 6，结合 ASID段的位数，说明 R3000 中可容纳不同的地址空间的最大数量。



### Thinking 2.5

>   请回答下述三个问题：
>
>   • tlb_invalidate 和 tlb_out 的调用关系？
>
>   • 请用一句话概括 tlb_invalidate 的作用。
>
>   • 逐行解释 tlb_out 中的汇编代码。



### Thinking 2.6

>   任选下述二者之一回答：
>
>   • 简单了解并叙述 X86 体系结构中的内存管理机制，比较 X86 和 MIPS 在内存管理上的区别。
>
>   • 简单了解并叙述 RISC-V 中的内存管理机制，比较 RISC-V 与 MIPS 在内存管理上的区别。





## 难点分析



## 实验体会

