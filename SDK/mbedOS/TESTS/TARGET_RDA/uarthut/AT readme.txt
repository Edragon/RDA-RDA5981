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

1.6 AT+WDBG							调整各模块的debug level
	AT+WDBG=DRV,2					调整driver debug level，级别为0~3，默认为0
	AT+WDBG=WPA,2					调整wpa debug level，级别为0~3，默认为0
	AT+WDBG=DRVD,1					打开/关闭driver dump，会打印出收发的数据，1为打开，0为关闭，默认为关闭
	AT+WDBG=WPAD,1					打开/关闭WPA dump，会打印出某些数据，1为打开，0为关闭，默认为关闭
	AT+WDBG=HUTD,1                  打开/关闭HUT dump，会打印出某些数据，比如每秒自动打印一次RX测试结果，1为打开，0为关闭，默认为关闭
		
2.efuse命令
开始的R表示Read，W表示Write。最后的E表示Efuse。所有传入的参数为必须为16进制，1个byte。
2.1 AT+RE                           读取所有efuse内容
	efuse共15个page，每个page2个bytes。用户可以读出page2~15的内容，读出的内容按照[page2 byte0] [page2 byte1] [page3 byte0] ... [page15 byte1]排列。
			byte1		        	byte0	
	page0	rda_reserved			
	page1	rda_reserved			
	page2	usb pid			
	page3	usb vid			
	page4	mac address[4]			mac address[5]	
	page5	mac address[2]			mac address[3]	
	page6	mac address[0]			mac address[1]	
	page7	mac address[4]_bak		mac address[5]_bak	
	page8	mac address[2]_bak		mac address[3]_bak	
	page9	mac address[0]_bak		mac address[1]_bak	
	page10	crystal_cal_bak1		crystal_cal	
	page11	crystal_cal_bak3		crystal_cal_bak2	
	page12	rda_reserved:Bit[12~15]	Tx_power: Bit[6~11]:b mode Bit[0~5]:g/n mode		
	page13	rda_reserved:Bit[12~15]	Tx_power_bak1: Bit[6~11]:b mode Bit[0~5]:g/n mode		
	page14	rda_reserved:Bit[12~15]	Tx_power_bak2: Bit[6~11]:b mode Bit[0~5]:g/n mode		
	page15	rda_reserved:Bit[12~15]	Tx_power_bak3: Bit[6~11]:b mode Bit[0~5]:g/n mode		

2.2 AT+WTPE                         写tx power到efuse
	AT+WTPE=n1,n2
	n1为11g/n模式下的TX POWER，范围为0x25~0x64。
	n2为11b模式下的TX POWER，范围为0x15~0x54。
	最少可以写入四次。
	
2.3 AT+RTPE                         从efuse读tx power
	返回的数据格式与写入的数据格式相同。
	
2.4 AT+WXCE                			写晶体校准到efuse
	AT+WXCE=n1
	n1为1个byte，取bit7~bit1，bit0无效，最少可以写入四次。

2.5 AT+RXCE                			从efuse读晶体校准
	返回的数据格式与写入的数据格式相同。
	
2.6 AT+WMACE                        写MAC地址到efuse
	AT+WTPE=n1,n2,n3,n4,n5,n6
	n1的最低位不能为0，n1~n6不能全为0。最少可以写入两次。
	
2.7 AT+RMACE                        从efuse读MAC地址
	返回的数据格式与写入的数据格式相同。

3.RF寄存器通用命令
开始的R表示Read，W表示Write。AC表示所有All Channels，SC表示Single Channel。所有的寄存器地址和值必须为16进制，channel可以为10进制。
在进行Read时，_CUR表示从寄存器读取，_DEF表示从flash读取。进行Write时，_CUR表示只写入寄存器，_DEF表示写入寄存器的同时写入flash。
3.1 AT+WRF_CUR                      写RF寄存器，此寄存器与channel无关
	AT+WRF_CUR=n1,n2
	n1,n2均为half word。n1为寄存器地址，n2为要写入的值。
	AT+WRF_CUR=0xDA,0x80

3.2 AT+WRF_DEF                      写RF寄存器，此寄存器与channel无关
	AT+WRF_DEF=n1,n2
	n1,n2均为half word。n1为寄存器地址，n2为要写入的值。
	AT+WRF_DEF=0xDA,0x80	
	
3.3 AT+WRFAC_CUR                    写RF寄存器，此寄存器与channel有关
	AT+WRFAC_CUR=n1,n2,n3,...,n15
	n1~n15均为half word。n1为寄存器地址，n2~n15依次为channel1~14的值。
	AT+WRFAC_CUR=0x8A,0x69A0,0x69A0,...,0x6820

