#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ArduinoOTA.h>

//------------------------------------------
//DS18B20
#define ONE_WIRE_BUS 2 //Pin to which is attached a temperature sensor
#define ONE_WIRE_MAX_DEV 15 //The maximum number of devices

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);
int numberOfDevices; //Number of temperature devices found
DeviceAddress devAddr[ONE_WIRE_MAX_DEV];  //An array device temperature sensors
float tempDevC[ONE_WIRE_MAX_DEV]; //Saving the last measurement of temperature
float tempDevCLast[ONE_WIRE_MAX_DEV]; //Previous temperature measurement
float tempDevF[ONE_WIRE_MAX_DEV]; //Saving the last measurement of temperature
float tempDevFLast[ONE_WIRE_MAX_DEV]; //Previous temperature measurement
long lastTemp; //The last measurement
const int measureTempFrequency = 5000; //The frequency of temperature measurement

//------------------------------------------
//HTTP
ESP8266WebServer server(80);

//------------------------------------------
//Convert device id to String
String GetAddressToString(DeviceAddress deviceAddress) {
  String str = "";
  for (uint8_t i = 0; i < 8; i++) {
    if ( deviceAddress[i] < 16 ) str += String(0, HEX);
    str += String(deviceAddress[i], HEX);
  }
  return str;
}

//Setting the temperature sensor
void SetupDS18B20() {
  DS18B20.begin();

  Serial.print("Parasite power is: ");
  if ( DS18B20.isParasitePowerMode() ) {
    Serial.println("ON");
  } else {
    Serial.println("OFF");
  }

  numberOfDevices = DS18B20.getDeviceCount();
  Serial.print( "Device count: " );
  Serial.println( numberOfDevices );

  lastTemp = millis();
  DS18B20.requestTemperatures();

  // Loop through each device, print out address
  for (int i = 0; i < numberOfDevices; i++) {
    // Search the wire for address
    if ( DS18B20.getAddress(devAddr[i], i) ) {
      Serial.print("Found device ");
      Serial.print(i, DEC);
      Serial.print(" with address: " + GetAddressToString(devAddr[i]));
      Serial.println();
    } else {
      Serial.print("Found ghost device at ");
      Serial.print(i, DEC);
      Serial.print(" but could not detect address. Check power and cabling");
    }

    //Get resolution of DS18b20
    Serial.print("Resolution: ");
    Serial.print(DS18B20.getResolution( devAddr[i] ));
    Serial.println();

    //Read temperature from DS18b20
    float tempC = DS18B20.getTempC( devAddr[i] );
    Serial.print("Temp C: ");
    Serial.println(tempC);
    float tempF = DS18B20.getTempF( devAddr[i] );
    Serial.print("Temp F: ");
    Serial.println(tempF);
  }
}

String GenerateMetrics() {
  String message = "";

  message += "# HELP beertemp_device_total A count of probe devices connected.\n";
  message += "# TYPE beertemp_device_total gauge\n";
  message += "beertemp_device_total ";
  message += numberOfDevices;
  message += "\n";

  for (int i = 0; i < numberOfDevices; i++) {
    char temperatureCString[6];
    char temperatureFString[6];
    dtostrf(tempDevC[i], 2, 2, temperatureCString);
    dtostrf(tempDevF[i], 2, 2, temperatureFString);

    String idString = "id=\"" + GetAddressToString( devAddr[i] ) + "\" ";

    message += "# HELP beertemp_device_temperature_celsius Current device temperature in celsius.\n";
    message += "# TYPE beertemp_device_temperature_celsius gauge\n";
    message += "beertemp_device_temperature_celsius";
    message += "{";
    message += idString;
    message += "resolution=\"";
    message +=  DS18B20.getResolution( devAddr[i] );
    message += "\"} ";
    message += temperatureCString;
    message += "\n";

    message += "# HELP beertemp_device_temperature_fahrenheit Current device temperature in fahrenheit.\n";
    message += "# TYPE beertemp_device_temperature_fahrenheit gauge\n";
    message += "beertemp_device_temperature_fahrenheit";
    message += "{";
    message += idString;
    message += "resolution=\"";
    message +=  DS18B20.getResolution( devAddr[i] );
    message += "\"} ";
    message += temperatureFString;
    message += "\n";
  }

  return message;
}

void HandleRoot() {
  server.send(200, "text/plain", GenerateMetrics() );
}
void HandleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/html", message);
}

//Loop measuring the temperature
void MeasureTemperature(long now) {
  if ( now - lastTemp > measureTempFrequency ) { //Take a measurement at a fixed interval (durationTemp = 5000ms, 5s)
    for (int i = 0; i < numberOfDevices; i++) {
      // Measure temp
      float tempC = DS18B20.getTempC( devAddr[i] );
      float tempF = DS18B20.getTempF( devAddr[i] );

      // Save the measured value to device array
      tempDevC[i] = tempC;
      tempDevF[i] = tempF;
    }
    DS18B20.setWaitForConversion(false); //No waiting for measurement
    DS18B20.requestTemperatures(); //Initiate the temperature measurement
    lastTemp = millis();  //Remember the last time measurement
  }
}


//------------------------------------------
void setup() {
  Serial.begin(9600);
  Serial.print("Initialized.\n\nConnecting to WiFi SSID ");
  Serial.print(WiFi.SSID());
  Serial.println();

  //Wait for WIFI connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to network ");
  Serial.println(WiFi.SSID());
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC address: ");
  Serial.println(WiFi.macAddress());

  server.on("/", HandleRoot);
  server.on("/metrics", HandleRoot);
  server.onNotFound( HandleNotFound );

  server.begin();
  Serial.println("HTTP server started at ip " + WiFi.localIP().toString() );

  SetupDS18B20();

  ArduinoOTA.setPasswordHash("f9f9c5fd6f84c7b3d2c4fb05cafcfcae");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
}

void loop() {
  long t = millis();
  ArduinoOTA.handle();
  server.handleClient();
  MeasureTemperature(t);
}
