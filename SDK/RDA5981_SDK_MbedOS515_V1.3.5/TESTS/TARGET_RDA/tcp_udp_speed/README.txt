1. 测试工具
   1) TCP/UDP测试工具,可参考使用case文件夹中的TCP/UDP测试工具
   2) windows操作系统PC一台
   3) 无线路由器一台

2. 相关命令

    命令部分一律使用大写，如AT，以\r\n结尾，
    参数部分以“,”分隔，字符串部分建议使用引号，防止包含特殊符号时出错，如AT+WSCONN=“a”,“qqqqqqqq”

    命令类型：
    设置命令：AT+<x>=<...>  如AT+WSCONN=“a”,“qqqqqqqq” 
    查询命令：AT+<x>=?      如AT+WSCONN=？
    执行命令：AT+<x>        如AT+WSDISCONN

    返回值
    +ok                     执行成功
    +ok=<...>               执行成功且有返回信息
    +error=err code         执行失败并返回错误码

    err code：
    无此指令：  -1
    不允许执行：-2
    执行失败：  -3
    参数有错误：-4

    2.1 连接到路由
    AT+WSCONN                       查询连接状态或者执行连接操作
    1) AT+WSCONN=？                 查看当前连接状态，若已连接，返回AP名称，信号强度及IP地址
    2) AT+WSCONN=“a”,“qqqqqqqq”     连接wifi，参数为用户名，密码，非加密时可以不输入密码

    2.2 测试命令
    AT+NSTART                       建立TCP或UDP连接，并进行TX/RX速度测试
    1) AT+NSTART=TCP,TX,192.168.1.101,3434
       建立TCP连接，并进行TX速度测试。最后2个参数分别为SERVER IP，SERVER port。
       其中，192.168.1.101为PC的IP地址，3434为测试工具创建的TCP服务器的端口号

    2) AT+NSTART=TCP,RX,192.168.1.101,3434
       建立TCP连接，并进行RX速度测试。最后2个参数分别为SERVER IP，SERVER port。
       其中，192.168.1.101为PC的IP地址，3434为测试工具创建的TCP服务器的端口号
    
    3) AT+NSTART=UDP,TX,192.168.1.101,1234,4321  
       建立UDP连接，并进行TX速度测试。最后3个参数分别为SERVER IP，SERVER port，LOCAL port.
       192.168.1.101为PC的IP地址，1234为PC端端口号，4321为本地端口号。

    4) AT+NSTART=UDP,RX,192.168.1.101,1234,4321  
       建立UDP连接，并进行RX速度测试。最后3个参数分别为SERVER IP，SERVER port，LOCAL port. 192.168.1.101为PC的IP地址，1234为PC端端口号，4321为本地端口号。
    
    2.3 断开路由器连接
    AT+WSDISCONN                     断开连接

3. 测试方法
   3.1 PC端使用网线连接到路由器,查看PC端IP地址（例如：192.168.1.101）。
   3.2 下载bin文件并运行
   3.3 测试
      1) 连接到路由器，输入命令：AT+WSCONN=“a”，“qqqqqqqq
      2) 终端显示Connect a success! Client IP Address is 192.168.1.xxx,连接成功。
      3) TCP测试
         打开TCP/UDP测试工具，创建服务器，并设置端口号（3434）。
         TX测试命令：AT+NSTART=TCP,TX,192.168.1.101,3434
         RX测试命令：AT+NSTART=TCP,RX,192.168.1.101,3434
         测试RX需要在工具端点击发送文件或者在发送区输入文本，点击“发送”按钮开始测试。
      4) UDP测试
         在TCP/UDP测试工具端建立UDP连接，类型为UDP，目标IP为5991H连接到路由时终端打印的IP地址192.168.1.xxx,
         并填写端口号(4321)以及指定PC端的端口号(1234)。填写完成，点击“创建”。
         TX测试命令: AT+NSTART=UDP,TX,192.168.1.101,1234,4321
         RX测试命令：AT+NSTART=UDP,TX,192.168.1.101,1234,4321
         测试RX需要在工具端点击发送文件或者在发送区输入文本，点击“发送”按钮开始测试。
      5) 断开到路由器的连接
         输入命令：AT+WSDISCONN

4. 其他注意事项
    4.1 测试工具中选项设置中的“每次发送数据块大小”会影响UDP RX的速度。
        测试时如果速度不高，可适当调整其值。
