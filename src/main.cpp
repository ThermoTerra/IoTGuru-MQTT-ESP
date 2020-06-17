#include <Arduino.h>
#include <ESP8266WiFi.h>




#include <PubSubClient.h>
#include <secrets.h>
#include <esp_mqtt.h>
#include <MqttClient.h>
#include <user_interface.h>
#include <ESP8266HTTPClient.h>
 
 #define RESEND_SUB_TIME 60000 // in millis


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


typedef struct {
    char description[30];
    const char *node;
    char field[100];
    byte value[20];
    int length;
} sub_messages;

#define NUM_SUB_TOPICS 3

sub_messages subscriptions[NUM_SUB_TOPICS] {
  {"topcell fan", TOPCELL_node, "fan_speed", 0,0},
  {"bottom fan", BOTTOMCELL_node, "fan_speed", 0,0},
  {"A/C switch", INROOM_node, "ac_on", 0,0},

};

void subscribe_topics(PubSubClient client )
{
 
  // calculate the lentgh of the base string
  sub_base_length=4+strlen(mqttUser)+1+strlen(deviceID)+1; 
  char sub_topic1_str[130];
  for (int i=0;i<NUM_SUB_TOPICS; i++)
  {
      sprintf(sub_topic1_str,"sub/%s/%s/%s/%s",mqttUser, deviceID,subscriptions[i].node, subscriptions[i].field);
      Serial.print("subscribing to " );
      Serial.println(subscriptions[i].description);
      Serial.println(sub_topic1_str);
      client.subscribe(sub_topic1_str);
  }
}

// every once in a while send the topics with the last recorded values over serial. This is useful in case of a reset on the receiving side
void retain_topics()
{
  char topic[150];
  //char str_value[15];
  for (int i=0;i<NUM_SUB_TOPICS; i++)
  {
      if (subscriptions[i].length>0) 
      {
        Serial.print("refreshing subscription ");
        Serial.print(subscriptions[i].description);
        Serial.print("with last value ");
        Serial.println((char *)subscriptions[i].value);
        sprintf(topic,"sub/%s/%s/%s/%s",mqttUser, deviceID,subscriptions[i].node, subscriptions[i].field);
    
        callback(topic, subscriptions[i].value,subscriptions[i].length);
      }
  }
}


WiFiClient espClient;
PubSubClient client(espClient);

char sub_topic1_str[200],sub_topic2_str[200],sub_topic3_str[200];

int time_diff=0;
time_t last_time=0;


time_t last_resend_time=0;


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
  Serial.println("Connected to the WiFi network");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP()); 

  struct rst_info * rstinfo=system_get_rst_info();

  Serial.print("Reason for last reset: ");
  switch (rstinfo->reason) {
    case 0: Serial.print("normal startup by power on "); sendEvent("RST-Normal Startup"); break; 
    case 1: Serial.print("hardware watch dog reset "); sendEvent("RST-Hardware Watchdog"); break; 
    case 2: Serial.print("exception reset: GPIO status won’t change "); sendEvent("RST-Exception reset");break; 
    case 3: Serial.print("software watch dog reset: GPIO status won’t change "); sendEvent("RST-SW watchdog"); break; 
    case 4: Serial.print("software restart ,system_restart : GPIO status won’t change "); sendEvent("RST-SW restart");break; 
    case 5: Serial.print("wake up from deep-sleep "); sendEvent("RST-Wakeup from sleep");break; 
    case 6: Serial.print("external system reset "); sendEvent("RST-External reset");break; 
  }
  Serial.println();




 
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
          Serial.println(client.state());
          sprintf(event,"MQTT fail %d", client.state());
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

  subscribe_topics(client);
  
  
  Serial.println();
  Serial.print(prompt);
 
}
 
