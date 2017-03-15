# limit_rate
 主要参考和更改 纸鸢 开源的限速插件 https://github.com/cnkedao/trafficserver-ratemaster
 
 limit_rate是一个remap插件，分时间段进行限速，0hour=1048576 代表00点 限速为1048576字节/s：

# For example:
 map http://xie.test.cn http://xie.test.cn  @plugin=/xxx/trafficserver/libexec/trafficserver/balancer.so @pparam=--policy=roundrobin @pparam=127.0.0.1  @plugin=/xxx/trafficserver/libexec/trafficserver/limit_rate.so @pparam=/xxx/limitconfig.config
 
# limitconfig.config:
 0hour=1048576<br />
 1hour=1048576<br />
 2hour=1048576<br />
 3hour=1048576<br />
 4hour=1048576<br />
 5hour=1048576<br />
 6hour=1048576<br />
 7hour=1048576<br />
 8hour=1048576<br />
 9hour=1048576<br />
 10hour=1048576<br />
 11hour=1048576<br />
 12hour=1048576<br />
 13hour=1048576<br />
 14hour=1048576<br />
 15hour=1048576<br />
 16hour=1048576<br />
 17hour=1048576<br />
 18hour=1048576<br />
 19hour=1048576<br />
 20hour=1048576<br />
 21hour=1048576<br />
 22hour=1048576<br />
 23hour=1048576<br />
