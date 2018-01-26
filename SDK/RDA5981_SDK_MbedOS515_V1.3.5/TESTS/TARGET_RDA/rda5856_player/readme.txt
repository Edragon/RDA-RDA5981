RDA5856是我司的一款BT（支持BLE）+CODEC芯片，目前支持mp3、aac、m4a、wav和ts流的播放，8K和16K采样率录音。

1、rda58xx.h和rda58xx.cpp提供直接操作RDA5856的接口（class rda58xx），player.h和player.cpp提供间接操作RDA5856的接口（class Player）。
2、开机默认进入CODEC模式，首先调用Player::isReady或者rda58xx::isReady，直到返回true才可以对RDA5856进行操作。
3、调用player::begin，扫描SD卡根目录里的音频文件并将文件名存放至list中，默认最多保存16首，文件名最长32个字节，可自行更改。
4、调用player::playFile播放音频文件，传入的参数为文件的路径，如"a.mp3"。
5、调用player::recordFile开始录音，传入的参数为文件的路径，如"a.wav"。

播放流程：
1、调用rda58xx::bufferReq(mci_type_enum ftype, uint16_t size, uint16_t threshold)开始播放。
其中ftype为音频类型，size为向RDA5856申请的buffer大小，目前最大为8K字节。
threshold为RDA5856开始播放前接收到的数据量，假如设置为4K，则RDA5856在接收到>4K数据后才开始播放。
2、循环调用rda58xx::sendRawData(uint8_t *databuf, uint16_t n)向RDA5856发送音频数据。
其中*databuf为数据指针，n为数据总数，目前n最大为2K。
3、播放完毕后，调用rda58xx::stopPlay(rda58xx_stop_type stype)停止播放。
stype为停止播放的模式，目前有WITH_ENDING和WITTOUT_ENDING两种。
WITH_ENDING模式，RDA5856会将其buffer内缓存的数据播放完毕。
可以通过调用rda58xx::getCodecStatus来确认其状态，如果其状态一直未PLAY，则说明还未播放完毕。
WITHOUT_ENDING模式，RDA5856会直接停止播放，其buffer内缓存的数据会被丢弃。
注意：m4a封装的aac，需要先转换成ADTS格式，再发送给RDA5856。Player::m4aPlay提供了转换方法。

录音流程：
1、调用rda58xx::startRecord(mci_type_enum ftype, uint16_t size, uint16_t sampleRate)开始录音。
其中ftype为音频类型，默认为wav；size为接收buffer大小，默认640字节；sampleRate为采样率，可以设置8K或者16K，默认为8K。
2、当接收buffer状态为FULL时，取走buffer内的数据，并设置buffer状态为EMPTY。
if (FULL == rda5856.getBufferStatus()) {
	rda5856.clearBufferStatus();
    fp->write(rda5856.getBufferAddr(), rda5856.getBufferSize());
	fileSize += rda5856.getBufferSize();
}
3、调用rda58xx::stopRecord(void)停止录音。
注意：如果要设置MIC增益，在开始录音前调用rda58xx::setMicGain(uint8_t gain)，其中gain为录音增益，最大值为12。

