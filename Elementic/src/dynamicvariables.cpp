/*
 * dynamicvariables.cpp
 *
 * Implements the dynamic variable registry, typed storage accessors, and helper routines for generic configuration values.
 */

#include "Elementic.h"
#include <Arduino.h>
#include <string.h>

// -------------------
//    TYPE DEFINITIONS
// -------------------

// Calculate total variables
//const int totalVariables = TOTAL_STRINGS + TOTAL_BYTES + TOTAL_INTS + TOTAL_IPS + TOTAL_PASSWORDS;

// Global array of all variables
// DynamicVariable variables[totalVariables];

// Indices to keep track of insertion points per type
int stringIndex   = 0;
int byteIndex     = 0;
int intIndex      = 0;
int ipIndex       = 0;
int passwordIndex = 0;
int boolIndex     = 0;

// -------------------
//   HELPER FUNCTIONS
// -------------------

// Find the index of a variable by name
int findVariableIndexByName(const String& varName) {
    if (varName.length() == 0) return -1;

    for (int i = 0; i < totalVariables; i++) {
        if (variables[i].name.length() > 0 && variables[i].name == varName) {
            return i;
        }
    }
    return -1;
}

// -------------------
//    DEFINE FUNCTIONS
// -------------------

void DefineString(const String& varName, const char val[20], int priority) {
    if (stringIndex < TOTAL_STRINGS) {
        int idx = stringIndex;
        variables[idx].type = DYNAMIC_STRING;
        variables[idx].name = varName;
        //variables[idx].value.strVal = strdup(val); // duplicate the string
        strncpy(variables[idx].value.strVal, val, MAX_STRING_LENGTH - 1);
        variables[idx].value.strVal[MAX_STRING_LENGTH - 1] = '\0'; // Always null-terminate
        variables[idx].priority = priority;
        stringIndex++;
    } else {
        Serial.println("No more space for strings!");
    }
}
//void DefineString(const String& varName, char val[20], int priority) {
//    if (stringIndex < TOTAL_STRINGS) {
//        int idx = stringIndex;
//        variables[idx].type = DYNAMIC_STRING;
//        variables[idx].name = varName;

        // Instead of strdup, copy the string safely
//        strncpy(variables[idx].value.strVal, val, sizeof(variables[idx].value.strVal));
//        variables[idx].value.strVal[sizeof(variables[idx].value.strVal) - 1] = '\0'; // Null terminator safety

//        variables[idx].priority = priority;
//        stringIndex++;
//    } else {
//        Serial.println("No more space for strings!");
//    }
//}

void DefineByte(const String& varName, byte val, int priority) {
    if (byteIndex < TOTAL_BYTES) {
        int idx = TOTAL_STRINGS + byteIndex;
        variables[idx].type = DYNAMIC_BYTE;
        variables[idx].name = varName;
        variables[idx].value.byteVal = val;
        variables[idx].priority = priority;
        byteIndex++;
    } else {
        Serial.println("No more space for bytes!");
    }
}

void DefineInt(const String& varName, int val, int priority) {
    if (intIndex < TOTAL_INTS) {
        int idx = TOTAL_STRINGS + TOTAL_BYTES + intIndex;
        variables[idx].type = DYNAMIC_INT;
        variables[idx].name = varName;
        variables[idx].value.intVal = val;
        variables[idx].priority = priority;
        intIndex++;
    } else {
        Serial.println("No more space for ints!");
    }
}

void DefineIP(const String& varName, byte ip1, byte ip2, byte ip3, byte ip4, int priority) {
    if (ipIndex < TOTAL_IPS) {
        int idx = TOTAL_STRINGS + TOTAL_BYTES + TOTAL_INTS + ipIndex;
        variables[idx].type = DYNAMIC_IP;
        variables[idx].name = varName;
        variables[idx].value.ipVal[0] = ip1;
        variables[idx].value.ipVal[1] = ip2;
        variables[idx].value.ipVal[2] = ip3;
        variables[idx].value.ipVal[3] = ip4;
        variables[idx].priority = priority;
        ipIndex++;
    } else {
        Serial.println("No more space for IPs!");
    }
}

