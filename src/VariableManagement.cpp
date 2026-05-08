/*
 * VariableManagement.cpp
 *
 * Implements helper functions to register variables and update status flags for inputs, outputs, and generic variables.
 */

#include <Arduino.h>
#include "Elementic.h"

void SetStatusFlag(byte &statusRegister, byte statusFlag, bool value) {
    if (value) {
        statusRegister |= statusFlag;
    } else {
        statusRegister &= ~statusFlag;
    }
}

void AddVariable( // integrate enum
    byte type1,
    byte type2,
    String varName,
    String MQTTTopic,
    byte pin,
    byte status,
    byte value,
    byte minValue,
    byte maxValue,
    byte defaultValue,
    byte relayPin){ // TBD: All possible vaiables?

}

void ModifyVariableStatus(byte type1, byte ID, const String& genericName, byte statusFlag, bool value) {
    switch (type1) {
        case VARIABLE_DYNAMIC: {
            if (genericName.length() == 0) return;

            int idx = findVariableIndexByName(genericName);
            if (idx < 0 || idx >= totalVariables) return;
            if (variables[idx].name.length() == 0) return;

            SetStatusFlag(DynamicVariableStatus[idx], statusFlag, value);
            SendStatus(VARIABLE_DYNAMIC, (byte)variables[idx].priority, DynamicVariableStatus[idx]);
            return;
        }

        case VARIABLE_OUTPUT:
            if (ID > OutputCounter) return;
            SetStatusFlag(OutputStatus[ID], statusFlag, value);
            SendStatus(VARIABLE_OUTPUT, ID, OutputStatus[ID]);
            return;

        case VARIABLE_INPUT:
            if (ID > SwitchCounter) return;
            SetStatusFlag(InputStatus[ID], statusFlag, value);
            SendStatus(VARIABLE_INPUT, ID, InputStatus[ID]);
            return;
    }
}

bool IsVariableStatusSet(byte type1, byte ID, const String& genericName, byte statusFlag) {
    switch (type1) {
        case VARIABLE_DYNAMIC: {
            int idx = findVariableIndexByName(genericName);
            if (idx >= 0 && idx < totalVariables) {
                return (DynamicVariableStatus[idx] & statusFlag) != 0;
            }
            break;
        }

        case VARIABLE_OUTPUT: {
            if (ID <= OutputCounter) {
                return (OutputStatus[ID] & statusFlag) != 0;
            }
            break;
        }

        case VARIABLE_INPUT: {
            if (ID <= InputChannels+1) {
                return (InputStatus[ID] & statusFlag) != 0;
            }
            break;
        }

        default:
            break;
    }
    return false;
}

// DeleteVariable(byte Type, byte ID, string genericName){

// }
