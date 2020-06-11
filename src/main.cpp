#include <Arduino.h>
#include <ESP8266WiFi.h>




#include <PubSubClient.h>
#include <secrets.h>
#include <esp_mqtt.h>
#include <MqttClient.h>
#include <user_interface.h>
#include <ESP8266HTTPClient.h>
 


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
void sendEvent(char *text);

WiFiClient espClient;
PubSubClient client(espClient);

char sub_topic1_str[200],sub_topic2_str[200],sub_topic3_str[200];

int time_diff=0;
time_t last_time=0;

void setup() {

  Serial.begin(115200);


Serial.println();
 /* IPAddress ip(192, 168, 0, 177); 
  IPAddress gw(192, 168, 0, 19); 
  IPAddress subnet(255, 255, 255, 0); 
  IPAddress dns (203,145,184,13);
  WiFi.config(ip,gw,subnet);*/
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("SSID: ");
    Serial.print(ssid);
    Serial.println(": Connecting to WiFi..");
  }


  struct rst_info * rstinfo=system_get_rst_info();

  Serial.print("Reason for last reset: ");
  switch (rstinfo->reason) {
    case 0: Serial.print("normal startup by power on "); sendEvent("RST:Normal Startup; connected"); break; 
    case 1: Serial.print("hardware watch dog reset "); sendEvent("RST: Hardware Watchdog; connected"); break; 
    case 2: Serial.print("exception reset: GPIO status won’t change "); sendEvent("RST: Exception reset; connected");break; 
    case 3: Serial.print("software watch dog reset: GPIO status won’t change "); sendEvent("RST: SW watchdog; connected"); break; 
    case 4: Serial.print("software restart ,system_restart : GPIO status won’t change "); sendEvent("RST: SW restart; connected");break; 
    case 5: Serial.print("wake up from deep-sleep "); sendEvent("RST: Wakeup from sleep, connected");break; 
    case 6: Serial.print("external system reset "); sendEvent("RST: External reset, connected");break; 
  }


  Serial.println("Connected to the WiFi network");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP()); 


 
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
  //sendEvent("Connected");
 
  
  while (!client.connected()) {
        last_time=millis(); // implement without the delay();   
        Serial.println("Connecting to MQTT...");
    
        if (client.connect(clientID, mqttUser, mqttPassword )) {
    
          Serial.println("connected");  
    
        } else {
          char event[32];
          Serial.print("failed with state ");
          Serial.print(client.state());
          sprintf(event,"MQTT failed with state %d", client.state());
          sendEvent(event);
          //delay(2000);
        while(time_diff<2000)
         {
           time_diff=millis()-last_time;
         }
         last_time=millis();
         time_diff=0;
       }
  }
  client.publish("pub/g46NIng-txfaUvdQZj0R6g/klrv5wfaa6t82ClQfcIR6g/jWcrox0QP1arY-cAfcIR6g/co2","00.00");

 
  // calculate the lentgh of the base string
  sub_base_length=4+strlen(mqttUser)+1+strlen(deviceID)+1; 
  sprintf(sub_topic1_str,"sub/%s/%s/%s/fan_speed",mqttUser, deviceID,BOTTOMCELL_node);
  Serial.print("subscribing to bottom cell fan ");
  Serial.println(sub_topic1_str);
  client.subscribe(sub_topic1_str);
  sprintf(sub_topic2_str,"sub/%s/%s/%s/fan_speed",mqttUser, deviceID,TOPCELL_node);
  Serial.print("subscribing to to cell fan");
  Serial.println(sub_topic2_str);
  client.subscribe(sub_topic2_str);
  sprintf(sub_topic3_str,"sub/%s/%s/%s/ac_on",mqttUser, deviceID,INROOM_node);
  Serial.print("subscribing to AC on-off ");
  Serial.println(sub_topic3_str);
  client.subscribe(sub_topic3_str);
  
  Serial.println();
  Serial.print(prompt);
 
}
 
void callback(char* topic, byte* payload, unsigned int length) {
 
  
  Serial.print(sub_prefix);
  Serial.print(topic+sub_base_length); // print only from the point of difference 
 
  Serial.print("/");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
 
  Serial.println(sub_suffix);
 
}
 

void loop() {
  
  reciveTuple();
  client.loop();
  recevieTupleAndSend();
  //delay(1);
  while(time_diff<5)
  {
    time_diff=millis()-last_time;
    if (time_diff<0) // counter overflowed and starting over
    {
      last_time=time_diff;
      time_diff=0;
    }
  }
  last_time=millis();
  time_diff=0;
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
        client.loop(); // to make sure we keep MQTT alive even through this loop
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
                Serial.print(err_prefix);
                Serial.println("tupple exceeded max");
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
            Serial.println();
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
        if (WiFi.status() != WL_CONNECTED) {
            Serial.print(err_prefix);
            Serial.println("Wifi disconnected");
            while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            Serial.print(err_prefix);
            Serial.print("SSID: ");
            Serial.print(ssid);
            Serial.println(": Connecting to WiFi..");
          }
          Serial.print(err_prefix);
          Serial.println("Connected to the WiFi network");
        }
        if (!client.connected()) {
          while (!client.connected()) {
            Serial.print(err_prefix);
            Serial.println("Reconnecting to MQTT...");
        
            if (client.connect(clientID, mqttUser, mqttPassword )) {
              Serial.print(err_prefix);
              Serial.println("connected");  
        
            } else {
              Serial.print(err_prefix);
              char event[32];
              Serial.print("failed with state ");
              Serial.print(client.state());
              sprintf(event,"MQTT failed with state %d", client.state());
              sendEvent(event);
        
            }

          }
          client.subscribe(sub_topic1_str);
          client.subscribe(sub_topic2_str);
        }
        
        client.publish(MQTT_path, MQTT_payload); //Topic name
        newData = false;
        Serial.print(millis());
        Serial.print(" ");
        Serial.print(prompt);
    }
}


void sendEvent(char *text)
{

    HTTPClient http;

    static char url[200];

    sprintf(url,"http://%s/deviceEvent/create/%s/%.32s",httpServer,device_key,text);

    Serial.print("[HTTP] begin...\n");
    if (http.begin(espClient, url)) {  // HTTP


      //Serial.print("[HTTP] GET ");
      //Serial.println(url);
      http.addHeader("Content-Type", "application/json");
      //http.addHeader("Content-Type", "application/json");
      // start connection and send HTTP header
      int httpCode = http.GET();

      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
       // Serial.printf("[HTTP] GET... code: %d\n", httpCode);

        // file found at server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          String payload = http.getString();
          Serial.println(payload);
        }
      } else {
        Serial.print(err_prefix);
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }

      http.end();
    } else {
      Serial.print(err_prefix);
      Serial.printf("[HTTP} Unable to connect\n");
    }
  
}