void DefinePassword(const String& varName, const char* val, int priority) {
    if (passwordIndex < TOTAL_PASSWORDS) {
        int idx = TOTAL_STRINGS + TOTAL_BYTES + TOTAL_INTS + TOTAL_IPS + passwordIndex;
        variables[idx].type = DYNAMIC_PASSWORD;
        variables[idx].name = varName;
        //variables[idx].value.passVal = strdup(val);
        strncpy(variables[idx].value.passVal, val, MAX_STRING_LENGTH - 1);
variables[idx].value.passVal[MAX_STRING_LENGTH - 1] = '\0';

        variables[idx].priority = priority;
        passwordIndex++;
    } else {
        Serial.println("No more space for Passwords!");
    }
}

void DefineBool(const String& varName, bool val, int priority) {
    if (boolIndex < TOTAL_BOOLS) {
        int idx = TOTAL_STRINGS + TOTAL_BYTES + TOTAL_INTS + TOTAL_IPS + TOTAL_PASSWORDS + boolIndex;
        variables[idx].type = DYNAMIC_BOOL;
        variables[idx].name = varName;
        variables[idx].value.byteVal = val ? 1 : 0;
        variables[idx].priority = priority;
        boolIndex++;
    } else {
        Serial.println("No more space for bools!");
    }
}

// -------------------
//    SET FUNCTIONS
// -------------------

bool SetStringValue(const String& varName, const char* newVal) {
    int idx = findVariableIndexByName(varName);
    if (idx == -1) return false;

    if (variables[idx].type == DYNAMIC_STRING) {
        strncpy(variables[idx].value.strVal, newVal, MAX_STRING_LENGTH - 1);
        variables[idx].value.strVal[MAX_STRING_LENGTH - 1] = '\0';
        return true;
    }
    return false;
}

bool SetByteValue(const String& varName, byte newVal) {
    int idx = findVariableIndexByName(varName);
    if (idx == -1) return false;

    if (variables[idx].type == DYNAMIC_BYTE) {
        variables[idx].value.byteVal = newVal;
        return true;
    }
    return false;
}

bool SetIntValue(const String& varName, int newVal) {
    int idx = findVariableIndexByName(varName);
    if (idx == -1) return false;

    if (variables[idx].type == DYNAMIC_INT) {
        variables[idx].value.intVal = newVal;
        return true;
    }
    return false;
}

bool SetIPValue(const String& varName, byte ip1, byte ip2, byte ip3, byte ip4) {
    int idx = findVariableIndexByName(varName);
    if (idx == -1) return false;

    if (variables[idx].type == DYNAMIC_IP) {
        variables[idx].value.ipVal[0] = ip1;
        variables[idx].value.ipVal[1] = ip2;
        variables[idx].value.ipVal[2] = ip3;
        variables[idx].value.ipVal[3] = ip4;
        return true;
    }
    return false;
}

bool SetPasswordValue(const String& varName, const char* newVal) {
    int idx = findVariableIndexByName(varName);
    if (idx == -1) return false;

    if (variables[idx].type == DYNAMIC_PASSWORD) {
        strncpy(variables[idx].value.passVal, newVal, MAX_STRING_LENGTH - 1);
        variables[idx].value.passVal[MAX_STRING_LENGTH - 1] = '\0';
        return true;
    }
    return false;
}

bool SetBoolValue(const String& varName, bool newVal) {
    int idx = findVariableIndexByName(varName);
    if (idx == -1) return false;

    if (variables[idx].type == DYNAMIC_BOOL) {
        variables[idx].value.byteVal = newVal ? 1 : 0;
        return true;
    }
    return false;
}

// -------------------
//    GET FUNCTIONS
// (each returns only the value)
// -------------------

/*
 * If the variable isn't found or has a different type,
 * we return a "safe default":
 *   - "" for strings/password
 *   - 0 for byte/int
 *   - "0.0.0.0" for IP
 */

String GetStringValue(const String& varName) {
    int idx = findVariableIndexByName(varName);
    if (idx == -1) return "";
    if (variables[idx].type == DYNAMIC_STRING && variables[idx].value.strVal != NULL) {
        return String(variables[idx].value.strVal);
    }
    return "";
}

