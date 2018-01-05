
命令部分一律使用大写，如AT，以\r\n结尾，
参数部分以“,”分隔，字符串部分建议使用引号，防止包含特殊符号时出错，如AT+WSCONN=“a”,“qqqqqqqq”
未具体解释的命令均为执行命令，直接执行即可，如AT+RST

命令类型：
设置命令：AT+<x>=<...>  如AT+WSMAC=123456789abc
查询命令：AT+<x>=?		如AT+WSMAC=？
执行命令：AT+<x>		如AT+RST

返回值
+ok            			执行成功
+ok=<...>				执行成功且有返回信息
+error=err code			执行失败并返回错误码

err code：
无此指令：	-1
不允许执行：-2
执行失败：	-3
参数有错误：-4


1.基础命令
1.1	AT 								测试AT模式是否使能
1.2 AT+H							查看AT帮助
1.3	AT+RST 							重启模块
1.4	AT+VER	 						查看软件版本
1.5 AT+ECHO         				打开或者关闭串口回显
	AT+ECHO=1/0						1表示打开，0表示关闭，启动后默认是打开的
1.6	AT+UART 						查询或者设置UART信息，支持保存至flash，目前只有波特率
	AT+UART=?						查看波特率，返回如+ok=115200
	AT+UART=0,115200  				设置波特率，0表示临时设置，1表示永久设置，将保存至flash，下次启动时自动加载，返回如+ok
1.7 AT+USERDATA      				写入或者读出用户数据
	AT+USERDATA=?					读取用户数据，返回如+ok=abcdef
	AT+USERDATA=10   				写入用户数据，参数为要写入长度，在输入制定长度数据后返回+ok
1.8 AT+SLEEP						使能/关闭sleep功能
	AT+SLEEP=1/0					1表示使能，0表示关闭，默认为0不开启
1.9 AT+RESTORE						清除flash中保存的信息，包括串口波特率以及其他网络信息（详见后续命令）

2.WIFI命令
*** W表示WIFI，S表示STA模式，A表示AP模式
2.1 AT+WSMAC 						查询或者设置MAC地址
	AT+WSMAC=?      				查询MAC地址，返回如+ok=123456789abc
	AT+WSMAC=123456789abc			设置MAC地址，返回如+ok
									STA和AP MAC间有关联，所以只有STA MAC可以设置，两者同步改变，只有STA AP均未使能时可以设置

2.2 AT+WSSCAN	                    扫描当前空中可用AP
	AT+WSSCAN						执行扫描操作
	AT+WSSCAN=? 					查看当前扫描结果，为保证时效性，查询前都会进行扫描操作，
									如果没有扫描到特定AP可以重复执行，避免一次扫描不全的问题
	
2.3 AT+WSCONN 						查询连接状态或者执行连接操作，支持保存ssid及密码至flash
	AT+WSCONN						若flash中保存有连接信息，则读出并连接，若之前未保存过则报错
	AT+WSCONN=?                     查看当前连接状态，若已连接，返回AP名称，信号强度及IP地址
	AT+WSCONN=0,"a","qqqqqqqq"    	连接wifi，参数为用户名，密码，非加密时可以不输入密码，
									0表示不保存至flash，1表示保存，下次可以不输入ssid等信息直接连接
2.4 AT+WSC=0/1						以smartconfig模式进行连接，0表示连接成功后不保存ssid等信息，1表示保存
	
2.5 AT+WSDISCONN					断开连接

2.6 AT+WSFIXIP						使能/禁用固定IP地址，支持保存至flash，该命令只在sta未连接时有效
	AT+WSFIXIP=0,1,192.168.1.100,255.255.255.0,192.168.1.1
	0表示不保存至flash，仅本次连接生效，1表示使能FIXIP功能，后面分别为IP，子网掩码以及网关
	连接时，若发现有过FIXIP相关设置则按设置内容连接，若未设置则尝试从flash中读取信息，若读取失败则按DHCP处理

