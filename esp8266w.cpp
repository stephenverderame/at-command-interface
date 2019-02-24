#include <esp8266w.h>
#include <string.h>
#include <Arduino.h>
ESPWifi::ESPWifi(SoftwareSerial * s) : serial(s), protocol(nullptr), mode(wifiMode::both), connectedToServer(false) {

}
void ESPWifi::writeToBoard(const char * data, bool newLine) const {
    if(newLine)
        serial->println(data);
	else
		serial->print(data);
}
bool ESPWifi::sendCommand(const char * cmd, char ** outputBuffer, unsigned long delay, unsigned long timeout) const
{
	writeToBoard(cmd, true);
	unsigned long start_t = millis();
	if (delay) safeDelay(delay);
	while (!serial->available())
		if (millis() - start_t > timeout) return false;
	*outputBuffer = readFromBoard();
	return strstr(*outputBuffer, "OK");

}
bool ESPWifi::testDevice() const
{
	char * output;
	bool success = sendCommand("AT", &output);
	delete[] output;
	return success;
}
bool ESPWifi::setWifiMode(wifiMode m)
{
	char msg[13];
	sprintf(msg, "AT+CWMODE=%d", (int)m);
	char * output;
	bool s = sendCommand(msg, &output);
	if (s) mode = m;
	delete[] output;
	return s;
}
bool ESPWifi::joinAP(const char * name, const char * psswd) const
{
	unsigned long len = strlen(name) + strlen(psswd) + 16;
	char * msg = new char[len];
	sprintf(msg, "AT+CWJAP=%c%s%c,%c%s%c", DQ, name, DQ, DQ, psswd, DQ);
	char * output;
	bool s = sendCommand(msg, &output, 10000);
	delete[] output;
	return s;
}
bool ESPWifi::quitAP() const
{
	char * output;
	bool s = sendCommand("AT+CWQAP", &output);
	delete[] output;
	return s;
}
void ESPWifi::setIPProto(const char * proto)
{
	protocol = proto;
}
bool ESPWifi::connectToServer(const char * ip, unsigned short port) const
{
	if (protocol == nullptr) return false;
	char cmd[40];
	sprintf(cmd, "AT+CIPSTART=%c%s%c,%c%s%c,%hu", DQ, protocol, DQ, DQ, ip, DQ, port);
	char * output;
	bool s = sendCommand(cmd, &output);
	if (!s && strstr(output, "ALREADY")) s = true;
	delete[] output;
	connectedToServer = s;
	return s;
}
bool ESPWifi::disconnect() const
{
	char * out;
	bool s = sendCommand("AT+CIPCLOSE", &out);
	delete[] out;
	connectedToServer = false;
	return s;
}
bool ESPWifi::send(const char * msg) const
{
	unsigned long len = strlen(msg);
	char cmd[15];
	sprintf(cmd, "AT+CIPSEND=%lu", len);
	char * out;
	bool s = sendCommand(cmd, &out);
	if (s)
		writeToBoard(msg, true);
	delete[] out;
	return s;

}
bool ESPWifi::recv(char ** output, unsigned long * timeout) const
{
	unsigned long start = millis();
	while (!serial->available())
		if (timeout != nullptr && millis() - start > *timeout) return false;
	char * msg = readFromBoard();
	char * ptr = strstr(msg, "+IPD");
	if (ptr == nullptr) {
		delete[] msg;
		return false;
	}
	unsigned long len = (unsigned long)(strchr(ptr, ':') - (ptr + 5));
	char * size = new char[len + 1];
	memcpy(size, ptr + 5, len);
	size[len] = '\0';
	int sizeNum = atoi(size);
	delete[] size;
	*output = new char[sizeNum + 1];
	memcpy(*output, strchr(ptr, ':') + 1, sizeNum);
	*output[sizeNum] = '\0';
	return true;


}
bool ESPWifi::startAP(const char * ssid, const char * psswd, wifiEnc enc, int channelId)
{
	if (mode == wifiMode::station) return false;
	char cmd[64];
	sprintf(cmd, "AT+CWSAP_CUR=%c%s%c,%c%s%c,%d,%d", DQ, ssid, DQ, DQ, psswd, DQ, channelId, (int)enc);
	char * out;
	bool s = sendCommand(cmd, &out);
	delete[] out;
	return s;
}
bool ESPWifi::multipleConnections(bool t) const
{
	char msg[12];
	sprintf(msg, "AT+CIPMUX=%d", (int)t);
	char * out;
	bool s = sendCommand(msg, &out);
	delete[] out;
	return s;
}
bool ESPWifi::tcpServer(bool create, unsigned short port)
{
	char msg[20];
	sprintf(msg, "AT+CIPSERVER=%d,%hu", (int)create, port);
	char * out;
	bool s = sendCommand(msg, &out);
	delete[] out;
	return s;
}
char * ESPWifi::getIpData()
{
	char * out;
	sendCommand("AT+CIFSR", &out, 1000);
	return out;
}
bool ESPWifi::setMaxConn(int connections)
{
	char msg[25];
	sprintf(msg, "AT+CIPSERVERMAXCONN=%d", connections);
	char * out;
	bool s = sendCommand(msg, &out);
	delete[] out;
	return s;
}
bool ESPWifi::setTcpTimeout(int timeout)
{
	char msg[15];
	sprintf(msg, "AT+CIPSTO=%d", timeout);
	char * out;
	bool s = sendCommand(msg, &out);
	delete[] out;
	return s;
}
bool ESPWifi::getConnectedClients(connection ** connections, unsigned long & numConns) const
{
	if (mode == wifiMode::ap || mode == wifiMode::both) {
		char * out;
		bool s = sendCommand("AT+CWLIF", &out, 1000);
		if (!s) {
			delete[] out;
			return false;
		}
		unsigned long i = 0;
		char * line = readline(out, i);
		numConns = 0;
		unsigned long maxconns = 10;
		*connections = new connection[10];
		while (line != nullptr) {
			if (strchr(line, ',')) {
				unsigned long ipSize = (unsigned long)(strchr(line, ',') - line);
				(*connections)[numConns].ip = new char[ipSize + 1];
				memcpy((*connections)[numConns].ip, line, ipSize);
				(*connections)[numConns].ip[ipSize] = '\0';
				unsigned long macSize = (unsigned long)(strlen(line) - (strchr(line, ',') + 1 - line));
				(*connections)[numConns].mac = new char[macSize + 1];
				memcpy((*connections)[numConns].mac, strchr(line, ',') + 1, macSize);
				(*connections)[numConns].mac[macSize] = '\0';
				if (numConns++ >= maxconns) {
					connection * ptr = new connection[maxconns * 2];
					memcpy(ptr, *connections, sizeof(connection) * maxconns);
					maxconns *= 2;
					delete[] (*connections);
					*connections = ptr;
				}
			}
			delete[] line;
			line = readline(out, i);
		}
		return true;
	}
	return false;
}
char * ESPWifi::readFromBoard() const {
    char * msg = new char[65];
    unsigned long size = 0;
    unsigned long maxSize = 64;
    while(serial->available()){
        char letter = serial->read();
        if(size >= maxSize){
            char * cpy = new char[maxSize];
            memcpy(cpy, msg, maxSize);
            msg = new char[maxSize * 2 + 1];
            memcpy(msg, cpy, maxSize);
            maxSize *= 2;
            delete[] cpy;
        }
        msg[size++] = letter;
		safeDelay(10);
    }
    msg[size] = '\0';
    return msg;
}

