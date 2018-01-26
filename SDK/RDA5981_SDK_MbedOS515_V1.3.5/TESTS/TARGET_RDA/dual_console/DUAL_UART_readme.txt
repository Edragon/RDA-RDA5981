关于双console的说明：

1） 目前代码里默认UART0为log的打印串口，基于各个用户需求的不同，需要用户手动修改文件\hal\targets\hal\target_rda\target_uno_91h\PeripheralNames.h：
    STDIO_UART_TX 从 UART0_TX 改成 UART1_TX
    STDIO_UART_RX 从 UART0_RX 改成 UART1_RX
    STDIO_UART    从 UART_0   改成 UART_1

2）两个串口都可以响应AT命令，AT命令参考AT_readme.txt;

3）基于1）的修改，用户使用printf函数打印在串口1，用户可以使用console_uart0_printf从串口0打印;

4) 编译case参考指令：
   mbed compile -t ARM -m UNO_91H --source TESTS/TARGET_RDA/dual_console/ --source ./
   
5) 默认RDA_AT_DEBUG的宏打开，如果用户不需要可以自行关闭。