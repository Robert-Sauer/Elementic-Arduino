#include <Arduino.h>
#include "Elementic.h"
#include <EEPROM.h>
#define MAX_SERIAL_BUFFER 500

template <typename Func>
void ProcessVariablesByPriority(Func func) {
    for (int id = 0; id < totalVariables; id++) {
        func(id, variables[id]);
    }
}

void serialsetup() {
    Serial.begin(115200);
    Serial.println("Serial port initiated. . .");
    databuffersize = 255; // kan niet hoger zijn dan 255
}

void serialwrite(byte i) {
    if (databufferposition >= databuffersize) {
        delay(1);
        databufferposition = 0;
    }
    Serial.write(i);
    databufferposition++;
}

void serialPrintLimited(char data[400]) {
    size_t dataLen = strlen(data);
    if ((databufferposition + dataLen) >= databuffersize) {
        delay(1);
        databufferposition = 0;
    }
    Serial.print(data);
    databufferposition += dataLen;
}

int DynamicVariableActions(int EP, bool ER, bool SD, bool MEM, DynamicVariable &var,int varId) {
    // Write variable type block ("Type")
    if (SD) { serialPrintLimited("S02"); }
    if (SD) { serialwrite(2); }    //if (ER) { EEPROM.write(EP++, (byte)2); } // Type1: Generic
    if (SD) { serialwrite(0); }    //if (ER) { EEPROM.write(EP++, (byte)0); } // Type2: Type
    if (SD) { serialwrite(var.priority); } //if (ER) { EEPROM.write(EP++, (byte)var.priority); } // id
    if (SD) { serialwrite(0); }    //if (ER) { EEPROM.write(EP++, (byte)0); } // length not required
    if (SD) { serialwrite((byte)var.type); } //if (ER) { EEPROM.write(EP++, (byte)var.type); } // value
    if (SD) { serialPrintLimited("E02"); }
    
    
    // Write variable Name block
    if (SD) { serialPrintLimited("S02"); }
    if (SD) { serialwrite(2); }    //if (ER) { EEPROM.write(EP++, (byte)2); } // Type1: Generic
    if (SD) { serialwrite(1); }    //if (ER) { EEPROM.write(EP++, (byte)1); } // Type2: Name
    if (SD) { serialwrite(var.priority); } //if (ER) { EEPROM.write(EP++, (byte)var.priority); } // id
    // Write string length for name
    size_t nameLen = strlen(var.name.c_str());
    if (SD) { serialwrite((byte)nameLen); } 
    //if (ER) { EEPROM.write(EP++, (byte)nameLen); }
    // Write the name bytes
    if (SD) { Serial.write(var.name.c_str(), nameLen); } 
    //if (ER) {
    //    for (int i2 = 0; i2 < nameLen; i2++) {
    //        EEPROM.write(EP++, var.name.c_str()[i2]);
    //    }
    //}
    if (SD) { serialPrintLimited("E02"); }

    // Write variable Value block
    if (SD) { serialPrintLimited("S02"); }
    if (SD) { serialwrite(2); }    if (ER) { EEPROM.write(EP++, (byte)2); } // Type1: Generic
    if (SD) { serialwrite(2); }    if (ER) { EEPROM.write(EP++, (byte)2); } // Type2: Value
    if (SD) { serialwrite(var.priority); } if (ER) { EEPROM.write(EP++, (byte)varId); } // id

    switch (var.type) {
        case DYNAMIC_STRING: {
            size_t strLen = strlen(var.value.strVal);
            if (SD) { serialwrite((byte)strLen); } 
            if (ER) { EEPROM.write(EP++, (byte)strLen); }
            if (SD) { serialPrintLimited(var.value.strVal); }
            if (ER) {
                for (int i2 = 0; i2 < strLen; i2++) {
                    EEPROM.write(EP++, var.value.strVal[i2]);
                }
            }
            break;
        }
        case DYNAMIC_BYTE:
            if (SD) { serialwrite(0); }    if (ER) { EEPROM.write(EP++, (byte)0); }
            if (SD) { serialwrite(var.value.byteVal); } 
            if (ER) { EEPROM.write(EP++, (byte)var.value.byteVal); }
            break;
        case DYNAMIC_INT: {
            if (SD) { serialwrite(0); }    if (ER) { EEPROM.write(EP++, (byte)0); } // 16-04-2024 length
            // Write 2-byte integer (high byte first)
            if (SD) {
                serialwrite((byte)(var.value.intVal >> 8));
                serialwrite((byte)(var.value.intVal & 0x00));
            }
            if (ER) {
                EEPROM.write(EP++, (byte)(var.value.intVal >> 8));
                EEPROM.write(EP++, (byte)(var.value.intVal & 0x00));
            }
            break;
        }
        case DYNAMIC_IP:
            if (SD) { serialwrite(0); } if (ER) { EEPROM.write(EP++, (byte)0); }// length not required, added ER 0 16-04-2025
            for (int i = 0; i < 4; i++) {
                if (SD) { serialwrite(var.value.ipVal[i]); } 
                if (ER) { EEPROM.write(EP++, (byte)var.value.ipVal[i]); }
            }
            break;
        case DYNAMIC_PASSWORD: {
            size_t passLen = strlen(var.value.passVal);
            if (SD) { serialwrite((byte)passLen); } 
            if (ER) { EEPROM.write(EP++, (byte)passLen); }
            if (SD) { serialPrintLimited(var.value.passVal); }
            if (ER) {
                for (int i2 = 0; i2 < passLen; i2++) {
                    EEPROM.write(EP++, var.value.passVal[i2]);
                }
            }
            break;
        }
    }
    if (SD) { serialPrintLimited("E02"); }
    
    return EP;  // Return the updated pointer
}

