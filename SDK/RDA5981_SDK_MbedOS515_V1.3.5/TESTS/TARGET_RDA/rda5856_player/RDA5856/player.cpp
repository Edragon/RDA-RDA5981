#include "player.h"
#include "rda58xx.h"
#include "SDMMCFileSystem.h"
#include "mp4ff.h"
#include "rda_mp4.h"

//#define BUFFER_2K

SDMMCFileSystem sd(PB_9, PB_0, PB_3, PB_7, PC_0, PC_1, "sd"); //SD Card pins
rda58xx rda5856(PB_2, PB_1, PB_8);//UART pins: tx, rx, xreset //RDA5991H_HDK_V1.0,5981A/C_HDK_V1.1
//rda58xx rda5856(PB_2, PB_1, PC_6);//UART pins: tx, rx, xreset //RDA5991H_HDK_V1.2
//rda58xx rda5856(PD_3, PD_2, PB_8);//UART pins: tx, rx, xreset //RDA5956
//rda58xx rda5856(PB_2, PB_1, PC_9);//UART pins: tx, rx, xreset //HanFeng

playerStatetype  playerState;
ctrlStatetype ctrlState;
#ifdef BUFFER_2K
#define BUFF_SIZE 2048
#else
#define BUFF_SIZE 1024
#endif
static uint8_t fileBuf[BUFF_SIZE];
uint8_t *bufptr;

char list[15][32];            //song list
uint8_t index_current = 0;       //song play index
uint8_t index_MAX;               //how many song in all
//uint8_t vlume = 7;   //vlume
//uint8_t vlumeflag = 0;  //set vlume flag
volumeStatetype volumeState = VOL_NONE;

uint32_t m4a_read_callback(void *user_data, void *buffer, uint32_t length)
{
    return ((FileHandle *)user_data)->read(buffer, length);
}

uint32_t m4a_seek_callback(void *user_data, uint64_t position)
{
    return ((FileHandle *)user_data)->lseek(position, SEEK_SET);
}

void Player::begin(void)
{
    DirHandle *dir;
    struct dirent *ptr;
    FileHandle *fp;

    dir = opendir("/sd");
    printf("\r\n**********playing list**********\r\n");
    uint8_t i = 0, j = 0;
    while(((ptr = dir->readdir()) != NULL)&&(i <15)) {
        if(strstr(ptr->d_name,".mp3")||strstr(ptr->d_name,".MP3")||strstr(ptr->d_name,".AAC")||strstr(ptr->d_name,".aac")
            ||strstr(ptr->d_name,".WAV")||strstr(ptr->d_name,".wav")||strstr(ptr->d_name,".M4A")||strstr(ptr->d_name,".m4a"))
        {
            fp =sd.open(ptr->d_name, O_RDONLY);
            if(fp != NULL) {
                char *byte = ptr->d_name;
                j=0;
                while(*byte) {
                    list[i][j++]  = *byte++;
                }
                printf("%2d . %s\r\n", i,list[i++]);
                fp->close();
            }
        }
    }
    index_MAX = i-1;
    dir->closedir();
    printf("\r\n");
}