2.7 AT+WDBG							调整各模块的debug level
	AT+WDBG=DRV,2					调整driver debug level，级别为0~3，默认为0
	AT+WDBG=WPA,2					调整wpa debug level，级别为0~3，默认为0
	AT+WDBG=DRVD,1					打开/关闭driver dump，会打印出收发的数据，1为打开，0为关闭，默认为关闭
	AT+WDBG=WPAD,1					打开/关闭WPA dump，会打印出某些数据，1为打开，0为关闭，默认为关闭
	
2.8 AT+WAP							使能AP
	AT+WAP							使能AP，从flash中读取ssid等信息，若读取失败则报错
	AT+WAP=0,6,a,qqqqqqqq			使能AP，0表示不保存至flash，1为保存，其余为channel，ssid和密码，非加密时无需输入密码
	
2.9 AT+WSTOPAP						停止AP，断开现有一切连接	

2.10 AT+WAMAC=? 					查询AP MAC地址，STA和AP MAC间有关联，所以只有STA MAC可以设置，两者同步改变

2.11 AT+WASTA						查看已连接至AP的STA

2.12 AT+WANET						设置AP的网络信息，包括IP，子网掩码，网关以及DHCP的起始和结束
	 AT+WANET=0,192.168.1.100,255.255.255.0,192.168.1.1,192.168.1.101,192.168.1.120
	 0表示不保存至flash，1为保存，其余为IP，子网掩码，网关以及DHCP的起始和结束
	 使能AP时，若设置过则按设置内容启动，若未设置则尝试从flash中读取信息，若读取失败则按代码中默认参数建立
	
3.NET命令
3.1 AT+NSTART						建立TCP或UDP连接，返回的参数表示链接号，目前最多支持4个
	AT+NSTART=UDP,192.168.1.100,1234,4312
	建立UDP连接，参数为SERVER IP，SERVER port，LOCAL port，若不输入LOCAL port，则不绑定本地端口，返回如+ok=0,0表示第0个连接
	AT+NSTART=TCP,192.168.1.100,1234
	建立TCP连接，参数为SERVER IP，SERVER port   返回如+ok=1，1表示第一个链接
	在连接断开时不会进行重连，会提示“+LINKDOWN=0”，0表示链接号
	接收到数据时会以“+IPD=链接号,数据长度,服务器地址,服务器端口,数据”的格式返回
	
3.2 AT+NSTOP 						断开TCP或UDP连接，参数为之前建立的链接号
	AT+NSTOP=0 						断开第0个链接，返回+ok
	
3.3 AT+NSEND 						通过某个TCP或者UDP发送数据
	AT+NSEND=0,10					通过第0个链接发送10个字节数据，发送完成后回复+ok
									
3.4 AT+NMODE 						将某个链接设置为透传模式
	AT+NMODE=0  					将第0个链接设置为透传模式，所有串口数据都会被发送，所有接收数据也会被输出到串口
****关于退出透传模式
	退出透传模式时，先暂停200ms，确保串口内的数据被发送清空，之后输入“+++”，待收到回复“a”后在输入“a”，在暂停200ms，确保后续没有数据输入即可退出透传模式
	退出透传模式，对应链接会被关闭，“+++”和“a”前后不要有任何输入，包括"\r\n"， 以尽量避免误操作，使用SecureCRT时做成button比较方便
	退出透传模式时，该socket会被关闭，不会恢复为普通接口，且断开时会不断重连直至退出透传模式

3.5 AT+NLINK						查看当前连接，包括连接类型，目标地址，目标端口以及本地端口（如果设置了的话） 	

3.6 AT+NPING						触发PING操作 
	AT+NPING=192.168.1.100,200,1,32,1
	开始执行PING操作，参数分别为：
	目标IP，
	执行次数，
	发送时间间隔（以秒为单位，可选值1-10秒之间，大于10秒取值10秒，小于1秒取值1秒），
	发送数据包长度（最大值为14600，超过此值按最大值算）。
	是否打印回显（1为打印回显，0为关闭回显）。
	返回值：如+ok。

3.7 AT+NDNS 						查询域名的IP地址
	AT+NDNS="www.baidu.com"		    查询百度网址








