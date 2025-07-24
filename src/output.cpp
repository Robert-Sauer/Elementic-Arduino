#include <Arduino.h>
#include "Elementic.h"
void OutputRelayUpdateFunction(){
            for (uint8_t i = 0; i < TotalRelays; i++){ // 2020-02-02 - <= veranderd in <
                bool relaytemp = false;
                for (uint8_t j = 1; j <= OutputCounter; j++){
                    if (OutputType[j]>0&&OutputType[j]<5){ // 5 is voor alleen interne types
                        if(OutputValueMemory[j]+OutputValueActual[j]>0&&OutputRelay[j]==i){ //OutputRedMemory[j]+OutputGreenMemory[j]+OutputBlueMemory[j]+
                            relaytemp = true;
                            }
                        }
                }
            if(relaytemp){
                digitalWrite(Relay[i], HIGH);
                }
            else{
                digitalWrite(Relay[i], LOW);
                }
            }
        }

void MQTTSubscribe(){
for (int i=1; i <= OutputCounter; i++){
        //mqttClient.publish(MQTT_Output[i],"INITIALIZED"); //Moet één niveau dieper
        OutputValueMemory[i] = 0; // Alles uit bij inschakelen
        switch (OutputType[i]){
        case 1: //Lamp aan/uit
        sprintf(msgmqtt, "%s%s", MQTT_Output[i], onoffset);
        mqttClient.publish(msgmqtt,"INITIALIZED");
        mqttClient.subscribe(msgmqtt);
        break;
        case 2: //Lamp PWM
        sprintf(msgmqtt, "%s%s", MQTT_Output[i], onoffset);
        mqttClient.publish(msgmqtt,"INITIALIZED");
        mqttClient.subscribe(msgmqtt);
        sprintf(msgmqtt, "%s%s", MQTT_Output[i], dimvalue_set);
        mqttClient.publish(msgmqtt,"INITIALIZED");
        mqttClient.subscribe(msgmqtt);
        sprintf(msgmqtt, "%s%s", MQTT_Output[i], defaultvalue_set);
        mqttClient.publish(msgmqtt,"INITIALIZED");
        mqttClient.subscribe(msgmqtt);
        break;
        case 3: //RGB DMX
        sprintf(msgmqtt, "%s%s", MQTT_Output[i], color_set);
        mqttClient.publish(msgmqtt,"INITIALIZEDRGB");
        mqttClient.subscribe(msgmqtt);        
        break;
        case 4: //LAMP DMX
        sprintf(msgmqtt, "%s%s", MQTT_Output[i], onoffset);
        mqttClient.publish(msgmqtt,"INITIALIZED");
        mqttClient.subscribe(msgmqtt);
        sprintf(msgmqtt, "%s%s", MQTT_Output[i], dimvalue_set);
        mqttClient.publish(msgmqtt,"INITIALIZED");
        mqttClient.subscribe(msgmqtt);
        sprintf(msgmqtt, "%s%s", MQTT_Output[i], defaultvalue_set);
        mqttClient.publish(msgmqtt,"INITIALIZED");
        mqttClient.subscribe(msgmqtt);
        break;
        case 5: //Lamp aan/uit (Ext.)
        sprintf(msgmqtt, "%s%s", MQTT_Output[i], onoffstatus);
        mqttClient.publish(msgmqtt,"INITIALIZED");
        mqttClient.subscribe(msgmqtt);
        break;
        case 6: //Lamp PWM (Ext.)
        sprintf(msgmqtt, "%s%s", MQTT_Output[i], dimvalue_status);
        mqttClient.publish(msgmqtt,"INITIALIZED");
        mqttClient.subscribe(msgmqtt);
        sprintf(msgmqtt, "%s%s", MQTT_Output[i], defaultvalue_set);
        //mqttClient.publish(msgmqtt,"INITIALIZED");
        mqttClient.subscribe(msgmqtt);
        break;
        }
    }
}

// void outputsetup(){
//     outputsetupvariables(); // Laadt alle output variabelen
    
//     for (uint8_t i = 0; i < TotalRelays; i++) //TotalRelays
//     {
//         pinMode(Relay[i], OUTPUT);
//     }
    
// } 

