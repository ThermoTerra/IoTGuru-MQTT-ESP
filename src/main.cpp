#include <Arduino.h>
#include <ESP8266WiFi.h>

#define MQTT_KEEPALIVE 100 // change the value that PubSubClient will be using - keep conection alive for longer, as we wait for the end of interval.

#include <PubSubClient.h>
#include <secrets.h>
#include <esp_mqtt.h>
#include <MqttClient.h>
#include <user_interface.h>
 

static char MQTT_path[200];
static char MQTT_payload[20];

// used by serial read
const byte numChars = 50;
char receivedChars[numChars];
boolean newData = false;

// more efficienty to allocate this way
char tuple_values[TUPLE_MAX][numChars];

int sub_base_length; // the part of the subscription string which is constant - will not transfer to Mega

void reciveTuple();
void recevieTupleAndSend();
void callback(char* topic, byte* payload, unsigned int length);

WiFiClient espClient;
PubSubClient client(espClient);
 
void setup() {
 
  Serial.begin(115200);

  struct rst_info * rstinfo=system_get_rst_info();

  Serial.print("Reason for reset: ");
  switch (rstinfo->reason) {
    case 0: Serial.print("normal startup by power on "); break; 
    case 1: Serial.print("hardware watch dog reset "); break; 
    case 2: Serial.print("exception reset: GPIO status won’t change "); break; 
    case 3: Serial.print("software watch dog reset: GPIO status won’t change "); break; 
    case 4: Serial.print("software restart ,system_restart : GPIO status won’t change "); break; 
    case 5: Serial.print("wake up from deep-sleep "); break; 
    case 6: Serial.print(" external system reset "); break; 
  }
Serial.println();

 
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("SSID: ");
    Serial.print(ssid);
    Serial.println(": Connecting to WiFi..");
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

  char substring[200];
  // calculate the lentgh of the base string
  sub_base_length=4+strlen(mqttUser)+1+strlen(deviceID)+1; 
  sprintf(substring,"sub/%s/%s/kUE3wPdX4hRmtEBAi40R6g/fan_speed",mqttUser, deviceID);
  Serial.print("subscribing to bottom cell fan ");
  Serial.println(substring);
  client.subscribe(substring);
  sprintf(substring,"sub/%s/%s/ggSLn0x7f3iF4suAfl4R6g/fan_speed",mqttUser, deviceID);
  Serial.print("subscribing to to cell fan");
  Serial.println(substring);
  client.subscribe(substring);
  
  Serial.print("\nPrompt> ");
 
}
 
void callback(char* topic, byte* payload, unsigned int length) {
 
  
  Serial.print(returnValue);
  Serial.print(topic+sub_base_length); // print only from the point of difference 
 
  Serial.print("/");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
 
  Serial.println(returnValueTerm);
 
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
          case Seperator:
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
          case EndMarker:
            if (!recvInProgress) {Serial.print('!'); break; } // expect only start marker initally
            newData=true;
            // terminate this string
            tuple_values[position][ndx] = '\0';
            recvInProgress=false;
            position=0;
            ndx=0;

          break;
          case StartMarker:
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
          Serial.println("Reconnecting to MQTT...");
      
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
        Serial.print(millis());
        Serial.print("\nOK> ");
    }
}