/*  This function plays back an audio file.  */
void Player::playFile(const char *file)
{
    int32_t ID3TagSize = 0;
    uint32_t fileSize = 0;
    int32_t bytes;
    uint16_t bufferSize = 0;
    uint16_t bufferThreshold = 0;
    uint32_t byteRate = 0;
    mci_type_enum audioType;
    rda58xx_at_status ret;
    rda58xx_status status;
    int32_t playRet;

    playerState = PS_PLAY;

    FileHandle *fp =sd.open(file, O_RDONLY);
    if(fp == NULL) {
        printf("Could not open %s\r\n",file);
        if(index_current != index_MAX)
            index_current++;
        else
            index_current = 0;
        return;
    } else {
        printf("Playing %s ...\r\n",file);

        fileSize = fp->flen();
        printf("fileSize:%d\r\n", fileSize);

        if (strstr(file, ".mp3") || strstr(file, ".MP3"))
            audioType = MCI_TYPE_DAF;
        else if (strstr(file, ".aac") || strstr(file, ".AAC"))
            audioType = MCI_TYPE_AAC;
        else if (strstr(file, ".wav") || strstr(file, ".WAV"))
            audioType = MCI_TYPE_WAV;
        else if (strstr(file, ".m4a") || strstr(file, ".M4A"))
            audioType = MCI_TYPE_M4A;
        else {
            printf("Error! Unsupported Format!\r\n");
            fp->close();
            if(index_current != index_MAX)
                index_current++;
            else
                index_current = 0;
            return;
        }

        if (audioType == MCI_TYPE_WAV) {
            bytes = fp->read(fileBuf,44);
            byteRate = (fileBuf[31] << 24) | (fileBuf[30] << 16) | (fileBuf[29] << 8) | fileBuf[28];
            fp->lseek(0, SEEK_SET);
        } else if (audioType == MCI_TYPE_DAF) {
            bytes = fp->read(fileBuf,10);
            if (fileBuf[0] == 0x49 && fileBuf[1] == 0x44 && fileBuf[2] == 0x33) {
                ID3TagSize = (fileBuf[6] << 21) | (fileBuf[7] << 14) | (fileBuf[8] << 7) | (fileBuf[9] << 0);
                printf("ID3TagSize:%d\r\n", ID3TagSize);
                ID3TagSize += 10;
                fp->lseek(ID3TagSize, SEEK_SET);
                printf("ID3 tag skip\r\n");
            } else {
                fp->lseek(0, SEEK_SET);
                printf("No ID3 tag\r\n");
            }
        }

        if ((fileSize - ID3TagSize) > (1024 * 5)) {
            bufferSize = 1024 * 8;
            bufferThreshold = 1024 * 5;
        } else {
            bufferSize = 1024 * 8;
            bufferThreshold = (fileSize - ID3TagSize) / 2;
        }
        
        if (audioType == MCI_TYPE_M4A) {
            ret = rda5856.bufferReq(MCI_TYPE_AAC, bufferSize, bufferThreshold);
            rda5856.atHandler(ret);
            playRet = m4aPlay(fp);
        } else {
            ret = rda5856.bufferReq(audioType, bufferSize, bufferThreshold);
            rda5856.atHandler(ret);
            playRet = commonPlay(fp);
        }

        if (playRet == 0) {//wait for all data in codec's buffer to be played
            ret = rda5856.stopPlay(WITH_ENDING);
            rda5856.atHandler(ret);
            Thread::wait(100);
        
            ret = rda5856.getCodecStatus(&status);
            rda5856.atHandler(ret);
            while (PLAY == status) {
                Thread::wait(100);
                if(playerState != PS_PLAY) {
                    break;
                }
                ret = rda5856.getCodecStatus(&status);
                rda5856.atHandler(ret);
            }
        } else {//stop play immediately, 
            ret = rda5856.stopPlay(WITHOUT_ENDING);
            rda5856.atHandler(ret);
        }
        fp->close();
        printf("[done!]\r\n");

        if (playRet != 1) {//KEY Pre and Next are not pressed down, play next file
            if(index_current != index_MAX)
                index_current++;
            else
                index_current = 0;
        }
    }
}

int32_t Player::commonPlay(FileHandle *fp)
{
    int32_t bytes;        // How many bytes in buffer left
    int32_t n;
    uint32_t frame = 0;
    rda58xx_at_status ret;

    /* Main playback loop */
    while((bytes = fp->read(fileBuf,BUFF_SIZE)) > 0) {
        bufptr = fileBuf;
        switch (volumeState) {
        case VOL_NONE:
            break;
        case VOL_ADD:
            ret = rda5856.volAdd();
            rda5856.atHandler(ret);
            volumeState = VOL_NONE;
            break;
        case VOL_SUB:
            ret = rda5856.volSub();
            rda5856.atHandler(ret);
            volumeState = VOL_NONE;
            break;
        default:
            break;
        }

        // actual audio data gets sent to RDA58XX.
        while(bytes > 0) {
            n = (bytes < BUFF_SIZE) ? bytes : BUFF_SIZE;
            ret = rda5856.sendRawData(bufptr, n);
            rda5856.atHandler(ret);
            bytes -= n;
            bufptr += n;
            frame++;
        }

        if (playerState == PS_PAUSE) {
            ret = rda5856.pause();
            rda5856.atHandler(ret);
            printf("PAUSE\r\n");
        }
            
        while (playerState == PS_PAUSE) {//Pause
            Thread::wait(50);
        }
            
        if (playerState == PS_RESUME) {
            ret = rda5856.resume();
            rda5856.atHandler(ret);
            printf("RESUME\r\n");
            playerState = PS_PLAY;
        }

        if(playerState != PS_PLAY) {       //stop
            printf("Stop Playing\r\n");
            return 1;
        }
    } 

    return 0;
}

