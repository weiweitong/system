# 手撕代码（LRU） 实现遍历和插入这两个函数。

```go



```


# 二分搜索


```go



```



# 求二叉树中的两个节点间的最大距离


```go



```




# 堆排序


```go



```



# csp并发


```go

func main() {
	origin, wait := make(chan int), make(chan struct{})
	Processor(origin, wait)
	for num := 2; num < 1000; num++ {
		origin <- num
	}
	close(origin)
	<-wait
}

func Processor(seq chan int, wait chan struct{}) {
	go func() {
		prime, ok := <-seq
		if !ok {
			close(wait)
			return
		}
		fmt.Println(prime)
		out := make(chan int)
		Processor(out, wait)
		for num := range seq {
			if num % prime != 0 {
				out <- num
			}
		}
		close(out)
	}()
}
```



# 一个链表，里面四个元素，每个元素是一个tuple，每天tuple里面两个值，一个数字一个字母。数字是后面字母的权重。。。然后随机生成100内的数字，根据权重输出字母。。。


```go



```



# find the largest kth number in list

```go



```