void ESPWifi::safeDelay(unsigned long milli) const
{
	unsigned long start = millis();
	while (millis() < start + milli);
}

char * readline(const char * input, unsigned long & pos)
{
	char * eol = strstr(input + pos, "\r\n");
	if (eol) {
		unsigned long len = eol - (input + pos);
		char * out = new char[len + 1];
		memcpy(out, input + pos, len);
		out[len] = '\0';
		pos = (eol + 2) - input;
		return out;
	}
	return nullptr;
}
void ESPWifi::handleConnections(client * clients) {
	char * msg = readFromBoard();
	if (strstr(msg, "FAIL")) { //disconnect
		unsigned long len = (unsigned long)(strchr(msg, ',') - msg);
		char * ch1 = new char[len + 1];
		memcpy(ch1, msg, len);
		ch1[len] = '\0';
		int channel = atoi(ch1);
		clients[channel].channel = channel;
		clients[channel].connected = false;
		delete[] ch1;
	}
	else if (strstr(msg, "CONNECT")) {
		unsigned long len = (unsigned long)(strchr(msg, ',') - msg);
		char * ch1 = new char[len + 1];
		memcpy(ch1, msg, len);
		ch1[len] = '\0';
		int channel = atoi(ch1);
		clients[channel].channel = channel;
		clients[channel].connected = true;
		delete[] ch1;
	}
	if (strstr(msg, "+IPD")) {
		char * c = strstr(msg, "+IPD,") + 5;
		unsigned long channelLen = (unsigned long)(strrchr(c, ',') - c);
		char * ch = new char[channelLen + 1];
		memcpy(ch, c, channelLen);
		ch[channelLen] = '\0';
		int channel = atoi(ch);
		delete[] ch;
		clients[channel].hasMessage = true;
		clients[channel].msgBuffer = msg;
		return;

	}
	delete[] msg;
}

