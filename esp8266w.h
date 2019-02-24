#ifndef AT_DRIVER_H
#define AT_DRIVER_H

#include <SoftwareSerial.h>
//Double Quote
#define DQ 0x22 //" - For internal use

#define TCP "TCP" //For use with setIPProto
#define UDP "UDP" //For use with setIPProto
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
/**
* Stores ip and mac address of each client connected to this ap
* Deconstructor frees the ip and mac, which is null-terminated
* Use strcpy if you need the data to persist 
*/
struct connection {
public:
	char * ip;
	char * mac;
	~connection();
	connection();
};
/**
* Stores tcp server client data
* If you are using handleConnections() and getPendingMsg() you should only read from this struct
* No need to manage the msgBuffer memory because handleConnections() and getPendingMsg() does it for you
* @see handleConnections() or getPendingMsg()
* Refer to the README tcp server example 
*/
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
    * @return null terminated string. Must be freed
    */
    char * readFromBoard() const;
	/**
	* Delays for at least @param milli milliseconds
	*/
	void safeDelay(unsigned long milli) const;
public:
	//All boolean return functions return true on successful response and false otherwise
	
	/**
	* SoftwareSerial should have a lifetime >= to the instance
	* Address of an instance should be passed
	*/
    ESPWifi(SoftwareSerial * s);
	/**
	* Sends command to esp8266.
	* @return true if there is an OK in the response. A few commands do not return OK in a good response and thus this function will return false even though it succeeded
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
	/**
	* Sets current wifi mode of the module
	* This change is NOT saved in flash and will reset on next boot.
	* The module can either be a station, access point or both
	*/
	bool setWifiMode(wifiMode m);
	/**
	* Connects to specefied wifi
	*/
	bool joinAP(const char * name, const char * psswd) const;
	/**
	* Disconnects from connected wifi
	*/
	bool quitAP() const;
	/**
	* Sets the protocol to use.
	* @see connectToServer()
	*/
	void setIPProto(const char * proto);
	/**
	* Connects to server with specefied ip, port and ip protocol
	* You must call setIPProto() before this
	* Use this function even for UDP even though that is a connectionless protocol
	*/
	bool connectToServer(const char * ip, unsigned short port) const;
	/**
	* Disconnects from a server
	*/
	bool disconnect() const;


	/**
	* @param msg, null terminated string
	*/
	bool send(const char * msg) const;
	/**
	* @param output, address of output buffer. Must be freed
	* @param timeout, address of timeout in milliseconds. If nullptr is passed timeout is infinite
	* This is a blocking function
	*/
	bool recv(char ** output, unsigned long * timeout = nullptr) const;
	/**
	* Starts an access point with the set ssid, password and encryption
	* Wifi mode must be either ap or both
	*/
	bool startAP(const char * ssid, const char * psswd, wifiEnc enc = wifiEnc::open, int channelId = 5);
	/**
	* Enable or disable multiple connections
	* Passthroughmode must already be disabled for a TCP server
	*/
	bool multipleConnections(bool t) const;
	/**
	* @param create, when true creates server otherwise deletes one
	*/
	bool tcpServer(bool create, unsigned short port);
	/**
	* @return a string containing the station and access point IP and MAC address. Must be freed
	*/
	char * getIpData();
	/**
	* Sets the max connections. Multiple connections must be enabled
	*/
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
	* Will still work in single connection mode just ignore the channel ouput parameter
	*/
	bool recvNonBlock(char ** output, int & channel);
	/**
	* If a client is marked for sending a message, use this function to get it. 
	* @param output, address of buffer to which the message is saved to. Must be freed
	* @param c, client that just send a message
	* @see handleConnections()
	*/
	bool getPendingMsg(char ** output, client c);
	/**
	* Send data on a specefic channel
	* Multiple connections must be enables
	*/
	bool send(const char * msg, int channel) const;

	//Must be false for multi-connection TCP
	bool passthroughmode(bool t);

	/**
	* Should be called after send() if you are not expecting a response.
	* If a response is possible, use one of the recv() functions
	* @param delay, millisecond delay before checking serial
	* @return true on successful send (Receives a SEND OK)
    */
	bool checkSendCode(unsigned long delay = 0) const;
	
	/**
	* @return true if connected to a server
	*/
	bool isConnectedToServer() const;

};
/**
* @param pos, keeps track of lines already read
* @return the next \r\n delim line from the input buffer. Must be freed
*/
char * readline(const char * input, unsigned long & pos);
#endif //AT_DRIVER_H