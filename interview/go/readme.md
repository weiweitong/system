
# 协程与线程
子程序，或函数，层级调用，如A调用B，B调用C，C执行完返回到B，B执行完返回到A

子程序通过栈来实现的，一个线程就是执行一个子程序，子程序调用总是一个入口，一次返回，调用顺序是明确的

但协程的调用与子程序不同。协程内部可中断，转而执行别的子程序，在适当的时候再返回执行。最大的优势在于协程极高的执行效率，因为子程序切换而不是线程切换，由程序自身控制，没有线程切换的开销，线程数量越多，协程的性能优势越明显

第二个优势是不需要多线程的锁机制，因为只要一个线程，也不存在同时写变量冲突，在协程中控制共享资源不加锁，只需判断状态即可，所以执行效率比多线程高很多。

因为协程是一个线程执行，为了利用多核CPU，最简单的方法是多进程+协程，既充分利用多核，又充分发挥协程的高效率


# go优秀项目
Docker, Golang, Shadowsocks-go, Kubernetes, Etcd, Prometheus, Grafana

# Goroutine

## Goroutine的并发
go中并发指的是让某个函数独立于其他函数运行的能力，一个goroutine就是一个独立的工作单元，go的runtime会在逻辑处理器上调度这些goroutine来运行，一个逻辑处理器绑定一个操作系统线程，所以说创建一个goroutine就是创建一个协程

创建完一个goroutine后，会先存放在全局运行队列中，等待Go运行时的调度器进行调度，把他们分配给其中一个逻辑处理器，并放到这个逻辑处理器对应的本地运行队列，等着被逻辑处理器执行。

可以某个goroutine执行了一部分，可以暂停执行其他的goroutine，这一套成为go的并发。

## Goroutine的并行
多创建一个逻辑处理器，便达到了并行的目的，这样调度器可以同时分配全局运行队列中的gorouutine到不同的逻辑处理器上并行执行。


```go
func main() {
    var wg sync.WaitGroup
    wg.Add(2)
    go func() {
        defer wg.Done()
        for i := 1; i < 100; i++ {
            fmt.Println("A:", i)
        }
    }()
    go func() {
        defer wg.Done()
        for i := 1; i < 100; i++ {
            fmt.Println("B:", i)
        }
    }()
    wg.Wait()
}

```

sync.WaitGroup这里是一个计数的信号量，使用它的主要目的是main函数等待两个goroutine执行完成后再结束，使用方法是先用Add方法设置计算器为2，每一个goroutine执行完成后，defer调用Done方法减1。Wait方法的意思是，如果计数器大于0，就会阻塞，所以main函数会一直等待2个goroutine完成后，再结束。

默认情况下，Go默认给每个可用的物理处理器都分配一个逻辑处理器。可以在程序的开头使用：

```go
// 设置逻辑处理器个数为核数
runtime.GOMAXPROCS(1)

// 设置逻辑处理器个数为核数
runtime.GOMAXPROCS(runtime.NumCPU())

// 让当前goroutine暂停，退回执行队列，让其他等待的goroutine运行
runtime.Gosched()

```


对共享资源的读写必须是原子化的，同一时间只能有一个goroutine对共享资源进行读写操作


```go
import (
    "fmt"
    "runtime"
    "sync"
    "sync/atomic"
)    
var (
    count int32
    wg      sync.WaitGroup
    mutex   sync.Mutex
)
// atomic包
// LoadInt32和StoreInt32在底层使用加锁机制，
// 保证了共享资源对同步与安全
value := atomic.LoadInt32(&count)
atomic.StoreInt32(&count, value)

// sync包
mutex.Lock()
mutex.Unlock()
// 使用一个加锁和解锁机制，对共享资源进行访问
```

# 并发实例-输出所有质因数
使用Goroutine和channel输出所有质因数
```go
    package main

    import "fmt"
    
    func Processor(input <-chan int, wait chan<- struct{}){
        go func() {
            prime, ok := <-input
            if !ok {
                close(wait)
                return
            }
            fmt.Println(prime)
            output := make(chan int)
            Processor(output, wait)
            for num := range input {
                if num % prime != 0 {
                    output <- num
                }
            }
            close(output)
        }()
    }
    
    func main() {
        origin, wait := make(chan int), make(chan struct{})
        Processor(origin, wait)
        for num := 2; num < 30; num++ {
            origin <- num
        }
        close(origin)
        <-wait
    }
```



