// Running DS temperature sensor(s) and relay(s) on one mysensor arduino node
// Combines Onewire and Relay code
// 2014-10-14 Pego: Tested and Running on Uno/Clone and MQTT gateway

// Example sketch showing how to send in OneWire temperature readings
// Example sketch showing how to control physical relays. 
// This example will remember relay state even after power failure.

#include <MySensor.h>  
#include <SPI.h>
#include <DallasTemperature.h>
#include <OneWire.h>

#define ONE_WIRE_BUS 3 // Pin where dallase sensor is connected 
#define MAX_ATTACHED_DS18B20 16

#define RELAY_1  4  // Arduino Digital I/O pin number for first relay (second on pin+1 etc)
#define NUMBER_OF_RELAYS 2 // Total number of attached relays
#define RELAY_ON 1  // GPIO value to write to turn on attached relay
#define RELAY_OFF 0 // GPIO value to write to turn off attached relay

unsigned long SLEEP_TIME = 30000; // Sleep time between reads (in milliseconds) 30000 orig
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
MySensor gw;
float lastTemperature[MAX_ATTACHED_DS18B20];
int numSensors=0;
boolean receivedConfig = false;
boolean metric = true; 
// Initialize temperature message
MyMessage msg(0,V_TEMP);

void setup()  
{ 
  // Startup OneWire 
  sensors.begin();

  // Startup and initialize MySensors library. Set callback for incoming messages. 
  //gw.begin(); 
  gw.begin(incomingMessage, AUTO, true);

  // Send the sketch version information to the gateway and Controller
  gw.sendSketchInfo("Temp and Relays", "1.0");

  // Fetch the number of attached temperature sensors  
  numSensors = sensors.getDeviceCount();

  // Present all sensors to controller
  for (int i=0; i<numSensors && i<MAX_ATTACHED_DS18B20; i++) {   
     gw.present(i, V_TEMP);
  }

  // Fetch relay status
  for (int sensor=1, pin=RELAY_1; sensor<=NUMBER_OF_RELAYS;sensor++, pin++) {
    // Register all sensors to gw (they will be created as child devices)
    gw.present(sensor, S_LIGHT);
    // Then set relay pins in output mode
    pinMode(pin, OUTPUT);   
    // Set relay to last known state (using eeprom storage) 
    digitalWrite(pin, gw.loadState(sensor)?RELAY_ON:RELAY_OFF);
  }

}


void loop()     
{     
  // Process incoming messages (like config from server)
  gw.process(); 

  // Fetch temperatures from Dallas sensors
  sensors.requestTemperatures(); 

  // Read temperatures and send them to controller 
  for (int i=0; i<numSensors && i<MAX_ATTACHED_DS18B20; i++) {
 
    // Fetch and round temperature to one decimal
    float temperature = static_cast<float>(static_cast<int>((gw.getConfig().isMetric?sensors.getTempCByIndex(i):sensors.getTempFByIndex(i)) * 10.)) / 10.;
 
    // Only send data if temperature has changed more then 1 degC and no error
    if (int(lastTemperature[i]) != int(temperature) && temperature != -127.00) { //added integer
 
      // Send in the new temperature
      gw.send(msg.setSensor(i).set(temperature,1));
      lastTemperature[i]=temperature;
    }
  }
  //gw.sleep(SLEEP_TIME); //no sleep for relays!!!!
}

void incomingMessage(const MyMessage &message) {
  // We only expect one type of message from controller. But we better check anyway.
  if (message.type==V_LIGHT) {
     // Change relay state
     digitalWrite(message.sensor-1+RELAY_1, message.getBool()?RELAY_ON:RELAY_OFF);
     // Store state in eeprom
     gw.saveState(message.sensor, message.getBool());
     // Write some debug info
     Serial.print("Incoming change for sensor:");
     Serial.print(message.sensor);
     Serial.print(", New status: ");
     Serial.println(message.getBool());
   } 
}

