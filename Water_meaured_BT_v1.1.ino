// this header is needed for Bluetooth Serial -> works ONLY on ESP32
#include "BluetoothSerial.h" 

#define SENSOR  33
#define ML_TOTAL 1
#define ML_FLOW 2

// init Class:
BluetoothSerial ESP_BT; 

// Parameters for Bluetooth interface
//int incoming;
long currentMillis = 0;
long previousMillis = 0;
int interval = 100;
float calibrationFactor = 90;
volatile byte pulseCount;
byte pulse1Sec = 0;
float flowRate;
float flowMilliLitres;
float totalMilliLitres;

// bluetooth send parameters
int incoming = -1;
int id = -1;
int val_byte1 = -1;
int val_byte2 = -1;

void reset_rx_BT()
{
  id = -1;
  val_byte1 = -1;
  val_byte2 = -1;
}

void IRAM_ATTR pulseCounter()
{
  pulseCount++;
}

void send_BT(int id, int value)
{
  ESP_BT.write(240 + id);
  ESP_BT.write(floor(value/240));
  ESP_BT.write(value%240);
}

void setup() {
  Serial.begin(115200);
  ESP_BT.begin("DNA Sampler"); //Name of your Bluetooth interface -> will show up on your phone

  pinMode(SENSOR, INPUT_PULLUP);

  pulseCount = 0;
  flowRate = 0.0;
  flowMilliLitres = 0;
  totalMilliLitres = 0;
  previousMillis = 0;

  attachInterrupt(digitalPinToInterrupt(SENSOR), pulseCounter, FALLING);
}

void loop() {

  if(ESP_BT.available())
  {
    incoming = ESP_BT.read();

    switch (incoming) 
    {
    case 1:
      Serial.println("Total volume reset");
      totalMilliLitres = 0;
    }
  }

   currentMillis = millis();
  if (currentMillis - previousMillis > interval) 
  {
    pulse1Sec = pulseCount;
    pulseCount = 0;
  
    flowRate = ((1000 / (millis() - previousMillis)) * pulse1Sec) / calibrationFactor;
    previousMillis = millis();

    // Divide the flow rate in litres/minute by 60 to determine how many litres have
    // passed through the sensor in this 1 second interval, then multiply by 1000 to
    // convert to millilitres.
    flowMilliLitres = (flowRate / 60) * 1000;

    // Add the millilitres passed in this second to the cumulative total
    totalMilliLitres += flowMilliLitres;

  // -------------------- Send Bluetooth signal ----------------------

      send_BT(1, int(totalMilliLitres));
      send_BT(2, int(flowMilliLitres));
      Serial.println(String(totalMilliLitres));
      Serial.println(String(flowMilliLitres) + " flowmililiters");
      Serial.println(pulse1Sec);
    

  }
    
}