void SendAllData(bool ER, bool SD, bool MEM) { // ER = store to EEPROM, SD = send via Serial, MEM = reserved for MEM
    if (SD) { sprintf(msgmqtt, "%s%s", MQTTName, "/LOG"); mqttClient.publish(msgmqtt, "Sending serial data"); }
    if (ER) { sprintf(msgmqtt, "%s%s", MQTTName, "/LOG"); mqttClient.publish(msgmqtt, "Storing EEPROM Data"); }
    int EP = 0; // EEPROM pointer
    char tempstring[20];
    
    if (SD) { serialPrintLimited("S01"); } // Initiate frame
    if (SD) { serialPrintLimited("1"); }    if (ER) { EEPROM.write(EP++, (byte)1); } // Type
    if (SD) { serialPrintLimited("1"); }    if (ER) { EEPROM.write(EP++, (byte)1); } // Version

    // Process inputs
    for (byte i = 1; i <= SwitchCounter; i++) {
        // Write channel for input
        if (SD) { serialPrintLimited("S02"); }
        if (SD) { serialwrite(1); }   if (ER) { EEPROM.write(EP++, (byte)1); }  // Type1: input
        if (SD) { serialwrite(0); }   if (ER) { EEPROM.write(EP++, (byte)0); }  // Type2: channel
        if (SD) { serialwrite(i); }   if (ER) { EEPROM.write(EP++, (byte)i); }  // id
        if (SD) { serialwrite(0); }   if (ER) { EEPROM.write(EP++, (byte)0); }  // length not required
        if (SD) { serialwrite(i); }   if (ER) { EEPROM.write(EP++, (byte)SwitchPin[i]); }  // value
        if (SD) { serialPrintLimited("E02"); }

        // Write outputs for input block
        if (SD) { serialPrintLimited("S02"); }
        if (SD) { serialwrite(1); }   if (ER) { EEPROM.write(EP++, (byte)1); }  // Type1: input
        if (SD) { serialwrite(1); }   if (ER) { EEPROM.write(EP++, (byte)1); }  // Type2: Outputs
        if (SD) { serialwrite(i); }   if (ER) { EEPROM.write(EP++, (byte)i); }  // id

        //memset(InputOutputsString[i], 0, sizeof(InputOutputsString[i])); // -- remain line for future JSON implementation
        //for (byte i2 = 0; i2 < InOutMatrixCouter[i]; i2++) {
        //    char buffer[4];
        //    itoa(InOutMatrix[i][i2], buffer, 10);  // Convert to string (base 10)
        //    strcat(InputOutputsString[i], buffer);
        //    if (i2 < InOutMatrixCouter[i] - 1) {
        //        strcat(InputOutputsString[i], ",");
        //    }
        //}

        // For Serial: write the length of the comma-separated string.
        if (SD) {
            //serialwrite((byte)strlen(InputOutputsString[i])); // 30-04-2025 -- convert to bytes!!!!!!! no CSV!!!! both android as here -- remain line for future JSON implementation
            serialwrite((byte)InOutMatrixCouter[i]);
        }
        // For EEPROM: write the number of outputs.
        if (ER) {
            EEPROM.write(EP++, (byte)InOutMatrixCouter[i]);
        }


        // Serial: write each character of the string.
        if (SD) {
            for (byte k = 0; k < InOutMatrixCouter[i]; k++) {
                serialwrite((byte) InOutMatrix[i][k]);        // value
            }
        }

        // EEPROM: write each output value.
        if (ER) {
            for (byte i2 = 0; i2 < InOutMatrixCouter[i]; i2++) {
                EEPROM.write(EP++, (byte)InOutMatrix[i][i2]);
            }
        }
        if (SD) { serialPrintLimited("E02"); }

        // Write Topic for input
        if (SD) { serialPrintLimited("S02"); }
        if (SD) { serialwrite(1); }   if (ER) { EEPROM.write(EP++, (byte)1); }  // Type1: input
        if (SD) { serialwrite(2); }   if (ER) { EEPROM.write(EP++, (byte)2); }  // Type2: channel
        if (SD) { serialwrite(i); }   if (ER) { EEPROM.write(EP++, (byte)i); }  // id
        size_t mqttInputLen = strlen(MQTT_Input[i]);
        if (SD) { serialwrite((byte)mqttInputLen); }   if (ER) { EEPROM.write(EP++, (byte)mqttInputLen); }
        if (SD) { serialPrintLimited(MQTT_Input[i]); }
        if (ER) {
            for (int i2 = 0; i2 < (int)mqttInputLen; i2++) {
                EEPROM.write(EP++, MQTT_Input[i][i2]);
            }
        }
        if (SD) { serialPrintLimited("E02"); }
    }

    // Process outputs
    for (byte i = 1; i <= OutputCounter; i++) {
        // Channel
        if (SD) { serialPrintLimited("S02"); }
        if (SD) { serialwrite(0); }   if (ER) { EEPROM.write(EP++, (byte)0); }
        if (SD) { serialwrite(0); }   if (ER) { EEPROM.write(EP++, (byte)0); }
        if (SD) { serialwrite(i); }   if (ER) { EEPROM.write(EP++, (byte)i); }
        if (SD) { serialwrite(0); }   if (ER) { EEPROM.write(EP++, (byte)0); }
        if (SD) { serialwrite(i); }   if (ER) { EEPROM.write(EP++, (byte)i); }
        if (SD) { serialPrintLimited("E02"); }

        // Type
        if (SD) { serialPrintLimited("S02"); }
        if (SD) { serialwrite(0); }   if (ER) { EEPROM.write(EP++, (byte)0); }
        if (SD) { serialwrite(1); }   if (ER) { EEPROM.write(EP++, (byte)1); }
        if (SD) { serialwrite(i); }   if (ER) { EEPROM.write(EP++, (byte)i); }
        if (SD) { serialwrite(0); }   if (ER) { EEPROM.write(EP++, (byte)0); }
        if (SD) { serialwrite((byte)OutputType[i]); } if (ER) { EEPROM.write(EP++, (byte)OutputType[i]); }
        if (SD) { serialPrintLimited("E02"); }

        // Relay channel
        if (SD) { serialPrintLimited("S02"); }
        if (SD) { serialwrite(0); }   if (ER) { EEPROM.write(EP++, (byte)0); }
        if (SD) { serialwrite(2); }   if (ER) { EEPROM.write(EP++, (byte)2); }
        if (SD) { serialwrite(i); }   if (ER) { EEPROM.write(EP++, (byte)i); }
        if (SD) { serialwrite(0); }   if (ER) { EEPROM.write(EP++, (byte)0); }
        if (SD) { serialwrite((byte)OutputRelay[i]); } if (ER) { EEPROM.write(EP++, (byte)OutputRelay[i]); }
        if (SD) { serialPrintLimited("E02"); }

        // Minvalue
        if (SD) { serialPrintLimited("S02"); }
        if (SD) { serialwrite(0); }   if (ER) { EEPROM.write(EP++, (byte)0); }
        if (SD) { serialwrite(3); }   if (ER) { EEPROM.write(EP++, (byte)3); }
        if (SD) { serialwrite(i); }   if (ER) { EEPROM.write(EP++, (byte)i); }
        if (SD) { serialwrite(0); }   if (ER) { EEPROM.write(EP++, (byte)0); }
        if (SD) { serialwrite((byte)OutputMinValue[i]); } if (ER) { EEPROM.write(EP++, (byte)OutputMinValue[i]); }
        if (SD) { serialPrintLimited("E02"); }
        
        // Maxvalue
        if (SD) { serialPrintLimited("S02"); }
        if (SD) { serialwrite(0); }   if (ER) { EEPROM.write(EP++, (byte)0); }
        if (SD) { serialwrite(4); }   if (ER) { EEPROM.write(EP++, (byte)4); }
        if (SD) { serialwrite(i); }   if (ER) { EEPROM.write(EP++, (byte)i); }
        if (SD) { serialwrite(0); }   if (ER) { EEPROM.write(EP++, (byte)0); }
        if (SD) { serialwrite((byte)OutputMaxValue[i]); } if (ER) { EEPROM.write(EP++, (byte)OutputMaxValue[i]); }
        if (SD) { serialPrintLimited("E02"); }

        // Default value
        if (SD) { serialPrintLimited("S02"); }
        if (SD) { serialwrite(0); }   if (ER) { EEPROM.write(EP++, (byte)0); }
        if (SD) { serialwrite(5); }   if (ER) { EEPROM.write(EP++, (byte)5); }
        if (SD) { serialwrite(i); }   if (ER) { EEPROM.write(EP++, (byte)i); }
        if (SD) { serialwrite(0); }   if (ER) { EEPROM.write(EP++, (byte)0); }
        if (SD) { serialwrite((byte)OutputDefaultValue[i]); } if (ER) { EEPROM.write(EP++, (byte)OutputDefaultValue[i]); }
        if (SD) { serialPrintLimited("E02"); }

        // Name
        if (SD) { serialPrintLimited("S02"); }
        if (SD) { serialwrite(0); }   if (ER) { EEPROM.write(EP++, (byte)0); }
        if (SD) { serialwrite(6); }   if (ER) { EEPROM.write(EP++, (byte)6); }
        if (SD) { serialwrite(i); }   if (ER) { EEPROM.write(EP++, (byte)i); }
        strncpy(tempstring, "No name yet", sizeof(tempstring) - 1);
        tempstring[sizeof(tempstring) - 1] = '\0'; // Ensure null termination
        size_t tempLen = strlen(tempstring);
        if (SD) { serialwrite((byte)tempLen); } 
        if (ER) { EEPROM.write(EP++, (byte)tempLen); }
        if (SD) { serialPrintLimited(tempstring); }
        if (ER) {
            for (int i2 = 0; i2 < (int)tempLen; i2++) {
                EEPROM.write(EP++, tempstring[i2]);
            }
        }
        if (SD) { serialPrintLimited("E02"); }

        // Topic
        if (SD) { serialPrintLimited("S02"); }
        if (SD) { serialwrite(0); }   if (ER) { EEPROM.write(EP++, (byte)0); }
        if (SD) { serialwrite(7); }   if (ER) { EEPROM.write(EP++, (byte)7); }
        if (SD) { serialwrite(i); }   if (ER) { EEPROM.write(EP++, (byte)i); }
        size_t mqttOutputLen = strlen(MQTT_Output[i]);
        if (SD) { serialwrite((byte)mqttOutputLen); }   if (ER) { EEPROM.write(EP++, (byte)mqttOutputLen); }
        if (SD) { serialPrintLimited(MQTT_Output[i]); }
        if (ER) {
            for (int i2 = 0; i2 < (int)mqttOutputLen; i2++) {
                EEPROM.write(EP++, MQTT_Output[i][i2]);
            }
        }
        if (SD) { serialPrintLimited("E02"); }


        // Value
        if (SD) { serialPrintLimited("S02"); }
        if (SD) { serialwrite(0); }   if (ER) { EEPROM.write(EP++, (byte)0); }
        if (SD) { serialwrite(8); }   if (ER) { EEPROM.write(EP++, (byte)8); }
        if (SD) { serialwrite(i); }   if (ER) { EEPROM.write(EP++, (byte)i); }
        if (SD) { serialwrite(0); }   if (ER) { EEPROM.write(EP++, (byte)0); }
        if (SD) { serialwrite((byte)OutputValueActual[i]); } if (ER) { EEPROM.write(EP++, (byte)OutputValueActual[i]); }
        if (SD) { serialPrintLimited("E02"); }

        //if (ER) { EEPROM.write(EP, 0xFF); } // End marker for EEPROM block
    }

    // Process dynamic variables by priority
    ProcessVariablesByPriority([&EP, ER, SD, MEM](int id, DynamicVariable &var) {
        // id is now available here alongside var
        EP = DynamicVariableActions(EP, ER, SD, MEM, var, id);
    });

    if (ER) {
        EEPROM.write(EP++, 0xFF); // Write a final end marker
        EEPROM.write(EP++, 0xFF); // Write a final end marker
        EEPROM.write(EP++, 0xFF); // Write a final end marker
        #if defined(ESP32) || defined(ESP8266)
            EEPROM.commit();       // Only needed on ESP platforms
        #endif
    }

    // End frame marker

    serialPrintLimited("E01");
}

