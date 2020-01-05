# 0. 出现的问题是:
客户端之前连接过服务端, 过一段时间后客户端再次连接服务端时, 客户端会出现一段时间的卡顿. 


# 1. recycle idle connection
报错日志-> store/tikv/client.go: 758

报错参数如下:
- target = 10.121.2.165:20174
- target = 10.121.2.167:20174
- target = 10.121.2.167:20172
- target = 10.121.2.169:20173
- target = 10.121.2.167:20171


客户端和服务器之间存在流量控制. 多个客户共享一个Tikv服务器的连接, 因此过多的并发请求可能会使Tikv服务器过载.

在客户端开始和Tikv服务器连通后, 会启动一个空连接的垃圾回收Goroutine. 它会定期检查和关闭,移除空连接.

而这个recycle信息, 表明空连接被不断回收, 而如上可以看到是, 当问题中的客户端再次连接时, 它的连接并没有成功, 所以客户端会不断重新选择不同的端口进行重连. 

上述的log日志最多只会print 10行, 如果有更多的日志则不会得到展示.
则在如上的参数中, 167客户端在短时间内尝试20174, 20172, 20171等至少三个端口, 但都没有连接成功. 

而为了让上述这些连接失败的空连接不占用共享的Tikv服务器连接, Tikv会启动一个后台的垃圾回收的Goroutine进程, 回收空连接. 显示在服务端的输入就是如上的 recycle idle connection.


# 2. wait response is cancelled, cause= context deadline exceeded
报错日志-> store/tikv/client.go:663

报错参数如下:
- to=10.121.2.167:20174
- to=10.121.2.167:20171

从这个简短的报错日志中, 可以看出因为上下文内容超时, 等待响应被取消.


# 3. switch region peer to next due to send request fail
报错日志-> region_cache.go:373

报错参数如下:
- region id: 36
- meta id: 36
- region_epoch: conf_ver:11 version:4
- peers: <id:37, store_id:1>
- peers: <id:48, store_id:12>
- peers: <id:51, store_id:14>
- peers: <id:48, store_id:12, addr: 10.121.2.167:20174, idx: 1>
- needReload=false
- error="context deadline exceeded"
- errorVerbose(冗长error, 具体分析在下面)="context deadline exceed"

从参数中, 36号region内有三个peers, 37, 48, 51号服务器以及对应的store id.


仔细查看这个冗长的error:
- errors.go:174, 通过errors的Trance功能, 得到以下:
- juju_adaptor.go:15
- tikv.sendBatchRequest
- store/tikv/client.go:665
- tidb/store/tikv.(*rpcClient).SendRequest
- store/tikv/client.go:689
- tidb/store/tikv.(*RegionRequestSender).sendReqToRegion
- store/tikv/region_request.go:145
- tidb/store/tikv.(*RegionRequestSender).SendReqCtx
- store/tikv/region_request.go:116
- store/tikv.(*RegionRequestSender).SendReq
- tikv/region_request.go:72
- store/tikv.(*RawKVClient).doBatchPut
- tikv/rawkv.go:659
- store/tikv.(*RawKVClient).sendBatchPut.func1
- tikv/rawkv.go:589
- runtime.goexit
- /usr/local/go/src/runtime/asm_amd64.s:1337


可以看出error跟BatchRequest有关, 批量请求命令可能是导致再连接时客户端出现一段时间卡顿的元凶.

在tidb的[#11283](https://github.com/pingcap/tidb/pull/11283)号issue中, 重构了空闲批量连接的垃圾回收代码. 

这个批量命令的连接实际上是由一个channel(通道)和一个goroutine(go协程)共同构成的. 尽管如此, 这个简单的结构有许多的缺陷:
- 这个协程出错或者退出后, 仍然会阻塞对通道发消息
- 连接也许已经为空连接了, 但是协程不会被回收
- 大量的data racing导致并发量太大
- 两次关闭同一个通道

关闭一个批量连接, 并不仅仅是单纯的删除关闭通道或者让这个协程退出, 一不小心就会触发上述的缺陷, 导致服务崩溃. 正确的关闭应该委托给rpcClient, 让它从connArray这个map中把批量连接中两个元素的对应关系移除后, 再删除该批量连接.

在tidb的[#10616](https://github.com/pingcap/tidb/pull/10616)号issue中, 修改了批量命令的回收机制. 在一个批量发送的环中, 增加一个计时器用于探测空闲状态. 如果批量命令的通道不再接受消息并且超过计时器的时间, 标记让rpcClient从ConnArray这个map中删掉此批量命令. 

在上面的报错日志中, 可以看到error发生在RawKVClient上. 而RawKVClient的真实结构如下:

```go
type RawKVClient struct {
	clusterID   uint64
	regionCache *RegionCache
	pdClient    pd.Client
	rpcClient   Client
}
```
RawKVClient 是Tikv存储服务器的一个客户端, 只有GET/PUT/DELETE 这三种命令. 

而上面的错误报告, 可以看出错误出在PUT上. 

在tidb的1.1版本中, 是没有BatchPut的, 但是可以通过原生的Put推测:
```go
func (c *RawKVClient) Put(key, value []byte) error {
    ...
    	resp, _, err := c.sendReq(key, req)
	if err != nil {
		return errors.Trace(err)
	}
	cmdResp := resp.RawPut
	if cmdResp == nil {
		return errors.Trace(ErrBodyMissing)
	}
	if cmdResp.GetError() != "" {
		return errors.New(cmdResp.GetError())
	}
    return nil
}
```

可以推测得出的是, 应该是在此Put函数中, 发生了错误, 导致出现客户端卡顿的情况.

具体问题, 因为按照错误日志, 版本应该是升级过了, 所以应当先确定版本, 再更深入的跟进此问题. 


# 4. switch region leader to specific leader dut to kv return NotLeader
报错日志-> region_cache.go:516

报错参数如下:
- regionID=40
- currIdx=2
- leaderStoreID=7

- regionID=36
- currIdx=2
- leaderStoreID=12

参数中, 第40号的Region的Leader丢失, 转移40号的Leader到7号服务器, 并将对Region的日志复制指针指向2, 意味着从第2个任期开始向Followers发送复制log.

而36号的Region的发生了同样的Leader丢失, 转移Leadder到12号服务器, 也是从第二个任期开始向成员复制.







