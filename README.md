# at-command-interface
Simple library to communicate from an arduino to an esp8266 wifi module with AT command firmware.

To get started create a software serial on the arduino RX and TX ports connecting to the esp's TX and RX ports respectively
The esp comes stock at a 115200 baud rate which is too high for the software serial. Use the `AT+UART_DEF` cmd to change the default
baud rate to something lower like 9600. DO NOT USE `AT+IPR`, it will work at first but may brick your wifi module.

Ex: `AT+UART_DEF=9600,8,1,0,0` I have bricked my esp many times trying to change the baud rate so be careful.

Once the softwareSerial is set up pass its address to the constructor of an instance of the `ESPWifi` class.
To test that everything is working run the `testDevice()` function which will return true if it receives an `OK` from the module


See the header file for function usage information

### Examples
##### Client

```C++
ESPWifi wifi(&softwareSerial);
if(!wifi.joinAP(ssid, psswd)) Serial.println("Failed to join AP");
wifi.setIPProto(TCP);
//wifi.setIPProto(UDP);
if(!wifi.connectToServer(ip, port)) Serial.println("Failed to connect");
if(!wifi.send("HELLO WORLD")) Serial.println("Failed to send");
char * response;
wifi.recv(&response);
Serial.println(response);
delete[] response
wifi.disconnect();
wifi.quitAP();
```

##### Server

```C++
ESPWifi wifi(&softwareSerial);
client clients[MAX_CLIENTS];
void setup() {
    if(!wifi.joinAP(ssid, psswd)) Serial.println("Failed to join AP");
    wifi.setIPProto(TCP);
    wifi.passthroughmode(false);
    wifi.multipleConnections(true);
    if(!wifi.tcpServer(true, 8000)) Serial.println("Could not create server");
}
void loop() {
    if(softwareSerial.available()){
        wifi.handleConnections(clients);
        char * msg = nullptr;
        unsigned long sender = 0;
        for(unsigned long i = 0; i < MAX_CLIENTS; ++i){
            if(clients[i].hasMessage){
                wifi.getPendingMsg(&msg, clients[i]);
                Serial.print(i); Serial.print(": "); Serial.println(msg);
                sender = i;
                break;
            }
        }
        if(msg != nullptr) {
            for(unsigned long i = 0; i < MAX_CLIENTS; ++i){
                if(sender != i && clients[i].connected)
                    if(!wifi.send(msg, clients[i].channel)) Serial.println("Failed to send");
            }
            delete[] msg;
        }
    }
}
```

##### AP

```C++
ESPWifi wifi(&softwareSerial);
wifi.setWifiMode(wifiMode::ap);
if(!wifi.startAP("NotSketchyWifi", "letmein1", wifiEnc::wpa2)) Serial.println("Could not start ap");
//Some code
connection * connections;
unsigned long num;
wifi.getConnectedClients(&connections, num);
for(unsigned long i = 0; i < num; ++i){
    Serial.print(connections[i].ip); Serial.print(" -- "); Serial.println(connections[i].mac);
}
delete[] connections;
```
##### ESPString
I added a string class in order to avoid you having to do memory management
It interfaces nicely with the exisiting functions with an implicit cast to `char *` and an explicit cast to `char **` used to set the data of the string. ESPString follows the same semantics as an std::string

```C++
ESPString str;
ESPString msg = "Hello";
msg += " World"; //string concatonation
wifi.send(msg);
wifi.recv((char**)str); //explicit cast overwrites previous data, meant to be passed to an output parameter
Serial.println(str.c_str()); //const char * conversion function
Serial.println(msg[0]); //get char in string
Serial.println(msg.length()); //get length of string
```
