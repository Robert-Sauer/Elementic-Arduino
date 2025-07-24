#include <Arduino.h>
#include "Elementic.h"




void input(){
for (uint8_t i=1; i <= SwitchCounter; i++){
    if(digitalRead(SwitchPin[i])==HIGH||SwitchPressedExternal[i]){
        if(SwitchPressed[i]==false){
            Serial.print("Switch pressed: ");
            Serial.println(i);
            mqttClient.publish(MQTT_Input[i],"PRESSED");
            SwitchPressed[i] = true;
            for (uint8_t j = 0; j < InOutMatrixCouter[i]; j++){
                if(OutputType[InOutMatrix[i][j]]==1||OutputType[InOutMatrix[i][j]]==5){ //Lamp aan/uit, //Lamp aan/uit (Ext.)
                    if (OutputValueActual[InOutMatrix[i][j]]==0){
                        OutputValueActual[InOutMatrix[i][j]]=1;
                        }
                    else{
                        OutputValueActual[InOutMatrix[i][j]]=0;
                        }

                    // 29-03-2020 - Hier nog naar toe verplaatsen na testen type 2 en 4 , mqqt status
                    }
                if(OutputType[InOutMatrix[i][j]]==2||OutputType[InOutMatrix[i][j]]==4||OutputType[InOutMatrix[i][j]]==6){//Lamp PWM, LAMP DMX, Lamp PWM (Ext.)
                    if (OutputValueActual[InOutMatrix[i][j]]<=OutputMaxValue[InOutMatrix[i][j]]/2){
                        SwitchDimDirection[i] = true;
                        }
                    else{
                        SwitchDimDirection[i] = false;
                        }
                    }
                if(OutputType[InOutMatrix[i][j]]==3){ //RGB DMX
                    

                    }
                }            
            }

        else
            {
            SwitchTimer[i]++;
 
            for (uint8_t j = 0; j < InOutMatrixCouter[i]; j++)
                {              
                //Serial.print("Matrix i: ");
                //Serial.println(i);
                //Serial.print("Matrix j: ");
                //Serial.println(j);
                //Serial.print("Outputtype: ");
                //Serial.println(OutputType[InOutMatrix[i][j]]);
                 if(OutputType[InOutMatrix[i][j]]==1||OutputType[InOutMatrix[i][j]]==5){ //Lamp aan/uit, //Lamp aan/uit (Ext.)
                    

                    }
                if(OutputType[InOutMatrix[i][j]]==2||OutputType[InOutMatrix[i][j]]==4||OutputType[InOutMatrix[i][j]]==6){//Lamp PWM, LAMP DMX, Lamp PWM (Ext.)
                    if (SwitchTimer[i]>100){
                        if (!SwitchDimDirection[i]){
                            if (OutputValueActual[InOutMatrix[i][j]]>OutputMinValue[InOutMatrix[i][j]]){
                                OutputValueActual[InOutMatrix[i][j]]--;
                                //Serial.println(OutputValueActual[InOutMatrix[i][j]]);
                                }
                            }
                        else{
                            if (OutputValueActual[InOutMatrix[i][j]]<OutputMaxValue[InOutMatrix[i][j]]){
                                OutputValueActual[InOutMatrix[i][j]]++;
                                //Serial.println(OutputValueActual[InOutMatrix[i][j]]);
                                }
                            }
                        if(OutputType[InOutMatrix[i][j]]==2||OutputType[InOutMatrix[i][j]]==4){ // 29-03-2020 - hier geplaatst om bij output weg te halen
                            MQTTUpdate[InOutMatrix[i][j]] = true;
                            }
                        }                            
                    }
                if(OutputType[InOutMatrix[i][j]]==3) //RGB DMX
                    {

                    }
                }
            }
        }
    else{
        if(SwitchPressed[i] == true){
            for (uint8_t j = 0; j < InOutMatrixCouter[i]; j++)
                {
                switch (OutputType[InOutMatrix[i][j]]){
                    case 1: //Lamp aan/uit
                    break;
                    case 2: //Lamp PWM
                    if (SwitchTimer[i]<100&&SwitchTimer[i]>2) // Variabel toevoegen!!!!!!!!!!!!!!
                        {
                        if (OutputValueActual[InOutMatrix[i][j]]==0)
                            { //Fout oplossen, indien in een groep één is uit geschakeld, krijg je nooit alles uit
                            OutputValueActual[InOutMatrix[i][j]]=OutputDefaultValue[InOutMatrix[i][j]];
                            }
                        else
                            {
                            OutputValueActual[InOutMatrix[i][j]]=0;
                            }
                        }
                    break;
                    case 3: //RGB DMX
                    break;
                    case 4: //LAMP DMX
                    if (SwitchTimer[i]<100&&SwitchTimer[i]>2) // Variabel toevoegen!!!!!!!!!!!!!!
                        {
                        if (OutputValueActual[InOutMatrix[i][j]]==0)
                            {
                            OutputValueActual[InOutMatrix[i][j]]=OutputDefaultValue[InOutMatrix[i][j]];
                            }
                        else
                            {
                            OutputValueActual[InOutMatrix[i][j]]=0;
                            }
                        }
                    break;
                    case 5: //Lamp aan/uit (Ext.)
                    break;
                    case 6: //Lamp PWM (Ext.)
                    if (SwitchTimer[i]<100&&SwitchTimer[i]>2) // Variabel toevoegen!!!!!!!!!!!!!!
                        {
                        if (OutputValueActual[InOutMatrix[i][j]]==0)
                            {
                            OutputValueActual[InOutMatrix[i][j]]=OutputDefaultValue[InOutMatrix[i][j]];
                            }
                        else
                            {
                            OutputValueActual[InOutMatrix[i][j]]=0;
                            }
                        }
                    break;
                    }
                if(OutputType[InOutMatrix[i][j]]==2||OutputType[InOutMatrix[i][j]]==4){ // 29-03-2020 - hier geplaatst om bij output weg te halen
                    MQTTUpdate[InOutMatrix[i][j]] = true;
                    }         
                }  
            MQTTBurstProtection=0;
            Serial.print("Switch released: ");
            Serial.print(i);
            Serial.print(" ,time: ");
            Serial.println(SwitchTimer[i]);
            mqttClient.publish(MQTT_Input[i],"RELEASED");
            SwitchPressed[i] = false;
            SwitchTimer[i] = 0;
            }
        }
    }

}
// Implementation for input
