//************************************************************
// this is a simple example that uses the painlessMesh library to
// connect to a another network and relay messages from a MQTT broker to the nodes of the mesh network.
// To send a message to a mesh node, you can publish it to "painlessMesh/to/12345678" where 12345678 equals the nodeId.
// To broadcast a message to all nodes in the mesh you can publish it to "painlessMesh/to/broadcast".
// When you publish "getNodes" to "painlessMesh/to/gateway" you receive the mesh topology as JSON
// Every message from the mesh which is send to the gateway node will be published to "painlessMesh/from/12345678" where 12345678 
// is the nodeId from which the packet was send.
//************************************************************

#include <Arduino.h>
#include <painlessMesh.h> // https://gitlab.com/painlessMesh/painlessMesh
#include <PubSubClient.h> // https://github.com/knolleary/pubsubclient
#include <WiFiClient.h>
#include <SPI.h>
#include <MFRC522.h> // https://github.com/miguelbalboa/rfid

// MQTT : https://diyi0t.com/microcontroller-to-raspberry-pi-wifi-mqtt-communication/
// TASKS : https://github.com/arkhipenko/TaskScheduler/wiki/Full-Document
// MOSQUITTO (used on RPi) : https://github.com/eclipse/paho.mqtt.python

#define ONBOARD_LED 2
#define SS_PIN  5  // ESP32 pin GIOP5 
#define RST_PIN 27 // ESP32 pin GIOP27 
MFRC522 rfid(SS_PIN, RST_PIN);

#define   MESH_PREFIX     "meshNetwork"
#define   MESH_PASSWORD   "password"
#define   MESH_PORT       5555

#define   STATION_SSID     "25-103"
#define   STATION_PASSWORD "5s94dxae"

#define HOSTNAME "MQTT_Bridge"

// Prototypes
void receivedCallback( const uint32_t &from, const String &msg );
void mqttCallback(char* topic, byte* payload, unsigned int length);

IPAddress getlocalIP();

IPAddress myIP(0,0,0,0);
IPAddress mqttBroker(10, 20, 126, 8);

painlessMesh  mesh;
WiFiClient wifiClient;
PubSubClient mqttClient(mqttBroker, 1883, mqttCallback, wifiClient);

String macAddressString = WiFi.macAddress();
String pubTopicString = "to/broker/" + macAddressString;
String subTopicString = "from/broker/" + macAddressString;

char pubTopic[100];
char subTopic[100];

