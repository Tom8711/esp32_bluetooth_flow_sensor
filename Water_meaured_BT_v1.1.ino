// this header is needed for Bluetooth Serial -> works ONLY on ESP32
#include "BluetoothSerial.h" 

// Define pins on ESP32
#define SENSOR  33

// Define Id's of incoming and outgoing bluetooth signals
#define ID_ML_TOTAL 1
#define ID_ML_FLOW 2

// init Class for bluetooth:
BluetoothSerial ESP_BT; 

// Bluetooth send parameters

int id = -1;                                      // Id to specify the incoming signal
int val_byte1 = -1;                               // Value of the first incoming byte
int val_byte2 = -1;                               // Value of the second incoming byte

// Init variables for calculating the flow from sensor input

long currentMillis = 0;                           // Milliseconds counted at start of measuremend loop
long previousMillis = 0;                          // Milliseconds counted after flowrate is calculated with pulsecounter
int interval = 100;                               // Interval of the the time between loops in ms
int calibrationFactor = 90;                       // Factor to be altered to calibrate sensor
volatile byte pulseCount;                         // Amount of pulsecounts read from the sensor
byte pulse1Loop = 0;                              // Amount of pulses during one loop


float flowRate;                                   // Calculated flowrate in ml per looptime
float flowMilliLitres;                            // Calculated flowrate in ml/s
float totalMilliLitres;                           // Total amount of ml



//--------FUNCTIONS---------

// Reset the parameters used to receive BT data
void reset_rx_BT()
{
  id = -1;
  val_byte1 = -1;
  val_byte2 = -1;
}

// Increment pulseCount in attach interupt
void IRAM_ATTR pulseCounter()
{
  pulseCount++;
}

// Send value with given id via bluetooth. Values 240 and up are recognized as ids
// Two bytes are send each can be upt to 239 resulting in possible values up to 57.121
// By adding the 239 floor division of byte 1 with the 239 modulus division of byt2, the value is retrieved
void send_BT(int id, int value)
{
  ESP_BT.write(240 + id);                         // 240 is added to the id. Once received 240 is subtracted again.
  ESP_BT.write(floor(value/239));                 // The first byte (most significant byte) is divided by floor division by 239
  ESP_BT.write(value%239);                        // The second byte (least significant byte) is divided through modulus division by 239
}



//------INITIAL SETUP------

void setup() {
  Serial.begin(115200);                            // Serial output for reading out ESP32
  ESP_BT.begin("DNA Sampler");                     // Name of your Bluetooth interface -> will show up on your phone

  pinMode(SENSOR, INPUT_PULLUP);                   // Pinmode of Sensor is defined input

  // Reset all variables
  pulseCount = 0;
  flowRate = 0.0;
  flowMilliLitres = 0;
  totalMilliLitres = 0;
  previousMillis = 0;

  // Fire pulseCounter method each time the sensor voltage changes (falls)
  attachInterrupt(digitalPinToInterrupt(SENSOR), pulseCounter, FALLING);
}



//-------MAIN LOOP------

void loop() {

  // If incoming BT signal is present
  if (ESP_BT.available())
  {
    incoming = ESP_BT.read();                     // Set received BT data to incoming

    // Check the incoming data for id. Launch the code dependant of the id that is received
    switch (incoming) 
    {
      // Id 1: reset totalMilliLitres
      case 1:
        Serial.println("Total volume reset");
        totalMilliLitres = 0;
    }
  }

  //------MEASUREMENT OF SENSOR AND CALCULATIONS------

  currentMillis = millis();                                                                    // Start time of the measurement. With millis() the amount of miliseconds since the start of the program is given

  // Start the measurement when the time since the last measurement is greater than the specified intervall
  if (currentMillis - previousMillis > interval) 
  {
    pulse1Loop = pulseCount;                                                                   // The pulses measured during the last loop
    pulseCount = 0;                                                                            // Reset pulseCount at the start of the measurement
  
    flowRate = ((1000 / (millis() - previousMillis)) * pulse1Loop) / calibrationFactor;        // Calculate the flowrate by dividing the (time of one loop times the number of pulses) with the determined calibration factor
    previousMillis = millis();                                                                 // Time after the calculation, used to determine if enough time has passed since last loop

    // Divide the flow rate in litres/minute by 60 to determine how many litres have
    // passed through the sensor in this 100 millisecond interval, then multiply by 1000 to
    // convert to millilitres.
    flowMilliLitres = (flowRate / 600) * 1000;

    // Add the millilitres passed in this second to the cumulative total
    totalMilliLitres += flowMilliLitres;

  // -------------------- Send Bluetooth signal ----------------------

      // Sent data with bluetooth
      send_BT(ID_ML_TOTAL, int(totalMilliLitres));
      send_BT(ID_ML_FLOW, int(flowMilliLitres));

      // Print serial data for debug
      Serial.println(String(totalMilliLitres));
      Serial.println(String(flowMilliLitres) + " flowmililiters");
      Serial.println(pulse1Sec);
    

  }
    
}
