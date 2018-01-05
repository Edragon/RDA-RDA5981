Smartconfig
使用说明：
1、编译smartconfig 测试程序，并烧录到开发板运行测试程序
手机安装RDA Smartconfig apk。RDA目前只提供android apk，下载地址如下：http://bbs.rdamicro.com/forum.php?mod=viewthread&tid=166&extra=page%3D1
RDA会向合作伙伴开放APK源码。
2、手机连上路由器后，打开apk点击confirm。apk默认填充的ssid为当前连接路由器的ssid，不可手动输入，切换路由器之后需重新打开apk。
3、开发板连上路由器之后，会通过局域网向apk发送通知，apk会显示开发板的mac地址和ip地址。

清除已保存SSID信息：
开发板联网成功后，会把SSID，PASSWD等信息保存到Flash中，下次开机直接连已保存的路由器，长按开发板指定按键（约5s）可清除Flash中保存的SSID信息。
SDK默认使用的按键为：RDA5981A_HDK_V1.0的SMART键，RDA5981A/C_HDK_V1.1、RDA5981A/C_HDK_V1.2的SW7。

airkiss
airkiss和smartconfig使用方法类似，需使用微信airkiss apk。