int32_t Player::m4aPlay(FileHandle *fp)
{
    mp4ff_t *infile;
    mp4ff_callback_t *mp4cb;
    mp4AudioSpecificConfig mp4ASC;
    uint8_t *buffer;
    uint32_t buffer_size;
    uint32_t sampleId, numSamples;
    int result;
#ifdef ADTS_OUT
    FILE *adtsFile;
#endif
    int32_t bytes;        // How many bytes in buffer left
    int32_t n;
    uint32_t frame = 0;
    rda58xx_at_status ret;
    int32_t errCode = 0;

#ifdef ADTS_OUT
    adtsFile = fopen("/sd/adts.aac", "wb");
#endif
    
    mp4cb = (mp4ff_callback_t *)malloc(sizeof(mp4ff_callback_t));
    if (mp4cb == NULL) {
        printf("Error malloc mp4cb\n");
        errCode = -1;
        goto ERROR;
    }
            
    mp4cb->read = m4a_read_callback;
    mp4cb->seek = m4a_seek_callback;
    mp4cb->user_data = fp;
            
    infile = mp4ff_open_read(mp4cb);
            
    if (infile == NULL) {
        printf("Error init m4a player\n");
        errCode = -1;
        goto ERROR;
    }
            
    int32_t track = rda_mp4ff_get_aac_track(infile);
    if (track < 0) {
        printf("Unable to find correct AAC sound track in the MP4 file.\n");
        errCode = -1;
        goto ERROR;
    }
            
    buffer = NULL;
    buffer_size = 0;
            
    mp4ff_get_decoder_config(infile, track, &buffer, &buffer_size);
            
    if (buffer) {
        rda_AudioSpecificConfig2(buffer, buffer_size, &mp4ASC, 0);
        free(buffer);
    }
    printf("objectTypeIndex=%d,samplingFrequencyIndex=%d,channelsConfiguration=%d,sbr_present_flag=%d\n",\
        mp4ASC.objectTypeIndex, mp4ASC.samplingFrequencyIndex, mp4ASC.channelsConfiguration, mp4ASC.sbr_present_flag);
    numSamples = mp4ff_num_samples(infile, track);
    printf("numSamples:%d\n", numSamples);
            
    sampleId = 0;
            
    result = rda_mp4ff_fill_stsz_table(infile, track, sampleId);
    printf("rda_mp4ff_fill_stsz_table:%d\n", result);
    if (result <= 0) {
        printf("Fill stsz table failed\n");
        errCode = -1;
        goto ERROR;
    }
                            
    do {
        int32_t rc;
        uint32_t dur;
        int32_t offset;
        int32_t size;
                            
        /* get acces unit from MP4 file */
        buffer = NULL;
        buffer_size = 0;
            
        dur = mp4ff_get_sample_duration(infile, track, sampleId);
        rda_mp4ff_get_sample_position_and_size(infile, track, sampleId, &offset, &size);
        printf("ID:%d,offset:%d,size:%d\n", sampleId, offset, size);
        rc = rda_mp4ff_read_sample_adts(infile, track, sampleId, &buffer,  &buffer_size);
        if (rc == 0) {
            printf("Reading from MP4 file failed.\n");
            errCode = -1;
            goto ERROR;
        }
            
        rda_MakeAdtsHeader(buffer, buffer_size, &mp4ASC);
#ifdef ADTS_OUT
        fwrite(buffer, sizeof(unsigned char), buffer_size , adtsFile);
#endif
        bufptr = buffer;
        bytes = buffer_size;

        switch (volumeState) {
        case VOL_NONE:
            break;
        case VOL_ADD:
            ret = rda5856.volAdd();
            rda5856.atHandler(ret);
            volumeState = VOL_NONE;
            break;
        case VOL_SUB:
            ret = rda5856.volSub();
            rda5856.atHandler(ret);
            volumeState = VOL_NONE;
            break;
        default:
            break;
        }

        // actual audio data gets sent to RDA58XX.
        while(bytes > 0) {
            n = (bytes < BUFF_SIZE) ? bytes : BUFF_SIZE;
            ret = rda5856.sendRawData(bufptr, n);
            rda5856.atHandler(ret);
            bytes -= n;
            bufptr += n;
            frame++;
        }
        
        if (playerState == PS_PAUSE) {
            ret = rda5856.pause();
            rda5856.atHandler(ret);
            printf("PAUSE\r\n");
        }
            
        while (playerState == PS_PAUSE) {//Pause
            Thread::wait(50);
        }
            
        if (playerState == PS_RESUME) {
            ret = rda5856.resume();
            rda5856.atHandler(ret);
            printf("RESUME\r\n");
            playerState = PS_PLAY;
        }
            
        if(playerState != PS_PLAY) {       //stop
            printf("Stop Playing\r\n");
            errCode = 1;
            goto ERROR;
        }
            
        sampleId++;
        printf("sampleId:%d\n", sampleId);
        result = rda_mp4ff_refill_stsz_table(infile, track, sampleId, numSamples);
            
        if (result <= 0) {
            printf("Refill stsz table failed\n");
            errCode = -1;
            goto ERROR;
        }
            
        if (buffer) {
            free(buffer);
            buffer = NULL;
        }
                            
    } while (sampleId < numSamples);
    errCode = 0;
ERROR:
#ifdef ADTS_OUT
    if (adtsFile)
        fclose(adtsFile);
#endif
    if (buffer) {
        free(buffer);
        buffer = NULL;
    }
    if (infile)
        mp4ff_close(infile);
    if (mp4cb) {
        free(mp4cb);
        mp4cb = NULL;
    }
    return errCode;
}

