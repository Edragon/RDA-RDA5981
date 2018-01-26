对于I2C写操作：
int write(int address, const char *data, int length, bool repeated = false);
其中，repeated为false的话，最后会发送stop，为true的话，则不会发送stop。length不包括address。

对于I2C读操作：
int read(int address, char *data, int length, bool repeated = false);
其中，repeated为false的话，最后会发送stop，为true的话，则不会发送stop。length不包括address。

读和写操作，address的R/W位需要用户自己处理。
读和写操作，都会发送start。

假如从设备的设备地址为0x23，需要读从设备的寄存器0x02的值，寄存器的值为2个字节，则执行以下操作：
char txbuf = 0x02;
char rxbuf[2] = {0};
write(0x23 << 1, (const char *)&txbuf, 1, true);
read((0x23 << 1) | 0x01, rxbuf, 2, false);