void callback(char* topic, byte* payload, unsigned int length) {
 
  char * token=strtok(topic,"/");
  char *node, *field;
  int i;
  // skip first 3 tokens: %s/%s/%s/%s",mqttUser, deviceID,subscriptions[i].node, subscriptions[i].field
  for (i=0; i<3; i++ )
  {
    token=strtok(NULL,"/");
  }
  node=token;
  for (int i=0;i<NUM_SUB_TOPICS; i++)
  {
      if (strcmp(token, subscriptions[i].node)==0)
      {
        field=strtok(NULL,"/");
        
        if (strcmp(field, subscriptions[i].field)==0) 
        {
          for (int j=0;j<length; j++)
          {
            subscriptions[i].value[j]=payload[j];
          }
          subscriptions[i].length=length;
        }
      }
  }
 
  Serial.print(sub_prefix);
  Serial.print(node); 
  Serial.print(Seperator);
  Serial.print(field);
  Serial.print(Seperator);
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
  if (millis()-last_resend_time>RESEND_SUB_TIME)
  {
    retain_topics();
    last_resend_time=millis();
  }

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
          #ifdef _SERIAL
            Serial.print(err_prefix);
            Serial.println("Wifi disconnected");
          #endif
            while (WiFi.status() != WL_CONNECTED) {
            delay(500);
          #ifdef _SERIAL
            Serial.print(err_prefix);
            Serial.print("SSID: ");
            Serial.print(ssid);
            Serial.println(": Connecting to WiFi..");
          #endif
          }
          #ifdef _SERIAL
          Serial.print(err_prefix);
          Serial.println("Connected to the WiFi network");
          #endif
        }
        if (!client.connected()) {
          while (!client.connected()) {
          #ifdef _SERIAL
            Serial.print(err_prefix);
            Serial.println("Reconnecting to MQTT...");
          #endif
        
            if (client.connect(clientID, mqttUser, mqttPassword )) {
              Serial.print(err_prefix);
              Serial.println("connected");  
        
            } else {
              Serial.print(err_prefix);
              char event[32];
              Serial.print("failed with state ");
              Serial.print(client.state());
              sprintf(event,"MQTT fail %d", client.state());
              sendEvent(event);
        
            }

          }
          subscribe_topics(client);
        }
        
        client.publish(MQTT_path, MQTT_payload); //Topic name
        newData = false;
        Serial.print(millis());
        Serial.print(" ");
        Serial.print(prompt);
    }
}

 

void urlencode(char *tostr, char *str)
{

    int output_indx=0;
    char encodedString[100]=""; // up to 3 times as large as input
    char c;
    char code0;
    char code1;
    char code2;
    for (int i =0; i < 32; i++){
      c=str[i];
      if (c==0) break; //end of string
      if (c == ' '){
        encodedString[output_indx++]= '_';
      } else if (isprint(c)){
        encodedString[output_indx++]=c;
      } else{
        code1=(c & 0xf)+'0';
        if ((c & 0xf) >9){
            code1=(c & 0xf) - 10 + 'A';
        }
        c=(c>>4)&0xf;
        code0=c+'0';
        if (c > 9){
            code0=c - 10 + 'A';
        }
        code2='\0';
        encodedString[output_indx++]='.';
        //encodedString[output_indx++]=code0;
        //encodedString[output_indx++]=code1;
        //encodedString+=code2;
      }
      yield();
    }
    encodedString[output_indx]='\0';
    strcpy(tostr,encodedString);
}

void sendEvent(char *text)
{

    HTTPClient http;

    static char encoded_text[100],url[300];

    urlencode(encoded_text,text);

    sprintf(url,"http://%s/deviceEvent/create/%s/%s",httpServer,device_key,encoded_text);

    //Serial.print("[HTTP] begin...\n");
    if (http.begin(espClient, url)) {  // HTTP


      //Serial.print("[HTTP] GET ");
      Serial.println(url);
      http.addHeader("Content-Type", "application/json");
      // start connection and send HTTP header
      int httpCode = http.GET();

      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
          Serial.printf("[HTTP] GET... code: %d\n", httpCode);

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