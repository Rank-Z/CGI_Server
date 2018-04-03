# CGI_Server
A WebServer with threadpool in C++ run on Linux ,able to handle CGI request.

比较贴近于半反应堆模式(half-sync/half-reactive)，event loop 有点为Reactor而Reactor的感觉

用互斥量和条件变量来搭线程池和请求队列

C Style 和 Modern C++ Style 混杂，求轻喷