/* PCM file Header */
uint8_t pcmHeader[44] = {
    'R', 'I', 'F', 'F',
    0xFF, 0xFF, 0xFF, 0xFF,
    'W', 'A', 'V', 'E',
    'f', 'm', 't', ' ',
    0x10, 0, 0, 0,          /* 16 */
    0x1, 0,                 /* PCM */
    0x1, 0,                 /* chan */
    0x40, 0x1F, 0x0, 0x0,     /* sampleRate */
    0x80, 0x3E, 0x0, 0x0,     /* byteRate */
    2, 0,                   /* blockAlign */
    0x10, 0,                /* bitsPerSample */
    'd', 'a', 't', 'a',
    0xFF, 0xFF, 0xFF, 0xFF
};

void Set32(uint8_t *d, uint32_t n)
{
    uint32_t i;
    for (i=0; i<4; i++) {
        *d++ = (uint8_t)n;
        n >>= 8;
    }
}

/*  This function records an audio file in WAV formats.Use LINK1,left channel  */
void Player::recordFile(const char *file)
{
    uint32_t fileSize = 0;
    uint32_t n = 0;
    rda58xx_at_status ret;

    ret = rda5856.stopPlay(WITHOUT_ENDING);
    rda5856.atHandler(ret);
    sd.remove(file);
    FileHandle *fp =sd.open(file, O_WRONLY|O_CREAT);
    if(fp == NULL) {
        printf("Could not open file for write\r\n");
    } else {
        fp->write(pcmHeader, sizeof(pcmHeader));
        //rda5856.setMicGain(7);
        ret = rda5856.startRecord(MCI_TYPE_WAV_DVI_ADPCM, rda5856.getBufferSize() * 2 / 4);
        rda5856.atHandler(ret);
        rda5856.setStatus(RECORDING);
        printf("recording......\r\n");

        while (playerState == PS_RECORDING) {
            if (FULL == rda5856.getBufferStatus()) {
                rda5856.clearBufferStatus();
                fp->write(rda5856.getBufferAddr(), rda5856.getBufferSize());
                fileSize += rda5856.getBufferSize();
                n++;
                //printf("REC:%d\n", n);
            }
        }
        ret = rda5856.stopRecord();
        rda5856.atHandler(ret);
        rda5856.clearBufferStatus();

        /* Update file sizes for an RIFF PCM .WAV file */
        fp->lseek(0, SEEK_SET);
        Set32(pcmHeader + 4, fileSize + 36);
        Set32(pcmHeader + 40, fileSize);
        fp->write(pcmHeader, sizeof(pcmHeader));
        fp->close();

        printf("recording end\r\n");
    }
}

/*  This function checks if the codec is ready  */
bool Player::isReady(void)
{
    return rda5856.isReady();
}

void Player::setUartMode(void)
{
    rda58xx_at_status ret;
    ret = rda5856.setMode(UART_MODE);
    rda5856.atHandler(ret);
}

void Player::setBtMode(void)
{
    rda58xx_at_status ret;
    ret = rda5856.setMode(BT_MODE);
    rda5856.atHandler(ret);
}

rda58xx_mode Player::getMode(void)
{
    return rda5856.getMode();
}

int8_t Player::getBtRssi(void)
{
    rda58xx_at_status ret;
    int8_t RSSI;
    
    ret = rda5856.getBtRssi(&RSSI);
    rda5856.atHandler(ret);
    
    return RSSI;
}