void clearIncomingData() {
    memset(incomingdata, 0, sizeof(incomingdata));
    data_index = 0;
    Serial.flush();
}

void ReadAllDataFromEEPROM() {
    int EP = 0; // EEPROM pointer
    byte type, type01, type02, id, length, version;

    sprintf(msgmqtt, "%s%s", MQTTName, "/LOG");
    mqttClient.publish(msgmqtt, "Reading data from EEPROM...");

    // Read type and version
    type = EEPROM.read(EP++);
    version = EEPROM.read(EP++);
    
    char debugBuf[100];
    sprintf(debugBuf, "Type: %d, Version: %d", type, version);
    mqttClient.publish(msgmqtt, debugBuf);

    if (type != 1 || version != 1) {
        mqttClient.publish(msgmqtt, "Invalid EEPROM data! Aborting.");
        return;
    }

    // Loop with a safeguard against overrun
    while (EP < EEPROM.length() && EEPROM.read(EP) != 0xFF) {
        //sprintf(debugBuf, "Processing block at EP = %d", EP);
        //mqttClient.publish(msgmqtt, debugBuf);

        type01 = EEPROM.read(EP++);
        type02 = EEPROM.read(EP++);
        id = EEPROM.read(EP++);
        length = EEPROM.read(EP++);

        //sprintf(debugBuf, "Block header - type01: %d, type02: %d, id: %d, length: %d",
        //        type01, type02, id, length);
        //mqttClient.publish(msgmqtt, debugBuf);

        byte i2 = 0;
        switch (type01) {
            case 0: // Output
                if (OutputCounter < id) { OutputCounter = id; }
                switch (type02) {
                    case 0: { // channel
                        byte val = EEPROM.read(EP++);
                        OutputChannel[id] = val;
                        sprintf(debugBuf, "OutputChannel[%d] = %d", id, val);
                        //mqttClient.publish(msgmqtt, debugBuf);
                        break;
                    }
                    case 1: { // Type
                        byte val = EEPROM.read(EP++);
                        OutputType[id] = val;
                        sprintf(debugBuf, "OutputType[%d] = %d", id, val);
                        //mqttClient.publish(msgmqtt, debugBuf);
                        break;
                    }
                    case 2: { // Relay channel
                        byte val = EEPROM.read(EP++);
                        OutputRelay[id] = val;
                        sprintf(debugBuf, "OutputRelay[%d] = %d", id, val);
                        //mqttClient.publish(msgmqtt, debugBuf);
                        break;
                    }
                    case 3: { // minValue
                        byte val = EEPROM.read(EP++);
                        OutputMinValue[id] = val;
                        sprintf(debugBuf, "OutputMinValue[%d] = %d", id, val);
                        //mqttClient.publish(msgmqtt, debugBuf);
                        break;
                    }
                    case 4: { // maxValue
                        byte val = EEPROM.read(EP++);
                        OutputMaxValue[id] = val;
                        sprintf(debugBuf, "OutputMaxValue[%d] = %d", id, val);
                        //mqttClient.publish(msgmqtt, debugBuf);
                        break;
                    }
                    case 5: { // DefaultValue
                        byte val = EEPROM.read(EP++);
                        OutputDefaultValue[id] = val;
                        sprintf(debugBuf, "OutputDefaultValue[%d] = %d", id, val);
                        //mqttClient.publish(msgmqtt, debugBuf);
                        break;
                    }
                    case 6: { // Name
                        char nameBuffer[50];
                        memset(nameBuffer, 0, sizeof(nameBuffer));
                        for (i2 = 0; i2 < length && i2 < sizeof(nameBuffer)-1; i2++) {
                            nameBuffer[i2] = (char)EEPROM.read(EP++);
                        }
                        nameBuffer[i2] = '\0';
                        strcpy(MQTT_Name[id], nameBuffer);
                        sprintf(debugBuf, "Output Name[%d] = %s", id, nameBuffer);
                        //mqttClient.publish(msgmqtt, debugBuf);
                        break;
                    }
                    case 7: { // Topic
                        char topicBuffer[50];
                        memset(topicBuffer, 0, sizeof(topicBuffer));
                        for (i2 = 0; i2 < length && i2 < sizeof(topicBuffer)-1; i2++) {
                            topicBuffer[i2] = (char)EEPROM.read(EP++);
                        }
                        topicBuffer[i2] = '\0';
                        strcpy(MQTT_Output[id], topicBuffer);
                        sprintf(debugBuf, "Output Topic[%d] = %s", id, topicBuffer);
                        //mqttClient.publish(msgmqtt, debugBuf);
                        break;
                    }
                    case 8: { // Value
                        byte val = EEPROM.read(EP++);
                        OutputValueActual[id] = val;
                        sprintf(debugBuf, "Output ValueActual[%d] = %d", id, val);
                        mqttClient.publish(msgmqtt, debugBuf);
                        break;
                    }
                    default:
                        EP += length;
                        sprintf(debugBuf, "Unknown output block. Skipped %d bytes.", length);
                        //mqttClient.publish(msgmqtt, debugBuf);
                        break;
                }
                break;
            case 1: // Input
                if (SwitchCounter < id) { SwitchCounter = id; }
                switch (type02) {
                    case 0: { // channel/pin
                        byte val = EEPROM.read(EP++);
                        SwitchPin[id] = val;
                        sprintf(debugBuf, "SwitchPin[%d] = %d", id, val);
                        //mqttClient.publish(msgmqtt, debugBuf);
                        break;
                    }
                    case 1: { // Outputs (multiple bytes)
                        InOutMatrixCouter[id] = length;
                        char matrixBuf[50] = "";
                        for (i2 = 0; i2 < length && i2 < sizeof(matrixBuf)-1; i2++) {
                            InOutMatrix[id][i2] = (byte) EEPROM.read(EP++);
                            char temp[5];
                            sprintf(temp, "%d ", InOutMatrix[id][i2]);
                            strcat(matrixBuf, temp);
                        }
                        sprintf(debugBuf, "InOutMatrix[%d] = %s", id, matrixBuf);
                        //mqttClient.publish(msgmqtt, debugBuf);
                        break;
                    }
                    case 2: { // Topic
                        char inputTopicBuf[50];
                        memset(inputTopicBuf, 0, sizeof(inputTopicBuf));
                        for (i2 = 0; i2 < length && i2 < sizeof(inputTopicBuf)-1; i2++) {
                            inputTopicBuf[i2] = (char)EEPROM.read(EP++);
                        }
                        inputTopicBuf[i2] = '\0';
                        strcpy(MQTT_Input[id], inputTopicBuf);
                        sprintf(debugBuf, "Input Topic[%d] = %s", id, inputTopicBuf);
                        //mqttClient.publish(msgmqtt, debugBuf);
                        break;
                    }
                    default:
                        EP += length;
                        sprintf(debugBuf, "Unknown input block. Skipped %d bytes.", length);
                        //mqttClient.publish(msgmqtt, debugBuf);
                        break;
                }
                break;
            case 2: { // Generic variables
                sprintf(debugBuf, "EEPROM READ Case 2(step 1), type: [%d]",variables[id].type);
                mqttClient.publish(msgmqtt, debugBuf);
                if (id >= totalVariables) break; // Safety check
                switch (type02) {
                    case 0: {
                        byte val = EEPROM.read(EP++);
                        variables[id].type = (VariableType)val;
                        sprintf(debugBuf, "Variable[%d] Type = %d", id, val);
                        mqttClient.publish(msgmqtt, debugBuf);
                        break;
                    }
                    case 1: {
                        char varName[50];
                        memset(varName, 0, sizeof(varName));
                        if (length >= MAX_STRING_LENGTH) length = MAX_STRING_LENGTH - 1;
                        for (i2 = 0; i2 < length && i2 < sizeof(varName)-1; i2++) {
                            varName[i2] = (char)EEPROM.read(EP++);
                        }
                        varName[i2] = '\0';
                        variables[id].name = varName;
                        sprintf(debugBuf, "Variable[%d] Name = %s", id, varName);
                        mqttClient.publish(msgmqtt, debugBuf);
                        break;
                    }
                    case 2: {
                        sprintf(debugBuf, "EEPROM READ Case 2, type: [%d]",variables[id].type);
                        mqttClient.publish(msgmqtt, debugBuf);
                        switch (variables[id].type) {
                            case DYNAMIC_STRING: {
                                char strVal[50];
                                memset(strVal, 0, sizeof(strVal));
                                if (length >= MAX_STRING_LENGTH) length = MAX_STRING_LENGTH - 1;
                                for (i2 = 0; i2 < length && i2 < sizeof(strVal)-1; i2++) {
                                    strVal[i2] = (char)EEPROM.read(EP++);
                                }
                                strVal[i2] = '\0';
                                strcpy(variables[id].value.strVal, strVal);
                                sprintf(debugBuf, "Variable[%d] Value (String) = %s", id, strVal);
                                mqttClient.publish(msgmqtt, debugBuf);
                                break;
                            }
                            case DYNAMIC_BYTE: {
                                byte bVal = EEPROM.read(EP++);
                                variables[id].value.byteVal = bVal;
                                sprintf(debugBuf, "Variable[%d] Value (Byte) = %d", id, bVal);
                                mqttClient.publish(msgmqtt, debugBuf);
                                break;
                            }
                            case DYNAMIC_INT: {
                                int intVal = (EEPROM.read(EP++) << 8) | EEPROM.read(EP++);
                                variables[id].value.intVal = intVal;
                                sprintf(debugBuf, "Variable[%d] Value (Int) = %d", id, intVal);
                                mqttClient.publish(msgmqtt, debugBuf);
                                break;
                            }
                            case DYNAMIC_IP: {
                                byte ipBytes[4];
                                for (i2 = 0; i2 < 4; i2++) {
                                    ipBytes[i2] = EEPROM.read(EP++);
                                }
                                snprintf(debugBuf, sizeof(debugBuf),
                                        "Variable[%d] Value (IP) = %d.%d.%d.%d",
                                        id,
                                        ipBytes[0], ipBytes[1], ipBytes[2], ipBytes[3]);
                                mqttClient.publish(msgmqtt, debugBuf);

                                // store
                                memcpy(variables[id].value.ipVal, ipBytes, 4);
                                break;
                            }
                            case DYNAMIC_PASSWORD: {
                                char passBuf[50];
                                memset(passBuf, 0, sizeof(passBuf));
                                if (length >= MAX_STRING_LENGTH) length = MAX_STRING_LENGTH - 1;
                                for (i2 = 0; i2 < length && i2 < sizeof(passBuf)-1; i2++) {
                                    passBuf[i2] = (char)EEPROM.read(EP++);
                                }
                                passBuf[i2] = '\0';
                                strcpy(variables[id].value.passVal, passBuf);
                                sprintf(debugBuf, "Variable[%d] Value (Password) = %s", id, passBuf);
                                mqttClient.publish(msgmqtt, debugBuf);
                                break;
                            }
                        }
                        break;
                    }
                    default:
                        EP += length;
                        sprintf(debugBuf, "Unknown generic variable block. Skipped %d bytes.", length);
                        mqttClient.publish(msgmqtt, debugBuf);
                        break;
                }
                break;
            }
            case 3: // System (not stored in EEPROM)
                break;
            default:
                EP += length;
                sprintf(debugBuf, "Unknown block type. Skipped %d bytes.", length);
                mqttClient.publish(msgmqtt, debugBuf);
                break;
        }
    }
    sprintf(debugBuf, "Final EP: %d", EP);
    mqttClient.publish(msgmqtt, debugBuf);
    EEPROMStorage = EP;
}



