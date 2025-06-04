#define DMX_MAX 10 // max. number of DMX data packages.
int dimvalue = 0;
int state = 0; //DMX
uint8_t DMXBuffer[DMX_MAX];

void DMXstart()
 {
 //Controllino_RS485Init();  // DMX is a 250000 baud serial data
 char Controllino_RS485Init( void ); //26-07-217 - Uitzoeken!! Deze regel toegevoegd en vorige uitgezet!
 Serial3.begin(25000);
 PORTJ = PORTJ & B10011111;
 PORTJ = PORTJ | B01000000;
 for (int n = 0; n < DMX_MAX; n++)   // initialize the DMX buffer
 DMXBuffer[n] = 0;
 }
 
void DMXwrite(int channel, uint8_t value)
 {
 if (channel < 1) channel = 1;               // press parameters to allowed range
 if (channel > DMX_MAX) channel = DMX_MAX; 
 if (value < 0)   value = 0;
 if (value > 255) value = 255;
 DMXBuffer[channel-1] = value;               // store value for later sending
 } 

void DMXflush()
 {
 Serial3.begin(125000);                       // send the break by sending a slow 0 byte
 Serial3.write((uint8_t)0);
 Serial3.begin(250000);                       // now back to DMX speed: 250000baud
 Serial3.write((uint8_t)0);                   // write start code
 Serial3.write(DMXBuffer, sizeof(DMXBuffer)); // write all the values from the array
 //Serial.print("DMX Output 0: "); Serial.println(DMXBuffer[0]);
 //Serial.print("DMX Output 1: "); Serial.println(DMXBuffer[1]);
 //Serial.print("DMX Output 2: "); Serial.println(DMXBuffer[2]);
 //Serial.print("DMX Output 3: "); Serial.println(DMXBuffer[3]);
 //Serial.print("DMX Output 4: "); Serial.println(DMXBuffer[4]);
 //Serial.print("DMX Output 5: "); Serial.println(DMXBuffer[5]);
 } 
