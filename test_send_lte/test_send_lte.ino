#include <SoftwareSerial.h>

// Define the RX and TX pins for the SIM7600 module
SoftwareSerial sim7600(2, 3);
int i=0;

const int ON_OFF_PIN_LTE = 7;

void setup() {
  // Start the Serial USB communication
  Serial.begin(57600);

  // Start the SIM7600 module communication
  sim7600.begin(9600);

  // ON/OFF signal for the sim7600
  pinMode(7, OUTPUT);

  sim7600.flush();
}

void loop() {
  // Define the URL for the GET request
  String url = "https://api.thingspeak.com/update?api_key=H6RF2QIRKQC1XK2P&field1="+String(i);

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
  delay(1000);
  i = i + 1;
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