bool ESPWifi::recvNonBlock(char ** output, int & channel)
{
	char * msg = readFromBoard();
	if (strstr(msg, "+IPD")) {
		if(strchr(msg, ',') != strrchr(msg, ',')) { //if multiple connection mode
			char * c = strstr(msg, "+IPD,") + 5;
			unsigned long channelLen = (unsigned long)(strrchr(c, ',') - c);
			char * ch = new char[channelLen + 1];
			memcpy(ch, c, channelLen);
			ch[channelLen] = '\0';
			channel = atoi(ch);
			delete[] ch;
		}

		char * s = strrchr(msg, ',') + 1;
		unsigned long sizeLen = (unsigned long)(strchr(s, ':') - s);
		char * si = new char[sizeLen + 1];
		memcpy(si, s, sizeLen);
		si[sizeLen] = '\0';
		int size = atoi(si);
		delete[] si;

		*output = new char[size + 1];
		memcpy(*output, strchr(msg, ':') + 1, size);
		(*output)[size] = '\0';
		delete[] msg;
		return true;

	}
	delete[] msg;
	return false;
}

bool ESPWifi::getPendingMsg(char ** output, client c)
{
	if (!c.hasMessage) return false;
	char * s = strrchr(c.msgBuffer, ',') + 1;
	unsigned long sizeLen = (unsigned long)(strchr(s, ':') - s);
	char * si = new char[sizeLen + 1];
	memcpy(si, s, sizeLen);
	si[sizeLen] = '\0';
	int size = atoi(si);
	delete[] si;

	*output = new char[size + 1];
	memcpy(*output, strchr(c.msgBuffer, ':') + 1, size);
	(*output)[size] = '\0';
	delete[] c.msgBuffer;
	c.hasMessage = false;
	return true;
}

bool ESPWifi::send(const char * msg, int channel) const
{
	char cmd[20];
	sprintf(cmd, "AT+CIPSEND=%d,%lu", channel, strlen(msg));
	char * out;
	bool s = sendCommand(cmd, &out);
	if (s)
		writeToBoard(msg, true);
	delete[] out;
	return s;
}

bool ESPWifi::passthroughmode(bool t)
{
	char cmd[20];
	sprintf(cmd, "AT+CIPMODE=%d", (int)t);
	char * out;
	bool s = sendCommand(cmd, &out);
	delete[] out;
	return s;
}

bool ESPWifi::checkSendCode(unsigned long delay) const
{
	if (delay) safeDelay(delay);
	char * out = readFromBoard();
	bool s = strstr(out, "SEND OK");
	delete[] out;
	return s;
}

bool ESPWifi::isConnectedToServer() const
{
	return connectedToServer;
}

connection::~connection()
{
	if (ip != nullptr) delete[] ip;
	if (mac != nullptr) delete[] mac;
}

connection::connection() : ip(nullptr), mac(nullptr)
{
}

client::client() : connected(false), hasMessage(false)
{
}
