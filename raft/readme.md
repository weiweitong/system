# Raft分布式一致性协议
Raft是为了管理复制日志的一致性算法，它提供了和Paxos算法相同的功能和性能。
Raft将一致性算法分解成了几个关键模块，例如领导人选举、日志复制和安全性。同时它通过实施一个更强的一致性来减少需要考虑的状态的数量。Raft算法还包括一个新机制来允许集群成员的动态改变，利用重叠的大多数来保证安全性。


# 一致性算法
允许一组机器像一个整体一样工作，即使其中一些机器出现故障也能够继续工作下去。Raft被分成领导人选举，日志复制和安全三个模块，并且减少状态机的状态。

# Raft特点
强领导者，比如目录条目只从领导者发送给其他的服务器。这种方式简化了对复制日志的管理并且使得Raft算法更加容易理解。

领导选举：Raft算法使用一个随机计时器来选举领导者。实质是在任何一致性算法都会实现的心跳机制上增加一点机制。在解决冲突时更加简单快捷。

成员关系调整：Raft使用一种共同一致的方法来处理集群成员更换的问题，处于调整过程中的两种不同的配置集群中大多数机器会重叠，这使得集群在成员变换的时候依然可以继续工作。

# 复制状态机
在复制状态机的方法中，一组服务器上的状态机产生相同状态的副本，并且在一些机器宕掉的情况下也可以继续运行。复制状态机在分布式系统中被用于解决很多容错的问题。例如，大规模的系统通常都有一个集群领导者，像GFS、HDFS和RAMCloud，典型应用就是一个独立的复制状态机去管理领导选举和存储配置信息并在领导人宕机的情况下也要存活下来，比如Chubby和ZooKeeper。

复制状态机基于复制日志实现的，每个服务器存储一个包含一系列指令的日志，并且按照日志的顺序进行执行。每个日志都是按照相同的顺序包含相同的指令，所以每个服务器都执行相同的指令序列。因为每个状态机都是确定的，每一次执行的操作都产生相同的状态和同样的序列。

保证复制日志相同就是一致性算法的工作，在一台服务器上，一致性模块接收客户端发送来的指令然后增加到自己的日志中去。它与其他服务器上的一致性模块进行通信来保证一个服务器上的日志最终都以相同的顺序包含相同的请求，尽管有些服务会宕机。一旦指令被正确的复制，每一个服务器的状态机按照日志顺序处理他们，然后输出结果被返回给客户端。因此，服务器集群看起来形成了一个高可靠的状态机。

实际系统中，通常要保证：

安全性保证（绝对不会返回一个错误的结果）：在非拜占庭错误情况下，包括网络延迟、分区、丢包、冗余和乱序等错误都可以保证正确

可用性：集群中只要有大多数的机器可运行并且能够相互通信、和客户端通信，就可以保证可用。因此，一个典型的包含5个节点的集群可以容忍两个节点的失败。服务器被停止就认为是失败，。当有稳定的存储时可以从状态中恢复回来并重新加入集群。

不依赖时序来保证一致性：物理时钟错误或者极端的消息延迟只有在最坏的情况下才会导致可用性问题

通常情况下， 一条指令可以尽可能快的在集群中的大多数节点响应一轮远程过程调用时完成。小部分比较慢的节点不会影响系统整体的性能。

# Raft算法
Rafit是一种用来管理章节2中描述的复制日志的算法啊，通过选举一个高贵的领导人，然后给予他全部的管理复制日志的责任来实现一致性。领导人从客户端接收日志条目，把日志条目复制到其他服务器上，并且当保证安全性的时候告诉其他的服务器应用日志条目到他们的状态机中。拥有一个领导人大大简化了对复制日志的管理。例如，领导人可以决定新的日志条目需要放在日志中的什么位置而不需要和其他的服务器上亿，并且数据都从领导人流向其他服务器。一个领导人可以宕机，可以与其他服务器失去连接，这样一个新的领导人会被选举出来。

通过领导人的方式，Raft将一致性问题分解成三个相对独立的子问题：

领导选举：一个新的领导人需要被选举出来，当现存的领导人宕机的时候

日志复制：领导人必须从客户端接收日志然后复制到集群中的其他节点，并且强制要求其他节点的日志保持和自己相同。

安全性：在Raft中安全性的关键是在图3中展示的状态机安全：如果有任何服务器节点已经应用了一个确定的日志条目到它的状态机，那么其他服务器节点不能在同一个日志索引位置应用一个不同的指令。