void output(){
    OutputRelayUpdate = false;
    for (int i=1; i <= OutputCounter; i++){
        if(OutputValueMemory[i]!=OutputValueActual[i]){//||OutputRedValue[i]!=OutputRedMemory[i]||OutputGreenValue[i]!=OutputGreenMemory[i]||OutputBlueValue[i]!=OutputBlueMemory[i]
            OutputRelayUpdate = true;
            switch (OutputType[i]){
                case 1: //Lamp aan/uit
                    if(OutputValueActual[i]>0){
                        //digitalWrite(OutputRelay[Relay[i]], HIGH); // 11-02-2020 Moet dit niet andersom zijn? of weg?
                        sprintf(msgmqtt, "%s%s", MQTT_Output[i], onoffstatus); // 03-04-2020, dit niet uitschakelen?
                        mqttClient.publish(msgmqtt,"ON");
                        OutputValueMemory[i]=1;
                        }
                    else{
                        //digitalWrite(OutputRelay[Relay[i]], LOW); // 11-02-2020 Moet dit niet andersom zijn? of weg?
                        sprintf(msgmqtt, "%s%s", MQTT_Output[i], onoffstatus);
                        mqttClient.publish(msgmqtt,"OFF");
                        OutputValueMemory[i]=0;
                        }
                break;
                case 2: //Lamp PWM
                    if(OutputValueActual[i]>OutputValueMemory[i])
                        {
                        OutputValueMemory[i]++;
                        analogWrite(OutputPin[i], OutputValueMemory[i]);
                        }
                    if(OutputValueActual[i]<OutputValueMemory[i])
                        {
                        OutputValueMemory[i]--;
                        analogWrite(OutputPin[i], OutputValueMemory[i]);
                        }
                        //sprintf(msgmqtt, "%s%s", MQTT_Output[i], dimvalue_status); 29-03-2020 verplaatst naar input/mqqt callback
                        //snprintf(msgmqtt2, 20 ,"%d",OutputValueActual[i]);
                        //mqttClient.publish(msgmqtt,msgmqtt2);
                break;
                case 3: //RGB DMX
                    // if(OutputRedValue[i]>OutputRedMemory[i]){
                    //     OutputRedMemory[i]++;
                    //     }
                    // if(OutputRedValue[i]<OutputRedMemory[i]){
                    //     OutputRedMemory[i]--;
                    //     }
                    // if(OutputGreenValue[i]>OutputGreenMemory[i]){
                    //     OutputGreenMemory[i]++;
                    //     }
                    // if(OutputGreenValue[i]<OutputGreenMemory[i]){
                    //     OutputGreenMemory[i]--;
                    //     }
                    // if(OutputBlueValue[i]>OutputBlueMemory[i]){
                    //     OutputBlueMemory[i]++;
                    //     }
                    // if(OutputBlueValue[i]<OutputBlueMemory[i]){
                    //     OutputBlueMemory[i]--;
                    //     }
                    // DMXwrite(OutputChannel[i],OutputGreenMemory[i]);
                    // DMXwrite(OutputChannel[i]+1,OutputRedMemory[i]);
                    // DMXwrite(OutputChannel[i]+2,OutputBlueMemory[i]);
                    // DMXstartupTimer = 40;
                    // //DMXSend();
                break;
                case 4: //LAMP DMX
                    if(OutputValueActual[i]>OutputValueMemory[i]) // 2020-02-03 de komende twee if's toegevoegd
                        {
                        OutputValueMemory[i]++;
                        //analogWrite(OutputPin[i], OutputValueMemory[i]);
                        }
                    if(OutputValueActual[i]<OutputValueMemory[i])
                        {
                        OutputValueMemory[i]--;
                        //analogWrite(OutputPin[i], OutputValueMemory[i]);
                        }
                    //sprintf(msgmqtt, "%s%s", MQTT_Output[i], dimvalue_status);
                    //snprintf(msgmqtt2, 20 ,"%d",OutputValueActual[i]);
                    //mqttClient.publish(msgmqtt,msgmqtt2);
                    DMXwrite(OutputChannel[i],OutputValueMemory[i]);
                    DMXstartupTimer = 40;
                    //DMXSend();
                break;
                case 5: //Lamp aan/uit (Ext.)
                    if(OutputValueActual[i]!=OutputValueMemory[i]){
                        sprintf(msgmqtt, "%s%s", MQTT_Output[i], onoffset);
                        if(OutputValueActual[i]==0){
                            snprintf(msgmqtt2, 20 ,"%d","OFF");
                            }
                        if(OutputValueActual[i]>0){
                            snprintf(msgmqtt2, 20 ,"%d","ON");
                            }
                        mqttClient.publish(msgmqtt,msgmqtt2);
                        OutputValueMemory[i]=OutputValueActual[i];
                    }
                break;
                case 6: //Lamp PWM (Ext.)
                    if(OutputValueActual[i]!=OutputValueMemory[i]&&MQTTBurstProtection==0){
                        sprintf(msgmqtt, "%s%s", MQTT_Output[i], dimvalue_set);
                        snprintf(msgmqtt2, 20 ,"%d",OutputValueActual[i]);
                        mqttClient.publish(msgmqtt,msgmqtt2);
                        OutputValueMemory[i]=OutputValueActual[i];
                        MQTTBurstProtection=5;
                        }
                        MQTTBurstProtection--;
                        //Serial.println("Output case 6");
                break;
                }
            OutputRelayUpdateFunction();
            }
        
        }
    if(DMXstartupTimer>0){
            DMXflush();
            DMXstartupTimer--;
            }

    // move to project        
    // if(DoorBellEnabled==true){
    //     if(DoorBellCounter>0){
    //         if(DoorBellCounter%20>10){
    //             digitalWrite(DoorBellLightOutput, LOW);    // LED
    //             }
    //         else{
    //             digitalWrite(DoorBellLightOutput, HIGH);    // LED 
    //             }
    //         if(DoorBellCounter>325){ // Variabel toevoegen
    //             digitalWrite(DoorBellRingOutput, HIGH);   // Deurbel
    //             }
    //         else{
    //             digitalWrite(DoorBellRingOutput, LOW);   // Deurbel 
    //             }
    //         DoorBellCounter = DoorBellCounter -1;
    //         }
    //     else{
    //         digitalWrite(DoorBellLightOutput, HIGH);  // LED
    //         digitalWrite(DoorBellRingOutput, LOW);   // Deurbel
    //         }
    //     }
    }

// Implementation for output