3.4 AT+WRFAC_DEF                    写RF寄存器，此寄存器与channel有关
	AT+WRFAC_DEF=n1,n2,n3,...,n15
	n1~n15均为half word。n1为寄存器地址，n2~n15依次为channel1~14的值。
	AT+WRFAC_DEF=0x8A,0x69A0,0x69A0,...,0x6820	
	
3.5 AT+WRFSC_CUR			        写RF寄存器，只写某个channel的值
	AT+WRFSC_CUR=n1,n2,n3
	n1和n3为half word。n1为寄存器地址，n2为channel，n3为要写入的值。
	AT+WRFSC_CUR=0x8A,1,0x69A0

3.6 AT+WRFSC_DEF			        写RF寄存器，只写某个channel的值
	AT+WRFSC_DEF=n1,n2,n3
	n1和n3为half word。n1为寄存器地址，n2为channel，n3为要写入的值。
	AT+WRFSC_DEF=0x8A,1,0x69A0	
	
3.7 AT+RRF_CUR                      读RF寄存器，此寄存器与channel无关
	AT+RRF_CUR=n1
	n1为寄存器地址，half word。返回的数据格式与写入的数据格式相同。
	AT+RRF_CUR=0xDA

3.8 AT+RRF_DEF                      读RF寄存器，此寄存器与channel无关
	AT+RRF_DEF=n1
	n1为寄存器地址，half word。返回的数据格式与写入的数据格式相同。
	AT+RRF_DEF=0xDA	
	
3.9 AT+RRFAC_CUR                    读RF寄存器，此寄存器与channel有关
	AT+RRFAC_CUR=n1
	n1为寄存器地址，half word。
	AT+WRFAC_CUR=0x8A

3.10 AT+RRFAC_DEF                   读RF寄存器，此寄存器与channel有关
	AT+RRFAC_DEF=n1
	n1为寄存器地址，half word。
	AT+WRFAC_DEF=0x8A	
	
3.11 AT+RRFSC_CUR				    读RF寄存器，只读某个channel的值
	AT+WRFSC_CUR=n1,n2
	n1为寄存器地址，half word。n2为channel。
	AT+WRFSC_CUR=0x8A,1

3.12 AT+RRFSC_DEF				    读RF寄存器，只读某个channel的值
	AT+WRFSC_DEF=n1,n2
	n1为寄存器地址，half word。n2为channel。
	AT+WRFSC_DEF=0x8A,1	

3.13 AT+DRF				            dump flash中保存的RF寄存器及其对应的值，这些寄存器与channel无关

3.14 AT+DRFAC				        dump flash中保存的RF寄存器及其对应的值，这些寄存器与channel有关

3.15 AT+ERF				    		擦除flash中保存的RF寄存器及其对应的值，这些寄存器与channel无关
	AT+ERF=n1
	n1为寄存器地址，half word。
	AT+ERF=0xDA

3.16 AT+ERFAC				        擦除flash中保存的RF寄存器及其对应的值，这些寄存器与channel有关
	AT+ERF=n1
	n1为寄存器地址，half word。
	AT+ERFAC=0x8A
	
4. PHY寄存器通用命令
开始的R表示Read，W表示Write。ALL表示所有All Channels，SC表示Single Channel。所有的寄存器地址和值必须为16进制，channel可以为10进制。
在进行Read时，_CUR表示从寄存器读取，_DEF表示从flash读取。进行Write时，_CUR表示只写入寄存器，_DEF表示写入寄存器的同时写入flash。
4.1 AT+WPHY_CUR                     写PHY寄存器，此寄存器与channel无关
	AT+WPHY_CUR=n1,n2
	n1,n2均为word。n1为寄存器地址，n2为要写入的值。
	AT+WPHY_CUR=0x11F,0x45

4.2 AT+WPHY_DEF                     写PHY寄存器，此寄存器与channel无关
	AT+WPHY_DEF=n1,n2
	n1,n2均为word。n1为寄存器地址，n2为要写入的值。
	AT+WPHY_DEF=0x11F,0x45	
	
4.3 AT+WPHYAC_CUR                   写PHY寄存器，此寄存器与channel有关
	AT+WPHYAC_CUR=n1,n2,n3,...,n15
	n1~n15均为word。n1为寄存器地址，n2~n15依次为channel1~14的值。

4.4 AT+WPHYAC_DEF                   写PHY寄存器，此寄存器与channel有关
	AT+WPHYAC_DEF=n1,n2,n3,...,n15
	n1~n15均为word。n1为寄存器地址，n2~n15依次为channel1~14的值。
	
4.5 AT+WPHYSC_CUR			        写PHY寄存器，只写某个channel的值
	AT+WPHYSC_CUR=n1,n2,n3
	n1和n3为word。n1为寄存器地址，n2为channel，n3为要写入的值。

