//************************************************************
// Code modified from basic.ino from painlessMesh library
//   and ESP32 code from esp32io.com; https://esp32io.com/tutorials/esp32-rfid-nfc
//   and mqtt code: https://diyi0t.com/microcontroller-to-raspberry-pi-wifi-mqtt-communication/
//       http://www.steves-internet-guide.com/using-arduino-pubsub-mqtt-client/
//************************************************************

#include <WiFi.h>
#include <WiFiClient.h>
#include <SPI.h>
#include <MFRC522.h> // https://github.com/miguelbalboa/rfid
#include <painlessMesh.h> // https://gitlab.com/painlessMesh/painlessMesh
#include <PubSubClient.h> // https://github.com/knolleary/pubsubclient
// MQTT : https://diyi0t.com/microcontroller-to-raspberry-pi-wifi-mqtt-communication/
// TASKS : https://github.com/arkhipenko/TaskScheduler/wiki/Full-Document
// MOSQUITTO (used on RPi) : https://github.com/eclipse/paho.mqtt.python

#define SS_PIN  5  // ESP32 pin GIOP5 
#define RST_PIN 27 // ESP32 pin GIOP27 
MFRC522 rfid(SS_PIN, RST_PIN);

#define   MESH_PREFIX     "iotMesh"
#define   MESH_PASSWORD   "iotMeshPassword"
#define   MESH_PORT       5555


// to store subscribe responses
char* responsesStr[] = {""};

const char* ssid = "25-103";
const char* password = "5s94dxae";
//const char* mqtt_server = "broker.mqtt-dashboard.com";
const char* mqtt_server = "10.20.126.8"; // RPi's ip address which runs mosquitto broker; port 1883
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

Scheduler userScheduler; // to control your personal task
painlessMesh  mesh;

// User stub
void sendMessage() ; // Prototype so PlatformIO doesn't complain

Task taskSendMessage( TASK_SECOND * 1 , TASK_FOREVER, &sendMessage );

void sendMessage() {
  String msg = "Hello from node ";
  msg += mesh.getNodeId();
  mesh.sendBroadcast( msg );
  taskSendMessage.setInterval( random( TASK_SECOND * 1, TASK_SECOND * 5 ));
}

Task readRFIDTask( TASK_MILLISECOND * 100 , TASK_FOREVER, &readRFID);
//Task readRFIDTask( TASK_MILLISECOND * 1000 , TASK_FOREVER, &readRFID);


// Needed for painless library
void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
}

void newConnectionCallback(uint32_t nodeId) {
    Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset) {
    Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(),offset);
}

void readRFID(){
  //Serial.print("start of read rfid");
    // modified, rfid
    //Serial.println("loop begin");
  if (rfid.PICC_IsNewCardPresent()) { // new tag is available
  //Serial.print("card detected");
    //Serial.println(responsesStr[-1]);
    if (rfid.PICC_ReadCardSerial()) { // NUID has been read
      //delay(100);
      MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
      Serial.print("RFID/NFC Tag Type: ");
      Serial.println(rfid.PICC_GetTypeName(piccType));

      // print UID in Serial Monitor in the hex format
      Serial.print("UID:");
      String uuidString = "";
      for (int i = 0; i < rfid.uid.size; i++) {
        //Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
        uuidString += rfid.uid.uidByte[i] < 0x10 ? " 0" : " ";
        //Serial.print(rfid.uid.uidByte[i], HEX);
        uuidString += String(rfid.uid.uidByte[i],HEX);
      }
      uuidString.toUpperCase();
      
      Serial.println(uuidString);
      char uuidChar[uuidString.length()+1];
      uint uuidLen = uuidString.length();
      uuidString.toCharArray(uuidChar,uuidLen+1);
     
      Serial.println();      
      //Serial.print("  Subscribed to: ");
      char* outTopic = "uuidReply";
      //Serial.print(outTopic);
      // setting qos to 1, hopefully this makes reliability better
      client.subscribe(outTopic,1);
      sendMQTTFn("uuid", (char*) uuidChar);
      
      rfid.PICC_HaltA(); // halt PICC
      rfid.PCD_StopCrypto1(); // stop encryption on PCD
    }
  }
}

void setup_wifi() {
  // mqtt
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  randomSeed(micros());
  
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  // mqtt
  Serial.print("  Subscription message [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  responsesStr[-1] = topic;
}

void reconnect() {
  // mqtt
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP32Client-";
    clientId += String(WiFi.macAddress());
    // Attempt to connect
    byte willQoS=1;
    const char* willTopic = "willTopic";
    const char* willMessage = "my will message";
    // may want to disable this
    boolean willRetain = true;
    if (client.connect(clientId.c_str(),willTopic,willQoS,willRetain,willMessage)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("log", "reconnected");
      // ... and resubscribe
      client.subscribe("toESPDebug");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void sendMQTTFn(char* pubTopic, char* pubMessage){
  
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  //snprintf (msg, MSG_BUFFER_SIZE, "hello world #%ld", value);
  Serial.print("  Publish message [");
  Serial.print(pubTopic);
  Serial.print("] ");
  Serial.println(pubMessage);
  client.publish(pubTopic, pubMessage);
  // qos 2, may need to make 1 and implement more algorithms  
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  //Serial.println("RFID setup: ");
  SPI.begin(); // init SPI bus
  rfid.PCD_Init(); // init MFRC522
  delay(500);
  Serial.println("Tap an RFID/NFC tag on the RFID-RC522 reader");
    
  // mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  // //mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages
  // mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  // // set mesh parameters
  // mesh.onReceive(&receivedCallback);
  // mesh.onNewConnection(&newConnectionCallback);
  // mesh.onChangedConnections(&changedConnectionCallback);
  // mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  //userScheduler.addTask( taskSendMessage );
  //taskSendMessage.enable();
  //readRFID();
  // userScheduler.addTask( mainCodeRun );
  // mainCodeRun.enable();
  //Serial.print("starting scheduler");
  //userScheduler.addTask( readRFIDTask );
  //readRFIDTask.enable();
  // userScheduler.addTask(sendMQTT);
  // sendMQTT.enable();
  //Serial.print("End of setup");
}

void loop() {
  //userScheduler.execute();
  // it will run the user scheduler as well
  //mesh.update();
  
  readRFID();
  client.loop();
  
}
