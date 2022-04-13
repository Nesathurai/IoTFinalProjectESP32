// udp client code from lecture
// these used for code reference
// https://techtutorialsx.com/2018/05/17/esp32-arduino-sending-data-with-socket-client/
// http://www.iotsharing.com/2017/06/how-to-use-udpip-with-arduino-esp32.html
// https://github.com/espressif/arduino-esp32/blob/master/libraries/WiFi/examples/WiFiUDPClient/WiFiUDPClient.ino
#include <WiFi.h>
#include <WiFiUdp.h>

const char* ssid = "25-103";
const char* password =  "5s94dxae";

const char * host = "10.20.126.8";
const uint16_t port = 1833;
int worked = 99;
boolean connected = false;

// creating udp
WiFiUDP udp;

void connectToWiFi(const char * ssid, const char * pwd){
  Serial.println("Connecting to WiFi network: " + String(ssid));

  // delete old config
  WiFi.disconnect(true);
  //register event handler
  WiFi.onEvent(WiFiEvent);
  
  //Initiate connection
  WiFi.begin(ssid, pwd);

  Serial.println("Waiting for WIFI connection...");
}


//wifi event handler
void WiFiEvent(WiFiEvent_t event){
    switch(event) {
      case SYSTEM_EVENT_STA_GOT_IP:
          //When connected set 
          Serial.print("WiFi connected! IP address: ");
          Serial.println(WiFi.localIP());  
          //initializes the UDP state
          //This initializes the transfer buffer
          //udp.begin(WiFi.localIP(),port);
          udp.begin('allan2.local',port);
          //worked = udp.begin('10.20.126.8',port);
          Serial.print("worked: ");
          Serial.print(worked);
          Serial.print("|");
          
          connected = true;
          Serial.print("wifi break");
          break;
      case SYSTEM_EVENT_STA_DISCONNECTED:
          Serial.println("WiFi lost connection");
          connected = false;
          break;
        Serial.print("wifi break later");
      default: break;
    }
    Serial.print("wifi end ");
}

void setup()
{
 
  Serial.begin(115200);
  Serial.print("in serial loop");
//   WiFi.begin(ssid, password);
//   while (WiFi.status() != WL_CONNECTED) {
//     delay(500);
//     Serial.println("...");
//   }
    connectToWiFi(ssid, password);
 Serial.print("after wifi connect");
  Serial.print("WiFi connected with IP: ");
  Serial.println(WiFi.localIP());
 
WiFi.setSleep(false);


};
 
void loop()
{
    
    if(connected){
          //data will be sent to server
      uint8_t buffer[50] = "hello world";
      //This initializes udp and transfer buffer
      Serial.print("starting packet");
      udp.beginPacket('10.20.126.8', port);
      udp.write(buffer, 11);
      udp.endPacket();
      memset(buffer, 0, 50);
      //processing incoming packet, must be called before reading the buffer
      udp.parsePacket();
      //receive response from server, it will be HELLO WORLD
      if(udp.read(buffer, 50) > 0){
        Serial.print("Server to client: ");
        Serial.println((char *)buffer);
      }
        // udp.beginPacket(host,port);
        
        // udp.printf("seconds since boot: %lu",millis()/1000);
        // udp.endPacket();
    }
    delay(10000);
};