void serialloop() {
    // 1) Read incoming bytes from Serial
    while (Serial.available() > 0) {
        incomingByte = Serial.read();
        uint8_t byteValue = (uint8_t)incomingByte;
        if (data_index < MAX_SERIAL_BUFFER) {
            incomingdata[data_index++] = byteValue;
        } else {
            Serial.println("Warning: Serial buffer overflow");
            clearIncomingData();
        }
    }

    // Need at least 5 bytes to check for "REQ01"
    if (data_index < 5) {
        return;
    }

    // 2) Check for "REQ01" in the buffer
    for (int i = 0; i <= data_index - 5; i++) {
        if (incomingdata[i]     == 'R' &&
            incomingdata[i + 1] == 'E' &&
            incomingdata[i + 2] == 'Q' &&
            incomingdata[i + 3] == '0' &&
            incomingdata[i + 4] == '1') {
            SendAllData(false, true, false); // (ER, SD, MEM)
            clearIncomingData();
            return;
        }
    }

    // 3) Look for a complete "S01 ... E01" block.
    int startPos = -1;
    int endPos = -1;
    for (int i = 0; i <= data_index - 3; i++) {
        if (incomingdata[i] == 'S' && incomingdata[i + 1] == '0' && incomingdata[i + 2] == '1') {
            startPos = i;
            break;
        }
    }
    if (startPos == -1) return;
    for (int j = startPos + 3; j <= data_index - 3; j++) {
        if (incomingdata[j] == 'E' && incomingdata[j + 1] == '0' && incomingdata[j + 2] == '1') {
            endPos = j;
            break;
        }
    }
    if (endPos == -1) return;

    // 4) Parse all "S02" sub-blocks between S01 and E01
    int counter = startPos + 3;
    while (true) {
        if (counter + 2 > endPos) break;
        if (incomingdata[counter] != 'S' || incomingdata[counter + 1] != '0' || incomingdata[counter + 2] != '2')
            break;
        counter += 3;
        if (counter + 4 + 1 + 3 > endPos + 1) break;
        byte Type01 = incomingdata[counter++];
        byte Type02 = incomingdata[counter++];
        byte id = incomingdata[counter++];
        byte length2 = incomingdata[counter++];

        snprintf(msgmqtt, sizeof(msgmqtt), "%s%s", MQTTName, "/RECEIVEDDATA");
        snprintf(msgmqtt2, sizeof(msgmqtt2), "Received updated: TYPE01=%d, TYPE02=%d, ID=%d, LENGTH=%d ", Type01, Type02, id,length2);
        mqttClient.publish(msgmqtt, msgmqtt2);

        if (counter >= endPos) break;
        
        // Apply data based on header values:
        byte Value;
        switch (Type01) {
            case 0: // Output
                Value = incomingdata[counter++];
                if (id < OutputCounter) {
                    switch (Type02) {
                        case 0: OutputChannel[id] = Value; break;
                        case 1: OutputType[id] = Value; break;
                        case 2: OutputRelay[id] = Value; break;
                        case 3: OutputMinValue[id] = Value; break;
                        case 4: OutputMaxValue[id] = Value; break;
                        case 5: OutputDefaultValue[id] = Value; break;
                        case 8: OutputValueActual[id] = Value; break;
                        default: break;
                    }
                    // MQTT post for Output update
                    snprintf(msgmqtt, sizeof(msgmqtt), "%s%s", MQTTName, "/OUTPUT");
                    snprintf(msgmqtt2, sizeof(msgmqtt2), "Output updated: id=%d, Type=%d, Value=%d", id, Type02, Value);
                    mqttClient.publish(msgmqtt, msgmqtt2);
                }
                break;
            case 1: // Input
                //Value = incomingdata[counter++];
                if (id < SwitchCounter) {
                    switch (Type02) {
                        case 0: SwitchPin[id] = incomingdata[counter++]; 
                        
                        break;
                        // Extend if needed for additional input subtypes
                        case 1: // switch matrix
                        InOutMatrixCouter[id]= length2;
                         for (byte i = 0; i < length2; i++)
                         {
                            InOutMatrix[id][i] = incomingdata[counter++];
                         }
                        case 2: //Topic
                        break;
                        case 3: //Type 0:on/off, 1:temperature
                        break;
                        case 4: //Value
                            uint8_t highByte = incomingdata[counter++];
                            uint8_t lowByte  = incomingdata[counter++];
                            int value = (highByte << 8) | lowByte; 
                            if (value==0){
                                SwitchPressedExternal[id]=false;
                            }
                            if (value==1){
                                SwitchPressedExternal[id]=true;
                            }
                        break;
                        //default: 
                        //break;
                    }
                }
                break;
            case 2: // Generic
                
                switch (Type02) {
                    case 0: break; // should not be changable externally
                    case 1: break; // should not be changable externally
                    case 2: //only type 2 can contain data
                        {
                            int priority = findVariableIndexByPriority(id);
                            int generictype = (int) variables[priority].type;
                            snprintf(msgmqtt, sizeof(msgmqtt), "%s%s", MQTTName, "/GENERICTYPE");
                            snprintf(msgmqtt2, sizeof(msgmqtt2), "Generic Typeof id=%d, Type=%d", id, generictype);
                            mqttClient.publish(msgmqtt, msgmqtt2);
                            //switch (generictype) {
                            //    case DYNAMIC_BYTE:
                            if(variables[priority].type == DYNAMIC_BYTE){
                                    Value = incomingdata[counter++]; 
                                    variables[id].value.byteVal = Value;
                                    snprintf(msgmqtt, sizeof(msgmqtt), "%s%s", MQTTName, "/GENERIC");
                                    snprintf(msgmqtt2, sizeof(msgmqtt2), "Generic BYTE updated: id=%d, Value=%d", priority, Value);
                                    mqttClient.publish(msgmqtt, msgmqtt2);
                                }
                            //        break;
                            //    case DYNAMIC_INT:
                            if(variables[priority].type == DYNAMIC_INT){                    
                                    byte ValueHigh = incomingdata[counter++]; 
                                    byte ValueLow = incomingdata[counter++]; 
                                    variables[id].value.intVal = ((int)ValueHigh << 8) | ValueLow;
                                    // MQTT post for Generic INT update
                                    snprintf(msgmqtt, sizeof(msgmqtt), "%s%s", MQTTName, "/GENERIC");
                                    snprintf(msgmqtt2, sizeof(msgmqtt2), "Generic INT updated: id=%d, ValueHigh=%d, ValueLow=%d, Combined=%d", priority, ValueHigh, ValueLow, variables[id].value.intVal);
                                    mqttClient.publish(msgmqtt, msgmqtt2);
                                }
                            //        break;
                            //    case DYNAMIC_IP:
                            if(variables[priority].type == DYNAMIC_IP){
                                    snprintf(msgmqtt, sizeof(msgmqtt), "%s%s", MQTTName, "/GENERICBefore");
                                    snprintf(msgmqtt2, sizeof(msgmqtt2), "Generic IP updated: id=%d", priority);
                                    mqttClient.publish(msgmqtt, msgmqtt2);
                                    byte IPAddress[4];
                                    IPAddress[0] = incomingdata[counter++];
                                    IPAddress[1] = incomingdata[counter++];
                                    IPAddress[2] = incomingdata[counter++];
                                    IPAddress[3] = incomingdata[counter++];
                                    variables[priority].value.ipVal[0] = IPAddress[0];
                                    variables[priority].value.ipVal[1] = IPAddress[1];
                                    variables[priority].value.ipVal[2] = IPAddress[2];
                                    variables[priority].value.ipVal[3] = IPAddress[3];
                                    // MQTT post for Generic IP update
                                    snprintf(msgmqtt, sizeof(msgmqtt), "%s%s", MQTTName, "/GENERICAfter");
                                    snprintf(msgmqtt2, sizeof(msgmqtt2), "Generic IP updated: id=%d, IP=%d.%d.%d.%d", priority, IPAddress[0], IPAddress[1], IPAddress[2], IPAddress[3]);
                                    mqttClient.publish(msgmqtt, msgmqtt2);
                                }
                            //    case DYNAMIC_String:
                            if(variables[priority].type == DYNAMIC_STRING||variables[priority].type == DYNAMIC_PASSWORD){
                                mqttClient.publish(msgmqtt, msgmqtt2);
                                char ReceivedString[30];
                                memset(ReceivedString, 0, sizeof(ReceivedString));
                                byte i2;
                                for(i2=0;i2<length2&&i2<sizeof(ReceivedString)-1;i2++)
                                    {
                                        ReceivedString[i2] = (char)incomingdata[counter++];
                                    }
                                
                                ReceivedString[i2] = '\0';

                                if(variables[priority].type == DYNAMIC_STRING){
                                    strcpy(variables[priority].value.strVal, ReceivedString);
                                    }
                                if(variables[priority].type == DYNAMIC_PASSWORD){
                                    strcpy(variables[priority].value.passVal, ReceivedString);
                                    }
                                // MQTT post for Generic IP update
                                snprintf(msgmqtt, sizeof(msgmqtt), "%s%s", MQTTName, "/GENERIC");
                                snprintf(msgmqtt2, sizeof(msgmqtt2), "Generic String updated: id=%d, Text=%s", priority, ReceivedString);
                                mqttClient.publish(msgmqtt, msgmqtt2);
                            }    
                            break;
                        }
                    //case 3:
                    //break; // should not be changable externally
                    //case 4: 
                    //break; // should not be changable externally
                    //case 5: 
                    //break; // should not be changable externally
                    default:{
                        snprintf(msgmqtt, sizeof(msgmqtt), "%s%s", MQTTName, "/GENERIC");
                        snprintf(msgmqtt2, sizeof(msgmqtt2), "No Type");
                        mqttClient.publish(msgmqtt, msgmqtt2);
                        break; 
                        }
                    }
                break;
            case 3: // System
            {
                byte ValueHigh = incomingdata[counter++];
                byte ValueLow = incomingdata[counter++];
                int ValueInt = ((int)ValueHigh << 8) | ValueLow;

                snprintf(msgmqtt, sizeof(msgmqtt), "%s%s", MQTTName, "/LOG");
                snprintf(msgmqtt2, sizeof(msgmqtt2), "SerialCommand TYPE02=3: %d", Value);
                mqttClient.publish(msgmqtt, msgmqtt2);
                if (Type02 == 6) {
                    SerialCommand = ValueInt;
                    snprintf(msgmqtt, sizeof(msgmqtt), "%s%s", MQTTName, "/LOG");
                    snprintf(msgmqtt2, sizeof(msgmqtt2), "SerialCommand: %d", Value);
                    mqttClient.publish(msgmqtt, msgmqtt2);
                }
            
                break;
            }
            default:
            {
                break;
            }
        }
        // can this be removed?? ****************
        if (counter + 2 > endPos) break;
        if (incomingdata[counter] == 'E' && incomingdata[counter + 1] == '0' && incomingdata[counter + 2] == '2') {
            counter += 3;
        } else {
            break;
        }
        //****************************************
    }
    clearIncomingData();


}



void SendSystem(SystemType systemTypeVal, int value) {
    Serial.print("S01"); // Initiate
    Serial.print("S02"); // Marker
    serialwrite(3); // Type01: 3 = System
    serialwrite(1); // Type02: 1 = Value
    serialwrite((byte)systemTypeVal); // system type as id
    serialwrite(0); // length not required
    // Write 2-byte value
    serialwrite((byte)(value >> 8));
    serialwrite((byte)(value & 0xFF));
    Serial.print("E02");
    Serial.print("E01");
}
