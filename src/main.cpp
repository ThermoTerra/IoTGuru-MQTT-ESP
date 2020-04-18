#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
 
const char* ssid = "Tronet"; // Enter your WiFi name
const char* password =  "Shira2306"; // Enter WiFi password
const char* mqttServer = "mqtt.iotguru.cloud";
const int mqttPort = 1883;
const char* mqttUser = "g46NIng-txfaUvdQZj0R6g";
const char* mqttPassword = "s_QOxnkl-bXRZnkJnOdDrQ";
const char* clientID = "klrv5wfaa6t82ClQfcIR6g";
const char* deviceID = mqttUser;//"g46NIng-txdn60mgZj4R6g"
 
// used by serial read
const byte numChars = 50;
char receivedChars[numChars];
boolean newData = false;
#define TUPLE_MAX 3
// more efficienty to allocate this way
char tuple_values[TUPLE_MAX][numChars];
const char startMarker = '< ';
const char endMarker = '>';
const char seperator = ',';


void reciveTuple();
void showNewData();
void callback(char* topic, byte* payload, unsigned int length);

WiFiClient espClient;
PubSubClient client(espClient);
 
void setup() {
 
  Serial.begin(115200);
 
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");
 
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
 
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
 
    if (client.connect(clientID, mqttUser, mqttPassword )) {
 
      Serial.println("connected");  
 
    } else {
 
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
 
    }
  }
 
  client.publish("pub/g46NIng-txfaUvdQZj0R6g/klrv5wfaa6t82ClQfcIR6g/jWcrox0QP1arY-cAfcIR6g/voc", "3.14"); //Topic name
  //client.subscribe("esp/test");
 
}
 
void callback(char* topic, byte* payload, unsigned int length) {
 
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
 
  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
 
  Serial.println();
  Serial.println("-----------------------");
 
}
 
void loop() {
  reciveTuple();
  client.loop();
}



void reciveTuple() {
/* read from serial a string composed as a tuple: <a1,a2,a3,...an> 
Insert every element to an element in a string array; */
    static boolean recvInProgress = false;
    static byte ndx = 0;

    char rc;
    short position;
 
 
    while (Serial.available() > 0 && newData == false) {
        rc = Serial.read();
        switch(rc) {
          case seperator:
            // terminate this string
            if (!recvInProgress) break; // expect only start marker initally
            tuple_values[position][ndx] = '\0';
            if (position<TUPLE_MAX-2)
              position=position+1;
            else
              { // if tuple larger then max size, the seperator will act as an end
                Serial.print('tupple exceeded max');
                position=0;
              }
            ndx=0;

              
          break;
          case endMarker:
            if (!recvInProgress) break; // expect only start marker initally
            newData=true;
            // terminate this string
            tuple_values[position][ndx] = '\0';
            position=0;
            ndx=0;

          break;
          case startMarker:
              recvInProgress = true;
          break;
          default:
            if (!recvInProgress) break; // expect only start marker initally
            tuple_values[position][ndx] = rc;
            ndx++;
            if (ndx >= numChars) {
                ndx = numChars - 1;
            }
          }  //switch  
    } //while
}

void showNewData() {
    if (newData == true) {
        Serial.print("This just in:");
        for (int p=0;p<TUPLE_MAX; p++)
        {
          Serial.println(tuple_values[p]);
        }
        
        newData = false;
    }
}
