# limit_rate
主要更改和参考 纸鸢 开源的限速插件 https://github.com/cnkedao/trafficserver-ratemaster

limit_rate是一个remap插件，分时间段进行限速，0hour=1048576 代表00点 限速为1048576字节/s：

#For example:
map http://xie.test.cn http://xie.test.cn  @plugin=/xxx/trafficserver/libexec/trafficserver/balancer.so @pparam=--policy=roundrobin @pparam=127.0.0.1  @plugin=/xxx/trafficserver/libexec/trafficserver/limit_rate.so @pparam=/xxx/limitconfig.config
 
#limitconfig.config:
0hour=1048576
1hour=1048576
2hour=1048576
3hour=1048576
4hour=1048576
5hour=1048576
6hour=1048576
7hour=1048576
8hour=1048576
9hour=1048576
10hour=1048576
11hour=1048576
12hour=1048576
13hour=1048576
14hour=1048576
15hour=1048576
16hour=1048576
17hour=1048576
18hour=1048576
19hour=1048576
20hour=1048576
21hour=1048576
22hour=1048576
23hour=1048576
