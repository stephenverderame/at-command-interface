#ifndef AT_DRIVER_H
#define AT_DRIVER_H

#include <SoftwareSerial.h>
//Double Quote
#define DQ 0x22 //"
#define TCP "TCP"
#define UDP "UDP"
enum class wifiMode {
	station = 1,
	ap,
	both
};
enum class wifiEnc {
	open,
	wpa,
	wpa2,
	wpa_wpa2
};
struct connection {
public:
	char * ip;
	char * mac;
	~connection();
	connection();
};
struct client {
	int channel;
	bool connected;
	bool hasMessage;
	char * msgBuffer;
	client();
};
class ESPWifi {
private:
    SoftwareSerial * serial;
	const char * protocol;
	wifiMode mode;
	mutable bool connectedToServer;
private:
	/**
	* @param data, must be null terminated
	*/
    void writeToBoard(const char * data, bool newLine = false) const;
    /**
    * @return null terminated string
    */
    char * readFromBoard() const;
	void safeDelay(unsigned long milli) const;
public:
    ESPWifi(SoftwareSerial * s);
	/**
	* Sends command to esp8266.
	* @return true on success
	* @param outputBuffer, the response. Memory must be freed
	* @param cmd, the command to send
	* @param delay, the time in milliseconds to delay before receiving a response
	* @param timeout, the max time in milliseconds to wait for serial activity for a response
	*/
	bool sendCommand(const char * cmd, char ** outputBuffer, unsigned long delay = 0, unsigned long timeout = 2000) const;
	/**
	* Sends the AT command to the board.
	* @return true on a successful OK response
	*/
	bool testDevice() const;
	bool setWifiMode(wifiMode m);
	bool joinAP(const char * name, const char * psswd) const;
	//Disconnects from AP
	bool quitAP() const;
	void setIPProto(const char * proto);
	bool connectToServer(const char * ip, unsigned short port) const;
	//Disconnects from a server
	bool disconnect() const;


	/**
	* @param msg, null terminated string
	*/
	bool send(const char * msg) const;
	/**
	* @param output, address of output buffer. Must be freed
	* @param timeout, address of timeout in milliseconds.
	*/
	bool recv(char ** output, unsigned long * timeout = nullptr) const;
	bool startAP(const char * ssid, const char * psswd, wifiEnc enc = wifiEnc::open, int channelId = 5);
	//Passthroughmode must be disabled
	bool multipleConnections(bool t) const;
	/**
	* @param create, when true creates server otherwise deletes one
	*/
	bool tcpServer(bool create, unsigned short port);
	char * getIpData();
	bool setMaxConn(int connections);
	bool setTcpTimeout(int timeout);

	/**
	* @param connections, address of the list of clients connected to the ap. Must be freed
	*/
	bool getConnectedClients(connection ** connections, unsigned long & numConns) const;


	
	/**
	* @param clients, an array of clients. Updates the connected field
	* If a message is received, marks the client that sent it as having a pending message to be processed with getPendingMsg()
	* @see getPendingMsg
	*/
	void handleConnections(client * clients);

	/**
	* Non blocking receive. 
	* @param channel, the channel from where the data came from. Output
	*/
	bool recvNonBlock(char ** output, int & channel);
	bool getPendingMsg(char ** output, client c);
	/**
	* Send data on a specefic channel
	* Multiple connections must be enables
	*/
	bool send(const char * msg, int channel) const;

	//Must be false for multi-connection TCP
	bool passthroughmode(bool t);

	/**
	* Should be called after send if you are not expecting a response.
	* If a response is possible use one of the recv functions
	* @param delay, millisecond delay before checking serial
	* @return true on successful send
    */
	bool checkSendCode(unsigned long delay = 0) const;

};
/**
* @param pos, keeps track of lines already read
* @return the next \r\n delim line from the input buffer. Must be freed
*/
char * readline(const char * input, unsigned long & pos);
#endif //AT_DRIVER_H