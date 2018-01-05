#include "mbed.h"
#include "rtos.h"

//#include "SDMMCFileSystem.h"
#include "WiFiStackInterface.h"
#include "NetworkStack.h"
//#include "EthernetInterface.h"
#include "TCPSocket.h"
#include "console.h"

Timer tmr;

#if 1
namespace {

const char *HTTPS_SERVER_NAME = "10.102.24.44";
const char *HTTPS_SERVER_ADDR = "10.102.24.44";
//const char *HTTPS_SERVER_NAME = "mirrors.sohu.com";
//const char *HTTPS_SERVER_ADDR = "119.188.36.70";
const char *Referer = "http://10.102.24.45/WiFi/USB3.0/";
const char *UserAgent = "Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/50.0.2661.94 Safari/537.36";
const char *Encoding = "gzip, deflate, sdch";
const char *Language = "zh-CN,zh;q=0.8";

const int HTTPS_SERVER_PORT = 80;
const int SEND_BUFFER_SIZE = 500;
const int RECV_BUFFER_SIZE = 1600;
char RECV[RECV_BUFFER_SIZE];
const char HTTPS_PATH[] = "/WiFi/USB3.0/USB_SEND.tar.gz";
//const char HTTPS_PATH[] = "/ubuntu/ubuntu/ls-lR.gz";
const size_t HTTPS_PATH_LEN = sizeof(HTTPS_PATH) - 1;

/* Test related data */
const char *HTTPS_OK_STR = "200 OK";

}

/**
 * \brief HelloHTTPS implements the logic for fetching a file from a webserver
 * using a TCP socket and parsing the result.
 */
class HelloHTTPS {
public:
    /**
     * HelloHTTPS Constructor
     * Initializes the TCP socket, sets up event handlers and flags.
     *
     * @param[in] domain The domain name to fetch from
     * @param[in] port The port of the HTTPS server
     */
    HelloHTTPS(const char * domain, const uint16_t port, NetworkInterface *net_iface) :
            _domain(domain), _port(port)
    {
        _tcpsocket = new TCPSocket(net_iface);
    }
    /**
     * HelloHTTPS Desctructor
     */
    ~HelloHTTPS() {
    }
    /**
     * Start the test.
     *
     * Starts by clearing test flags, then resolves the address with DNS.
     *
     * @param[in] path The path of the file to fetch from the HTTPS server
     * @return SOCKET_ERROR_NONE on success, or an error code on failure
     */
    void startTest(const char *path) {
        int ret = 0;
        /* Fill the request buffer */
        _bpos = snprintf(_buffer, sizeof(_buffer) - 1, "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: keep-alive\r\n Referer: %s\r\nUser-Agent: %s\r\nAccept-Encoding: %s\r\nAccept-Language: %s\r\n\r\n\n", path, HTTPS_SERVER_NAME, Referer, UserAgent, Encoding, Language);

        /* Connect to the server */
        printf("Connecting with %s, _bpos = %d\r\n", _domain, _bpos);
        
        SocketAddress address(HTTPS_SERVER_ADDR, HTTPS_SERVER_PORT);
        if (!address) {
            printf("address error\r\n");
            return;
        }

        //_tcpsocket->set_timeout(1000);
        ret = (int)_tcpsocket->connect(address);
        if(ret != 0)
            printf("connect err = %d\r\n", ret);

        _size = _tcpsocket->send(_buffer, strlen(_buffer) - 1);
        printf("send_size = %d\r\n", _size);
        
        _bpos = 0;
        
        memset(RECV, 0, RECV_BUFFER_SIZE);
        //time_t seconds1 = time(NULL);
        _stime = tmr.read_ms();
        _total = 0;
        do {
            //printf("start recv~~~~~\r\n");
            _size = _tcpsocket->recv((void *)RECV, RECV_BUFFER_SIZE);
            if(_size <= 0){
                printf("size = %d\r\n", _size);
                break;
            }
            _total += _size;
            _etime = tmr.read_ms();
            _delay = _etime - _stime;
            if(_delay > 1000)
            {
                printf("speed %0.5f KBs\r\n", _total*1000.0/1024/_delay);
                _total = 0;
                _stime = _etime;
            }
         }while(_size > 0);
        _tcpsocket->close();
        printf("recv over\r\n");
    }
    
    void close() {
        _tcpsocket->close();
        //while (!_disconnected)
        //    __WFI();
    }
    
protected:
    TCPSocket* _tcpsocket;

    const char *_domain;            /**< The domain name of the HTTPS server */
    const uint16_t _port;           /**< The HTTPS server port */
    char _buffer[SEND_BUFFER_SIZE]; /**< The response buffer */
    size_t _bpos;                   /**< The current offset in the response buffer */
    int _size;                      /**< The size of send or recv */
    int _fpsize;
    int _total;
    int _stime, _etime, _delay;
};
#endif

int main() {
    WiFiStackInterface wifi;

    printf("hello test\r\n");

    tmr.start();
    console_init();

    const char *ip_addr;
    wifi.connect("m", "qqqqqqqq", NSAPI_SECURITY_NONE);
    while(!wifi.get_ip_address());
    ip_addr = wifi.get_ip_address();
    if (ip_addr) {
        printf("Client IP Address is %s\r\n", ip_addr);
    } else {
        printf("No Client IP Address\r\n");
    }

    HelloHTTPS hello(HTTPS_SERVER_ADDR, HTTPS_SERVER_PORT, &wifi);
    hello.startTest(HTTPS_PATH);

    while (true) {
        ;
    }
}
