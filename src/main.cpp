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
static char MQTT_path[200];
static char MQTT_payload[20];

// used by serial read
const byte numChars = 50;
char receivedChars[numChars];
boolean newData = false;
#define TUPLE_MAX 3
// more efficienty to allocate this way
char tuple_values[TUPLE_MAX][numChars];
const char startMarker = '<';
const char endMarker = '>';
const char seperator = ',';


void reciveTuple();
void recevieTupleAndSend();
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
  client.publish("pub/g46NIng-txfaUvdQZj0R6g/klrv5wfaa6t82ClQfcIR6g/jWcrox0QP1arY-cAfcIR6g/co2","00.00");
  //client.subscribe("esp/test");
  Serial.print("\nPrompt> ");
 
}
 
void callback(char* topic, byte* payload, unsigned int length) {
 
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
 
  Serial.print("Message:");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
 
  Serial.println();
  Serial.println("-----------------------");
 
}
 
void loop() {
  reciveTuple();
  client.loop();
  recevieTupleAndSend();
}



void reciveTuple() {
/* read from serial a string composed as a tuple: <a1,a2,a3,...an> 
Insert every element to an element in a string array; */
    static boolean recvInProgress = false;
    static byte ndx = 0;

    char rc;
    static short position=0;
 
 
    while (Serial.available() > 0 && newData == false) {
        rc = Serial.read();
        Serial.print(rc); // echo back
        switch(rc) {
          case seperator:
            // terminate this string
            if (!recvInProgress) {Serial.print('x'); break; }; // expect only start marker initally
            tuple_values[position][ndx] = '\0';
            if (position<TUPLE_MAX-1)
              {
                position=position+1;
              }
            else
              { // if tuple larger then max size, this will end the tuple
                Serial.print("tupple exceeded max");
                position=0;
                recvInProgress=false;
                newData=true;
              }
            ndx=0;              
            break;
          case endMarker:
            if (!recvInProgress) {Serial.print('!'); break; } // expect only start marker initally
            newData=true;
            // terminate this string
            tuple_values[position][ndx] = '\0';
            recvInProgress=false;
            position=0;
            ndx=0;

          break;
          case startMarker:
              recvInProgress = true;
          break;
          default:
            if (!recvInProgress) {Serial.print('?'); break; } // expect only start marker initally
            tuple_values[position][ndx] = rc;
            ndx++;
            if (ndx >= numChars) {
                ndx = numChars - 1;
            }
          }  //switch  
    } //while
}

void recevieTupleAndSend() {
    if (newData == true) {
        
        strcpy(MQTT_path,"pub/"); //g46NIng-txfaUvdQZj0R6g/klrv5wfaa6t82ClQfcIR6g/");
        strcat(MQTT_path,mqttUser);
        strcat(MQTT_path,"/");
        strcat(MQTT_path,clientID);
        strcat(MQTT_path,"/");
        strcat(MQTT_path,tuple_values[0]);
        strcat(MQTT_path,"/");
        strcat(MQTT_path,tuple_values[1]);

        strcpy(MQTT_payload,tuple_values[2]);

       
        while (!client.connected()) {
          Serial.println("Connecting to MQTT...");
      
          if (client.connect(clientID, mqttUser, mqttPassword )) {
      
            Serial.println("connected");  
      
          } else {
      
            Serial.print("failed with state ");
            Serial.print(client.state());
            
      
          }
        }
        //client.publish("pub/g46NIng-txfaUvdQZj0R6g/klrv5wfaa6t82ClQfcIR6g/jWcrox0QP1arY-cAfcIR6g/co2","44.55");
        client.publish(MQTT_path, MQTT_payload); //Topic name
        newData = false;
        Serial.print("\nOK> ");
    }
}
