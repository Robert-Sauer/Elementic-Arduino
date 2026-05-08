/*
 * output.cpp
 *
 * Implements output state evaluation, relay updates, dimming, color handling, and other output-side runtime behavior.
 */

#include <Arduino.h>
#include "Elementic.h"
void OutputRelayUpdateFunction(){
    for (uint8_t i = 1; i <= OutputCounter; i++){ // 2020-02-02 - <= veranderd in <
        if (OutputRelay[i]>0){
            bool relaytemp = false;
            for (uint8_t j = 1; j <= OutputCounter; j++){
                if (OutputType[j]>0&&OutputType[j]<5){ // 5 is voor alleen interne types
                    if(OutputValueMemory[j]+OutputValueActual[j]>0&&OutputRelay[j]==OutputRelay[i]){ //OutputRedMemory[j]+OutputGreenMemory[j]+OutputBlueMemory[j]+
                        relaytemp = true;
                        }
                    }
                }
            if(relaytemp){
                digitalWrite(OutputRelay[i], HIGH);
                }
            else{
                digitalWrite(OutputRelay[i], LOW);
                }
            }
        }
    }

void MQTTSubscribe(){
for (int i=1; i <= OutputCounter; i++){
        //mqttClient.publish(MQTT_Output[i],"INITIALIZED"); //Moet één niveau dieper
        //OutputValueMemory[i] = 0; // Alles uit bij inschakelen 13-12-2025 - Dit veroorzaakte een uitschakeling bij elke reconnect, en ook een MQTT burst
        char suffixBuf[32]; // 22-08-2025
        switch (OutputType[i]){
        case 1: //Lamp aan/uit
        strcpy_P(suffixBuf, onoffset);
        snprintf(msgmqtt, sizeof(msgmqtt), "%s%s", MQTT_Output[i], suffixBuf);
        mqttClient.publish(msgmqtt,"INITIALIZED");
        mqttClient.subscribe(msgmqtt);
        break;
        case 2: //Lamp PWM
        strcpy_P(suffixBuf, onoffset);
        snprintf(msgmqtt, sizeof(msgmqtt), "%s%s", MQTT_Output[i], suffixBuf);
        mqttClient.publish(msgmqtt,"INITIALIZED");
        mqttClient.subscribe(msgmqtt);
        strcpy_P(suffixBuf, dimvalue_set);
        snprintf(msgmqtt, sizeof(msgmqtt), "%s%s", MQTT_Output[i], suffixBuf);
        mqttClient.publish(msgmqtt,"INITIALIZED");
        mqttClient.subscribe(msgmqtt);
        strcpy_P(suffixBuf, defaultvalue_set);
        snprintf(msgmqtt, sizeof(msgmqtt), "%s%s", MQTT_Output[i], suffixBuf);
        mqttClient.publish(msgmqtt,"INITIALIZED");
        mqttClient.subscribe(msgmqtt);
        break;
        case 3: //RGB DMX
        strcpy_P(suffixBuf, color_set);
        snprintf(msgmqtt, sizeof(msgmqtt), "%s%s", MQTT_Output[i], suffixBuf);
        mqttClient.publish(msgmqtt,"INITIALIZEDRGB");
        mqttClient.subscribe(msgmqtt);
        break;
        case 4: //LAMP DMX
        strcpy_P(suffixBuf, onoffset);
        snprintf(msgmqtt, sizeof(msgmqtt), "%s%s", MQTT_Output[i], suffixBuf);
        mqttClient.publish(msgmqtt,"INITIALIZED");
        mqttClient.subscribe(msgmqtt);
        strcpy_P(suffixBuf, dimvalue_set);
        snprintf(msgmqtt, sizeof(msgmqtt), "%s%s", MQTT_Output[i], suffixBuf);
        mqttClient.publish(msgmqtt,"INITIALIZED");
        mqttClient.subscribe(msgmqtt);
        strcpy_P(suffixBuf, defaultvalue_set);
        snprintf(msgmqtt, sizeof(msgmqtt), "%s%s", MQTT_Output[i], suffixBuf);
        mqttClient.publish(msgmqtt,"INITIALIZED");
        mqttClient.subscribe(msgmqtt);
        break;
        case 5: //Lamp aan/uit (Ext.)
        strcpy_P(suffixBuf, onoffstatus);
        snprintf(msgmqtt, sizeof(msgmqtt), "%s%s", MQTT_Output[i], suffixBuf);
        mqttClient.publish(msgmqtt,"INITIALIZED");
        mqttClient.subscribe(msgmqtt);
        break;
        case 6: //Lamp PWM (Ext.)
        strcpy_P(suffixBuf, dimvalue_status);
        snprintf(msgmqtt, sizeof(msgmqtt), "%s%s", MQTT_Output[i], suffixBuf);
        mqttClient.publish(msgmqtt,"INITIALIZED");
        mqttClient.subscribe(msgmqtt);
        strcpy_P(suffixBuf, defaultvalue_set);
        snprintf(msgmqtt, sizeof(msgmqtt), "%s%s", MQTT_Output[i], suffixBuf);
        //mqttClient.publish(msgmqtt,"INITIALIZED");
        mqttClient.subscribe(msgmqtt);
        break;
        case 7: //Signal
        strcpy_P(suffixBuf, onoffstatus);
        snprintf(msgmqtt, sizeof(msgmqtt), "%s%s", MQTT_Output[i], suffixBuf);
        mqttClient.publish(msgmqtt,"INITIALIZED");
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
    char suffixBuf[32]; // 25-08-2025
    for (int i=1; i <= OutputCounter; i++){
        if(OutputValueMemory[i]!=OutputValueActual[i]||OutputSignalCountdown[i]>0){//||OutputRedValue[i]!=OutputRedMemory[i]||OutputGreenValue[i]!=OutputGreenMemory[i]||OutputBlueValue[i]!=OutputBlueMemory[i]
            OutputRelayUpdate = true;
            switch (OutputType[i]){
                case 1: //Lamp aan/uit
                    // OutputRelayUpdateFunction();
                    if(OutputValueActual[i]>0){
                        //digitalWrite(OutputRelay[Relay[i]], HIGH); // 11-02-2020 Moet dit niet andersom zijn? of weg?
                        OutputValueMemory[i]=1;
                        SendOutputValueMQTT(i);
                        }
                    else{
                        //digitalWrite(OutputRelay[Relay[i]], LOW); // 11-02-2020 Moet dit niet andersom zijn? of weg?
                        OutputValueMemory[i]=0;
                        SendOutputValueMQTT(i);
                        }
                break;
                case 2: //Lamp PWM
                    // OutputRelayUpdateFunction();
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
                    // strcpy_P(suffixBuf, dimvalue_status);
                    // snprintf(msgmqtt, sizeof(msgmqtt), "%s%s", MQTT_Output[i], suffixBuf); //29-03-2020 verplaatst naar input/mqqt callback 08-12-2025 terug verplaatst
                    // snprintf(msgmqtt2, 20 ,"%d",OutputValueActual[i]);
                    // mqttClient.publish(msgmqtt,msgmqtt2);
                break;
                case 3: //RGB DMX
                    // OutputRelayUpdateFunction();
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
                    // OutputRelayUpdateFunction();
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
                    //snprintf(msgmqtt, sizeof(msgmqtt), "%s%s", MQTT_Output[i], dimvalue_status);
                    //snprintf(msgmqtt2, 20 ,"%d",OutputValueActual[i]);
                    //mqttClient.publish(msgmqtt,msgmqtt2);
                    DMXwrite(OutputChannel[i],OutputValueMemory[i]);
                    DMXstartupTimer = 40;
                    // DMXSend();
                break;
                case 5: //Lamp aan/uit (Ext.)
                    if(OutputValueActual[i]!=OutputValueMemory[i]){
                        SendOutputValueMQTT(i);
                        OutputValueMemory[i]=OutputValueActual[i];
                    }
                break;
                // case 6: //Lamp PWM (Ext.)
                //     if(OutputValueActual[i]!=OutputValueMemory[i]&&MQTTBurstProtection==0){
                //         strcpy_P(suffixBuf, dimvalue_set);
                //         snprintf(msgmqtt, sizeof(msgmqtt), "%s%s", MQTT_Output[i], suffixBuf);
                //         snprintf(msgmqtt2, 20 ,"%d",OutputValueActual[i]);
                //         mqttClient.publish(msgmqtt,msgmqtt2);
                //         OutputValueMemory[i]=OutputValueActual[i];
                //         MQTTBurstProtection=5;
                //         }
                //         MQTTBurstProtection--;
                //         //Serial.println("Output case 6");
                // break;
                case 6: //Lamp PWM (Ext.) No Burst Protection

                    if(OutputValueActual[i]!=OutputValueMemory[i])
                    {
                        strcpy_P(suffixBuf, dimvalue_set);
                        snprintf(msgmqtt, sizeof(msgmqtt), "%s%s", MQTT_Output[i], suffixBuf);
                        snprintf(msgmqtt2, sizeof(msgmqtt2), "%d", OutputValueActual[i]);

                        if (mqttClient.publish(msgmqtt, msgmqtt2)) {
                            OutputValueMemory[i] = OutputValueActual[i];
                        }
                    }
                break;

                case 7: //Signal
                    // OutputRelayUpdateFunction();
                    if(OutputValueActual[i]!=OutputValueMemory[i]){
                        //digitalWrite(OutputRelay[Relay[i]], HIGH); // 11-02-2020 Moet dit niet andersom zijn? of weg?
                        OutputValueMemory[i]=OutputValueActual[i];
                        // uint16_t signalTimeMs = (((uint16_t)OutputMaxValue[i] << 8) | (uint16_t)OutputMinValue[i]);
                        // OutputSignalCountdown[i] = signalTimeMs / 10;
                        OutputSignalCountdown[i] = (((uint16_t)OutputMaxValue[i] << 8) | (uint16_t)OutputMinValue[i]);

                        SendOutputValueMQTT(i);
                        snprintf(msgmqtt2, sizeof(msgmqtt2), "Set timer to: %lu ms.", (unsigned long)OutputSignalCountdown[i] * 10UL);
                        logging(LOG_INFO,msgmqtt2);
                        }
                    if (OutputSignalCountdown[i]>0){
                        if (OutputDefaultValue[i] & 0x02) {// bit 1(blinking) is TRUE
                            bool normallyOn = (OutputDefaultValue[i] & 0x01) != 0;
                            bool phaseOn = (OutputSignalCountdown[i] % blinkingTimer) < (blinkingTimer / 2);
                            digitalWrite(OutputPin[i], phaseOn ^ normallyOn ? HIGH : LOW);
                        }
                        if (!(OutputDefaultValue[i] & 0x02)) { // blinking FALSE
                            if (OutputDefaultValue[i] & 0x01) {      // normallyOn = 1
                                digitalWrite(OutputPin[i], LOW);
                            }

                            if (!(OutputDefaultValue[i] & 0x01)) {   // normallyOn = 0
                                digitalWrite(OutputPin[i], HIGH);
                            }
                        }
                        OutputSignalCountdown[i]--;
                    }
                    if (OutputSignalCountdown[i] == 0) {
                        if (OutputDefaultValue[i] & 0x01) { // NormallyOn = 1
                            digitalWrite(OutputPin[i], HIGH);
                            logging(LOG_INFO, "Set output to high");
                        } else { // NormallyOn = 0
                            digitalWrite(OutputPin[i], LOW);
                            logging(LOG_INFO, "Set output to low");
                        }

                        OutputValueMemory[i] = 0;
                        OutputValueActual[i] = 0;
                        SendOutputValueMQTT(i);
                        SendOutputValueSerial(i, 0);
                        logging(LOG_INFO, "End timer");
}

                break;

                }
            OutputRelayUpdateFunction();    
            }
        
        }
    if(DMXstartupTimer>0){
            DMXflush();
            DMXstartupTimer--;
            }
    }
    
void setrelays(){
    for (uint8_t i = 1; i <= OutputCounter; i++)
    {
        pinMode(OutputRelay[i], OUTPUT);
    }
    for (uint8_t i = 1; i <= SwitchCounter; i++)
    {
        pinMode(SwitchPin[i], INPUT);
    }
}