void readRFID(){
  //Serial.print("inside read rfid");
  if (rfid.PICC_IsNewCardPresent()) { // new tag is available
    if (rfid.PICC_ReadCardSerial()) { // NUID has been read
        MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
        Serial.print("RFID/NFC Tag Type: ");
        Serial.println(rfid.PICC_GetTypeName(piccType));

        // print UID in Serial Monitor in the hex format
        Serial.print("UID:");
        String uuidString = "";
        for (int i = 0; i < rfid.uid.size; i++) {
        uuidString += rfid.uid.uidByte[i] < 0x10 ? " 0" : " ";
        uuidString += String(rfid.uid.uidByte[i],HEX);
        }
        uuidString.toUpperCase();
        Serial.println(uuidString);
        char uuidChar[uuidString.length()+1];
        uint uuidLen = uuidString.length();
        uuidString.toCharArray(uuidChar,uuidLen+1);
        Serial.println();
        
        sendMQTTFn(pubTopic, uuidChar);

        rfid.PICC_HaltA(); // halt PICC
        rfid.PCD_StopCrypto1(); // stop encryption on PCD
    }
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      mqttClient.publish("outTopic", "hello world");
      // ... and resubscribe
      mqttClient.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void sendMQTTFn(char* pubTopic, char* pubMessage){
  
  
  if (!mqttClient.connected()) {
    reconnect();
  }
  mqttClient.loop();

  Serial.print("  Publishing to: ");  
  Serial.println(pubTopic);
  Serial.print("  Subscribing to: ");  
  Serial.println(subTopic);

  if(myIP != getlocalIP()){
    myIP = getlocalIP();
    Serial.println("My IP is " + myIP.toString());
    Serial.print("MAC Address: ");
    Serial.println(macAddressString);

    if (mqttClient.connect("painlessMeshClient")) {
      mqttClient.publish("painlessMesh/from/gateway","Ready!");
      mqttClient.subscribe("#");
    } 
  }



  //snprintf (msg, MSG_BUFFER_SIZE, "hello world #%ld", value);
  Serial.print("  Publish message [");
  Serial.print(pubTopic);
  Serial.print("] ");
  Serial.println(pubMessage);
  mqttClient.publish(pubTopic, pubMessage);
  
  digitalWrite(ONBOARD_LED, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);                       // wait for a second
  digitalWrite(ONBOARD_LED, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);                       // wait for a second

}

void setup() {
    Serial.begin(115200);
    // initialize digital pin LED_BUILTIN as an output.
    pinMode(ONBOARD_LED, OUTPUT);
    // convert topics from strings to char*s
    pubTopicString.toCharArray(pubTopic, pubTopicString.length() + 1 );
    subTopicString.toCharArray(subTopic, subTopicString.length() + 1 );
    
    // setting qos to 1, hopefully this makes reliability better
    //mqttClient.subscribe(subTopic,1);
    //mqttClient.subscribe("#",1);

    // set up rfid reader
    SPI.begin(); // init SPI bus
    rfid.PCD_Init(); // init MFRC522
    Serial.println("Tap an RFID/NFC tag on the RFID-RC522 reader");

    mesh.setDebugMsgTypes( ERROR | STARTUP | CONNECTION );  // set before init() so that you can see startup messages
    // Channel set to 149, (6 default). Make sure to use the same channel for your mesh and for you other
    mesh.init( MESH_PREFIX, MESH_PASSWORD, MESH_PORT, WIFI_AP_STA, 149 );
    mesh.onReceive(&receivedCallback);
    mesh.stationManual(STATION_SSID, STATION_PASSWORD);
    mesh.setHostname(HOSTNAME);
    // Bridge node, should (in most cases) be a root node. See [the wiki](https://gitlab.com/painlessMesh/painlessMesh/wikis/Possible-challenges-in-mesh-formation) for some background
    mesh.setRoot(true);
    // This node and all other nodes should ideally know the mesh contains a root, so call this on all nodes
    mesh.setContainsRoot(true);

}



void loop() {

  mesh.update();
  mqttClient.loop();
  readRFID();
  
  // if(myIP != getlocalIP()){
  //   myIP = getlocalIP();
  //   Serial.println("My IP is " + myIP.toString());
  //   Serial.print("MAC Address: ");
  //   Serial.println(macAddressString);

  //   if (mqttClient.connect("painlessMeshClient")) {
  //     mqttClient.publish("painlessMesh/from/gateway","Ready!");
  //     mqttClient.subscribe("painlessMesh/to/#");
  //   } 
  
  // }
  
}

void receivedCallback( const uint32_t &from, const String &msg ) {
  // callback for mesh sending messages to mesh nodes
  // not working for some reason 
  Serial.printf("bridge: Received from %u msg=%s\n", from, msg.c_str());
  String topic = "painlessMesh/from/" + String(from);
  mqttClient.publish(topic.c_str(), msg.c_str());
}

void mqttCallback(char* topic, uint8_t* payload, unsigned int length) {
    // callback for mqtt message recieved 
    char* cleanPayload = (char*)malloc(length+1);
    memcpy(cleanPayload, payload, length);
    cleanPayload[length] = '\0';
    String msg = String(cleanPayload);
    free(cleanPayload);

    String targetStr = String(topic).substring(16);
    Serial.println(topic);
    //Serial.println(msg);
    if(topic == subTopic){
        Serial.print(msg);
        if(cleanPayload == "True"){
          Serial.print("opening door!");
        }
    }

    // if(targetStr == "gateway"){
    //   if(msg == "getNodes"){
    //     auto nodes = mesh.getNodeList(true);
    //     String str;
    //     for (auto &&id : nodes)
    //     str += String(id) + String(" ");
    //     mqttClient.publish("painlessMesh/from/gateway", str.c_str());
    //   }
    // } 
    // else if(targetStr == "broadcast"){
    // Serial.println(msg);
    // // will broadcast to itself too 
    // mesh.sendBroadcast(msg,true);
    // }
    // else{
    //     uint32_t target = strtoul(targetStr.c_str(), NULL, 10);
    //     if(mesh.isConnected(target)){
    //         mesh.sendSingle(target, msg);
    //     }
    //     else{
    //         mqttClient.publish("painlessMesh/from/gateway", "Client not connected!");
    //     }
    // }
}

IPAddress getlocalIP() {
  return IPAddress(mesh.getStationIP());
}
