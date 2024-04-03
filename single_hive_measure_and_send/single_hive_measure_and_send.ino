#include <HX711_ADC.h>
#if defined(ESP8266)|| defined(ESP32) || defined(AVR)
#include <EEPROM.h>
#endif
#include <SoftwareSerial.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is plugged into pin 2 on the Arduino
#define ONE_WIRE_BUS 10
// Setup a OneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);
// Pass OneWire reference to Dallas Temperature
DallasTemperature sensors(&oneWire);

//pins:
const int HX711_dout = 4; //mcu > HX711 dout pin
const int HX711_sck = 5; //mcu > HX711 sck pin

//HX711 constructor:
HX711_ADC LoadCell(HX711_dout, HX711_sck);

const int calVal_eepromAdress = 0;
unsigned long t = 0;
const bool useLTE = true;

// Define the RX and TX pins for the SIM7600 module
SoftwareSerial sim7600(2, 3);
const int ON_OFF_PIN_LTE = 7;
int i=0;

void setup() {
  // Start the Serial USB communication
  Serial.begin(57600);
  // Start the SIM7600 module communication
  sim7600.begin(9600);
  // ON/OFF signal for the sim7600
  pinMode(7, OUTPUT);
  sim7600.flush();
  Serial.println();
  Serial.println("Starting...");
  LoadCell.begin();
  float calibrationValue; // calibration value (see example file "Calibration.ino")
  //calibrationValue = 887.24; // uncomment this if you want to set the calibration value in the sketch
  EEPROM.get(calVal_eepromAdress, calibrationValue); // uncomment this if you want to fetch the calibration value from eeprom
  unsigned long stabilizingtime = 2000; // preciscion right after power-up can be improved by adding a few seconds of stabilizing time
  boolean _tare = true; //set this to false if you don't want tare to be performed in the next step
  LoadCell.start(stabilizingtime, _tare);
  if (LoadCell.getTareTimeoutFlag()) {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    while (1);
  }
  else {
    LoadCell.setCalFactor(calibrationValue); // set calibration value (float)
    Serial.println("Startup is complete");
  }
  sensors.begin(); // Start up the library

  // Turn off sim7600
  //lte_power_off();

  delay(300);
}

void loop() {

  //--------------Measurements---------------------------
  int cur_weight = -1;
  float cur_temp= -100;
  bool newDataReady = false;
  int count_measurements = 0;
  t = millis();
  //getting X new data points (have to move out the moving average)
  Serial.println("Getting current weight...");
  while (count_measurements < 20)
  {
    // check for new data/start next conversion:
    if (LoadCell.update()) newDataReady = true;

    // get smoothed value from the dataset:
    if (newDataReady) {
      cur_weight = LoadCell.getData();
      //Serial.print("Load_cell output val: ");
      //Serial.println(cur_weight);
      newDataReady = 0;
      count_measurements++;
    }
    delay(200);
  }

  // get the last measurment used to save

  // check for new data/start next conversion:
  if (LoadCell.update()) newDataReady = true;

  // get smoothed value from the dataset:
  if (newDataReady) {
    cur_weight = LoadCell.getData();
    Serial.print("Load_cell output val: ");
    Serial.println(cur_weight);
    newDataReady = 0;
  }

  // call sensors.requestTemperatures() to issue a global temperature
  // request to all devices on the bus
  // Send the command to get temperature readings
  sensors.requestTemperatures();
  Serial.println("Temperature is: " + String(sensors.getTempCByIndex(0)) + "°C");
  cur_temp = sensors.getTempCByIndex(0);

  if(useLTE)
  {
    //--------------LTE communication----------------------

    // Define the URL for the GET request
    //String url = "https://api.thingspeak.com/update?api_key=H6RF2QIRKQC1XK2P&field1="+String(cur_weight);
    String url = "https://api.thingspeak.com/update?api_key=H6RF2QIRKQC1XK2P&&field1="+String(cur_weight)+"&field2="+String(cur_temp);
    // Turn on sim7600
    lte_power_on();
    //sendCommand("ATE0");
    // Send the AT commands to start the HTTP connection
    sendCommand("AT+CGATT=1"); // Attach to GPRS
    sendCommand("AT+CGDCONT=1,\"IP\",\"pinternet.interkom.de\""); // Set the APN
    sendCommand("AT+CGACT=1,1"); // Activate PDP context
    sendCommand("AT+HTTPINIT"); // Initialize HTTP service

    // Send the GET request
    sendCommand("AT+HTTPPARA=\"URL\",\"" + url + "\"");
    sendCommand("AT+HTTPACTION=0"); // Send GET request
    delay(5000);

    // End the HTTP connection and PDP context
    sendCommand("AT+HTTPTERM");
    sendCommand("AT+CGACT=0,1");

    // Turn off sim7600
    lte_power_off();
    Serial.println("LTE data sent");
    unsigned long time_diff = 600000 - (millis() - t);
    delay(time_diff);
    i = i + 1;

    }
  else 
  {Serial.println("Not sending LTE data");}
}





String sendCommand(String command) {
  // Send the AT command to the SIM7600
  sim7600.println(command);
  delay(500);
  // Read and print the response from the SIM7600
  String response = "";
  while (sim7600.available()) {
    char c = sim7600.read();
    response += c;
  }
  Serial.print(">>" + command + ": ");
  Serial.println(response);
  return response;
}

bool lte_power_on() {
  Serial.println("Starting LTE module...");

  if(sendCommand("AT") == "")
  {
    digitalWrite(ON_OFF_PIN_LTE, HIGH);
    delay(2000);
    digitalWrite(ON_OFF_PIN_LTE, LOW);
    delay(25000);    
  }
  else
  {
    Serial.println("LTE module seems to be already on");
  }

  if(sendCommand("AT").indexOf("OK") > 0)
  {
    Serial.println("LTE module running");
    return true;
  }else
  {
    Serial.println("LTE module startup error");
    return false;
  }

}

void lte_power_off() {
  Serial.println("Shut down LTE module...");
  digitalWrite(ON_OFF_PIN_LTE, HIGH);
  delay(3000);
  digitalWrite(ON_OFF_PIN_LTE, LOW);
  delay(25000);    
}