byte GetByteValue(const String& varName) {
    int idx = findVariableIndexByName(varName);
    if (idx == -1) return 0;
    if (variables[idx].type == DYNAMIC_BYTE) {
        return variables[idx].value.byteVal;
    }
    return 0;
}

int GetIntValue(const String& varName) {
    int idx = findVariableIndexByName(varName);
    if (idx == -1) return 0;
    if (variables[idx].type == DYNAMIC_INT) {
        return variables[idx].value.intVal;
    }
    return 0;
}

// Return IP as a dotted string for simplicity
// String GetIPValue(const String& varName) {
//     int idx = findVariableIndexByName(varName);
//     if (idx == -1) return "0.0.0.0";
//     if (variables[idx].type == DYNAMIC_IP) {
//         byte* ip = variables[idx].value.ipVal;
//         return String(ip[0]) + "." + String(ip[1]) + "." +
//                String(ip[2]) + "." + String(ip[3]);
//     }
//     return "0.0.0.0";
// }

byte* GetIPValue(const String& varName) {
    static byte nullIP[4] = {0, 0, 0, 0};  // Safe fallback
    int idx = findVariableIndexByName(varName);
    if (idx == -1 || variables[idx].type != DYNAMIC_IP) {
        return nullIP;
    }
    return variables[idx].value.ipVal;
}

String GetPasswordValue(const String& varName) {
    int idx = findVariableIndexByName(varName);
    if (idx == -1) return "";
    if (variables[idx].type == DYNAMIC_PASSWORD && variables[idx].value.passVal != NULL) {
        return String(variables[idx].value.passVal);
    }
    return "";
}

bool GetBoolValue(const String& varName) {
    int idx = findVariableIndexByName(varName);
    if (idx == -1) return false;
    if (variables[idx].type == DYNAMIC_BOOL) {
        return variables[idx].value.byteVal != 0;
    }
    return false;
}

// ----------------------
//   GET PRIORITY ONLY
// ----------------------
/*
 * If you want to read the priority for a given variable separately,
 * you can use this helper.
 */
int GetPriority(const String& varName) {
    int idx = findVariableIndexByName(varName);
    if (idx == -1) return -1; // not found
    return variables[idx].priority;
}

// ------------------------------
//   PROCESS VARIABLES BY PRIORITY
// ------------------------------

int compareByPriority(const void *a, const void *b) {
    const DynamicVariable *varA = (const DynamicVariable*)a;
    const DynamicVariable *varB = (const DynamicVariable*)b;
    return (varA->priority - varB->priority);
}

void ProcessVariablesByPriority(void (*action)(DynamicVariable &)) {
    for (int targetPriority = 1; targetPriority <= PriorityCounter; targetPriority++) {
        int idx = findVariableIndexByPriority(targetPriority);
        if (idx >= 0 && idx < totalVariables && variables[idx].name.length() > 0) {
            action(variables[idx]);
        }
    }
}

// Example action function to show each variable
void exampleAction(DynamicVariable &var) {
    Serial.print("Processing: ");
    Serial.print(var.name);
    Serial.print(" | Priority: ");
    Serial.print(var.priority);
    Serial.print(" | Value: ");

    switch (var.type) {
        case DYNAMIC_STRING:
            Serial.println(var.value.strVal);
            break;
        case DYNAMIC_BYTE:
            Serial.println(var.value.byteVal);
            break;
        case DYNAMIC_INT:
            Serial.println(var.value.intVal);
            break;
        case DYNAMIC_IP:
            Serial.print(var.value.ipVal[0]); Serial.print(".");
            Serial.print(var.value.ipVal[1]); Serial.print(".");
            Serial.print(var.value.ipVal[2]); Serial.print(".");
            Serial.println(var.value.ipVal[3]);
            break;
        case DYNAMIC_PASSWORD:
            Serial.println(var.value.passVal);
            break;
        case DYNAMIC_BOOL:
            Serial.println(var.value.byteVal ? "true" : "false");
            break;
    }
}

int findVariableIndexByPriority(int targetPriority) {
    for (int i = 0; i < totalVariables; i++) {
        if (variables[i].name.length() > 0 && variables[i].priority == targetPriority) {
            return i;
        }
    }
    return -1;
}