4.6 AT+WPHYSC_DEF			        写PHY寄存器，只写某个channel的值
	AT+WPHYSC_DEF=n1,n2,n3
	n1和n3为word。n1为寄存器地址，n2为channel，n3为要写入的值。
	
4.7 AT+RPHY_CUR                     读PHY寄存器，此寄存器与channel无关
	AT+RPHY_CUR=n1
	n1为寄存器地址，word。返回的数据格式与写入的数据格式相同。
	AT+RPHY_CUR=0x11F

4.8 AT+RPHY_DEF                     读PHY寄存器，此寄存器与channel无关
	AT+RPHY_DEF=n1
	n1为寄存器地址，word。返回的数据格式与写入的数据格式相同。
	AT+RPHY_DEF=0x11F	
	
4.9 AT+RPHYAC_CUR                   读PHY寄存器，此寄存器与channel有关
	AT+RPHYAC_CUR=n1
	n1为寄存器地址，word。

4.10 AT+RPHYAC_DEF                  读PHY寄存器，此寄存器与channel有关
	AT+RPHYAC_DEF=n1
	n1为寄存器地址，word。
	
4.11 AT+RPHYSC_CUR				    读PHY寄存器，只读某个channel的值
	AT+WPHYSC_CUR=n1,n2
	n1为寄存器地址，word。n2为channel。

4.12 AT+RPHYSC_DEF				    读PHY寄存器，只读某个channel的值
	AT+WPHYSC_DEF=n1,n2
	n1为寄存器地址，word。n2为channel。

4.13 AT+DPHY				        dump flash中保存的PHY寄存器及其对应的值，这些寄存器与channel无关

4.14 AT+DPHYAC				        dump flash中保存的PHY寄存器及其对应的值，这些寄存器与channel有关

4.15 AT+EPHY				        擦除flash中保存的PHY寄存器及其对应的值，这些寄存器与channel无关
	AT+EPHY=n1
	n1为寄存器地址，word。
	AT+EPHY=0x11F

4.16 AT+EPHYAC				        擦除flash中保存的PHY寄存器及其对应的值，这些寄存器与channel有关
	AT+EPHYAC=n1
	n1为寄存器地址，word。
	
5. 读写MAC地址命令
开始的R表示Read，W表示Write。读写MAC地址比较特殊，_DEF只写入flash和只从flash读取。
MAC地址必须为16进制。

5.1 AT+WMAC_DEF                     写MAC地址，只写入flash
	AT+WMAC_DEF=n1,n2,n3,n4,n5,n6
	n1~n6为MAC地址，均为1个byte。
	AT+WMAC_DEF=0x64,0x20,0xAB,0xC1,0x1D,0x34
	
5.2 AT+RMAC_DEF					    读MAC地址，从flash中读取

6. HUT模式下收发测试命令
所有的参数都是10进制的
6.1 AT+TXSTART						开始TX测试
	AT+TXSTART=n1,n2,n3,n4,n5,n6
	n1：主信道，取值范围1~14。
	n2：模式，0代表b/g mode，1代表11n green field mode，2代表11n mixed mode。
	n3：信道带宽，0代表20M，1代表40M。
	n4: 信号带宽，0代表全40M，1代表主信道为upper，2代表20M，3代表主信道为lower。
	n5：速率，11b/g模式取值为1 2 5 6 9 11 12 18 24 36 48 54。11n模式取值为0~7，代表mcs0到mcs7。
	n6：发送包长，单位为byte。
	注意：在信道带宽为40m时，若主信道为1~4，信号带宽只能选择0或者3，若主信道为10~13，信号带宽只能选择0或者1。 
	AT+TXSTART=1,0,0,2,54,1024
	
6.2 AT+TXSTOP						停止TX测试

6.3 AT+TXRESTART				    按照上次的配置重新开始TX测试
	仅在上一次测试为TX测试才有效
	
6.4 AT+RXSTART						开始RX测试
	AT+TXSTART=n1,n2,n3,n4
	n1~n4的定义与6.1相同，RX测试不需要设置速率和包长。
	AT+RXSTART=1,0,0,2

6.5 AT+RXSTOP						停止RX测试

6.6 AT+RXRESULT						读取RX测试结果
	中断打印：time:n1 recv:n2 fcs_passed:n3 per:n4
	n1为测试的时间，n2为从开始测试到获取结果期间收到的总的数据量，n3为n2中fcs pass的数据量，n4为fcs pass的百分比。

6.7 AT+RXRESTART				    按照上次的配置重新开始RX测试
	仅在上一次测试为RX测试才有效

6.8 AT+WTPOFS_DEF                   写tx power g/n偏置，g > n，写入flash
	AT+WTPOFS_DEF=n1
	n1取值范围0~16

6.9 AT+RTPOFS_DEF                   读tx power g/n偏置，从flash读取
