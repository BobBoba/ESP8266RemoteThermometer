#include <DHT_U.h>
#include <DHT.h>
#include <TaskScheduler.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include <WiFiServer.h>
#include <WiFiClientSecure.h>
#include <WiFiClient.h>
#include <ESP8266WiFiType.h>
#include <ESP8266WiFiSTA.h>
#include <ESP8266WiFiScan.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WiFiGeneric.h>
#include <ESP8266WiFiAP.h>
#include <ESP8266WiFi.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <vector>


// OneWire DS18S20, DS18B20, DS1822 Temperature Example
//
// http://www.pjrc.com/teensy/td_libs_OneWire.html
//
// The DallasTemperature library can do all this work for you!
// http://milesburton.com/Dallas_Temperature_Control_Library

OneWire  ds(0);  // on pin 10 (a 4.7K resistor is necessary)
int step = 0;

#define DISPLAY_I2C_ADDR 0x27
#define DISPLAY_WIDTH 16
#define DISPLAY_LINES 2

//LiquidCrystal_I2C lcd(0x27, 16, 2); // Устанавливаем дисплей
LiquidCrystal_I2C lcd(DISPLAY_I2C_ADDR, DISPLAY_WIDTH, DISPLAY_LINES); // Устанавливаем дисплей

// WIFI
String mac;
extern const char* WIFI_SSID;
extern const char* WIFI_PWD;


//IPAddress server(192, 168, 0, 10); // mosquitto address
//WiFiClient wclient;
WiFiClientSecure client;


//#define NUM_PROBES 2
//
//byte probe_addresses[NUM_PROBES][8] = {
//	{ 0x28, 0xFF, 0xBC, 0xCA, 0x88, 0x16, 0x3, 0xE7 },
//	{ 0x28, 0xFF, 0x2, 0x11, 0x88, 0x16, 0x3, 0x9F } };

//float probe_celsius_prev[NUM_PROBES] = { 0 };
//float probe_celsius[NUM_PROBES] = { 0 };

struct DS18x20
{
	byte addr[8];
	String saddr;
	float celsius;
};

std::vector<DS18x20> DS18x20s;
std::vector<float> probe_celsius, probe_celsius_prev;
float out_temp, out_hum;

extern char azureHost[];
extern int azurePort;
extern char authSAS[];
extern char deviceName[];
extern char azureUri[];

#define MILLISECONDS_IN_SECOND 1000

Scheduler runner;
Task EverySecondTask(1 * MILLISECONDS_IN_SECOND, TASK_FOREVER, &MainTask, &runner);
Task EveryMinuteTask(60 * MILLISECONDS_IN_SECOND, TASK_FOREVER, &SenderTask, &runner);

DHT dht(2, DHT11);

void setup(void) {
	Serial.begin(74880);
	delay(100);

	Serial.println();
	Serial.println();
	Serial.println("-=[ Start ]=-");
	Serial.println();

	lcd.init();
	lcd.noAutoscroll();
	lcd.backlight();// Включаем подсветку дисплея

	lcd.print("Welcome!!!");

	uint8_t macAddr[6];
	WiFi.macAddress(macAddr);
	//MAC2STR()
	for (int i = 0; i < 6; ++i)
		mac += String(macAddr[i], 16);

	mac.toUpperCase();

	//jsonObj["MAC"] = mac;

	lcd.setCursor(0, 1);
	lcd.print("MAC:");
	lcd.print(mac);

	delay(2000);


	byte degree[8] = {
		B00010,
		B00101,
		B00010,
		B00000,
		B00000,
		B00000,
		B00000,
	};
	lcd.createChar(0, degree);

	lcd.clear();

	lcd.setCursor(0, 0);
	lcd.print("WiFi...");


	WiFi.begin(WIFI_SSID, WIFI_PWD);

	Serial.print("Connecting to WiFi...");
	int counter = 0;
	while (WiFi.status() != WL_CONNECTED) {
		delay(1000);
		Serial.print(".");
		lcd.print(".");
		//display.clear();
		//display.drawString(64, 10, "Connecting to WiFi");
		//display.drawXbm(46, 30, 8, 8, counter % 3 == 0 ? activeSymbole : inactiveSymbole);
		//display.drawXbm(60, 30, 8, 8, counter % 3 == 1 ? activeSymbole : inactiveSymbole);
		//display.drawXbm(74, 30, 8, 8, counter % 3 == 2 ? activeSymbole : inactiveSymbole);
		//display.display();

		counter++;
	}
	Serial.println(" done!!!");
	lcd.print(" OK");


	lcd.setCursor(0, 1);
	lcd.print("Azure...");

	Serial.print("Connecting to ");
	Serial.print(azureHost);
	if (!client.connect(azureHost, azurePort)) {
		Serial.println(" failed");
		lcd.print(" Error");
		delay(1000);
		//return;
	}
	else {
		Serial.println(" done!!!");
		lcd.print(" OK");
		delay(500);
	}


	byte addr[8] = { 0 };
	while (ds.search(addr))
	{
		Serial.print("Found DS18x20 device: ");

		Serial.print("ROM =");
		for (int i = 0; i < 8; i++)
		{
			Serial.write(' ');
			//Serial.print(addr[i], HEX);
			Serial.printf("%02X", addr[i]);
		}

		if (OneWire::crc8(addr, 7) != addr[7])
		{
			Serial.println("CRC is not valid!");
			continue;
		}


		DS18x20 d;
		memcpy(d.addr, addr, 8);

		for (int i = 0; i < 8; ++i)
			d.saddr += String(addr[i], 16);

		d.saddr.toUpperCase();

		DS18x20s.push_back(d);

		probe_celsius.push_back(0.0);
		probe_celsius_prev.push_back(0.0);
		Serial.println();
	}

	dht.begin();

	lcd.setCursor(0, 0);
	lcd.print("T intake exhaust");
	lcd.setCursor(0, 1);
	lcd.print("out temp humidit");
	
	delay(2000);

	//runner.init();

	EverySecondTask.enable();
	EveryMinuteTask.enableDelayed(3 * MILLISECONDS_IN_SECOND);

	lcd.clear();

	//if (client.verify(fingerprint, azureHost)) {
	//	Serial.println("certificate matches");
	//}
	//else {
	//	Serial.println("certificate doesn't match");
	//}

	//wclient.connect()
}

void httpPost(String content)
{
	client.stop(); // закрываем подключение, если вдруг оно открыто
	if (client.connect(azureHost, azurePort)) {
		client.print("POST ");
		client.print(azureUri);
		client.println(" HTTP/1.1");
		client.print("Host: ");
		client.println(azureHost);
		client.print("Authorization: ");
		client.println(authSAS);
		client.println("Connection: close");
		client.print("Content-Type: ");
		client.println("text/plain");
		client.print("Content-Length: ");
		client.println(content.length());
		client.println();
		client.println(content);
		//delay(200);
	}
	else {
		Serial.println("HTTP POST отправка неудачна");
	}
}

void loop(void) {
	runner.execute();
}

JsonObject& prepareJson()
{
	StaticJsonBuffer<1024> jsonBuffer;
	JsonObject& jsonObj = jsonBuffer.createObject();

	jsonObj["MAC"] = mac;

	if (!isnan(out_hum) && !isnan(out_temp)) {
		JsonObject& jsonDTH11 = jsonObj.createNestedObject("DTH11");
		jsonDTH11["temperature"] = out_temp;
		jsonDTH11["humidity"] = out_hum;
	}

	if (DS18x20s.size() > 0)
	{
		JsonArray& jsonDS18x20s = jsonObj.createNestedArray("DS18x20");

		for (int p = 0, pc = DS18x20s.size(); p < pc; ++p)
		{
			JsonObject& jsonDS18x20 = jsonDS18x20s.createNestedObject();
			jsonDS18x20["address"] = DS18x20s[p].saddr;
			jsonDS18x20["temperature"] = DS18x20s[p].celsius;
		}
	}

	//String buffer;
	return jsonObj;// .printTo(buffer);

}

void MainTask()
{
	byte i;
	byte present = 0;
	byte type_s;
	byte data[12];
	float celsius;// , fahrenheit;
	//ROM = 28 FF BC CA 88 16 3 E7
	//ROM = 28 FF 2 11 88 16 3 9F


	Serial.printf("[%d]\n", step++);

	//byte addr[8] = { 0 };
	//int p = 0;
	//while (ds.search(addr)) {
	//++p;

	//JsonArray ja = jsonObj.createNestedArray("OneWire");

	//JsonObject jo = ja.createNestedObject();
	//	Serial.println("No more addresses.");
	//	//Serial.println();
	//	ds.reset_search();
	//	delay(1000);
	//	return;
	//}

	for (int p = 0, pc = DS18x20s.size(); p < pc; ++p)
	{
		byte* addr = DS18x20s[p].addr;

		//String saddr;
		//Serial.print("ROM =");
		//for (i = 0; i < 8; i++) {
		//	Serial.write(' ');
		//	//Serial.print(addr[i], HEX);
		//	Serial.printf("%02X", addr[i]);
		//	saddr += String(addr[i], 16);
		//}

		//if (OneWire::crc8(addr, 7) != addr[7]) {
		//	Serial.println("CRC is not valid!");
		//	continue;
		//}
		//Serial.println();

		// the first ROM byte indicates which chip
		switch (addr[0]) {
		case 0x10:
			Serial.print("  Chip = DS18S20");  // or old DS1820
			type_s = 1;
			break;
		case 0x28:
			Serial.print("  Chip = DS18B20");
			type_s = 0;
			break;
		case 0x22:
			Serial.print("  Chip = DS1822");
			type_s = 0;
			break;
		default:
			Serial.println("Device is not a DS18x20 family device.");
			continue;
		}

		Serial.print("  Temperature = ");

		ds.reset();
		ds.select(addr);
		ds.write(0x44, 1);        // start conversion, with parasite power on at the end

		delay(1000);     // maybe 750ms is enough, maybe not
		// we might do a ds.depower() here, but the reset will take care of it.

		present = ds.reset();
		ds.select(addr);
		ds.write(0xBE);         // Read Scratchpad

		//Serial.print("  Data = ");
		//Serial.print(present, HEX);
		//Serial.print(" ");
		for (i = 0; i < 9; i++) {           // we need 9 bytes
			data[i] = ds.read();
			//Serial.print(data[i], HEX);
			//Serial.print(" ");
		}
		//Serial.print(" CRC=");
		//Serial.print(OneWire::crc8(data, 8), HEX);
		//Serial.println();

		// Convert the data to actual temperature
		// because the result is a 16 bit signed integer, it should
		// be stored to an "int16_t" type, which is always 16 bits
		// even when compiled on a 32 bit processor.
		int16_t raw = (data[1] << 8) | data[0];
		if (type_s) {
			raw = raw << 3; // 9 bit resolution default
			if (data[7] == 0x10) {
				// "count remain" gives full 12 bit resolution
				raw = (raw & 0xFFF0) + 12 - data[6];
			}
		}
		else {
			byte cfg = (data[4] & 0x60);
			// at lower res, the low bits are undefined, so let's zero them
			if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
			else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
			else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
			//// default is 12 bit resolution, 750 ms conversion time
		}
		celsius = (float)raw / 16.0;

		if (p >= probe_celsius.size())
		{
			probe_celsius.push_back(celsius);
			probe_celsius_prev.push_back(0);
		}
		else
			probe_celsius[p] = celsius;

		DS18x20s[p].celsius = celsius;

		//fahrenheit = celsius * 1.8 + 32.0;
		Serial.print(celsius);
		Serial.print(" Celsius");
		//Serial.print(fahrenheit);
		//Serial.println(" Fahrenheit");
		Serial.println();
	}

	if (probe_celsius.size() >= 1)
	{
		lcd.setCursor(0, 0);
		int w = 0;
		w += lcd.print("IE ");
		w += lcd.print(probe_celsius[0], 2);
		w += lcd.write(byte(0)); // degrees symbol
		//w += lcd.print("");

		if (probe_celsius.size() >= 2)
		{
			w += lcd.print(" ");
			w += lcd.print(probe_celsius[1], 2);
			w += lcd.write(byte(0)); // degrees symbol
			//w += lcd.print("");
		}

		for (int i = w; i < DISPLAY_WIDTH; ++i)
			lcd.print(" ");
	}

	// Reading temperature or humidity takes about 250 milliseconds!
	// Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
	out_hum = dht.readHumidity();
	// Read temperature as Celsius (the default)
	out_temp = dht.readTemperature();
	
	lcd.setCursor(0, 1);
	int w = 0;
	w += lcd.print("OUT T:");
	w += lcd.print(out_temp, 0);
	w += lcd.write(byte(0)); // degrees symbol
	w += lcd.print(" H:");
	w += lcd.print(out_hum, 0);
	w += lcd.print("%"); // degrees symbol
	//w += lcd.print("");
	
	for (int i = w; i < DISPLAY_WIDTH; ++i)
		lcd.print(" ");



	// Check if any reads failed and exit early (to try again).
	if (isnan(out_hum) || isnan(out_temp)) {
		Serial.println("Failed to read from DHT sensor!");
	}
	else {
		Serial.print("DTH11 Humidity: ");
		Serial.print(out_hum);
		Serial.print(" %\t");
		Serial.print("Temperature: ");
		Serial.print(out_temp);
		Serial.println(" *C ");
	}


	//char buffer[1024] = { 0 };
	//buffer += "{\"";

	//buffer += "MAC\" : \"";
	//buffer += mac;
	//buffer += "\"";

	//buffer += ", ";

	//buffer += "probe1\" : \"";
	//buffer += String(probe_celsius[0], 2);
	//buffer += "\"";

	//buffer += ", ";

	//buffer += "\"probe2\" : \"";
	//buffer += String(probe_celsius[1], 2);
	//buffer += "\"";

	//buffer += "}";
	//sprintf_s(buffer, "{\"probe1\" : \"%f\", \"probe2\" : \"%f\"}", probe_celsius[0], probe_celsius[1]);
	Serial.println("json data: ");
	JsonObject& json = prepareJson();
	String buffer;
	json.prettyPrintTo(buffer);
	Serial.println(buffer.c_str());

	//delay(2000);
	//return;
}

void SenderTask()
{
	//TODO: optimization > don't sent very same values
	bool needUpdate = false;
	for (int p = 0, pc = probe_celsius.size(); p < pc; ++p)
	{
		if (probe_celsius_prev[p] != probe_celsius[p])
		{
			needUpdate = true;
			break;
		}
	}

	if (needUpdate)
	{
		JsonObject& json = prepareJson();

		String buffer;
		json.printTo(buffer);

		Serial.println("Send updated data to IoT Hub");

		httpPost(buffer);

		String response = "";
		char c;
		while (client.available()) {
			c = client.read();
			response.concat(c);
		}
		if (response.equals(""))
		{
			Serial.println("empty response");
		}
		else
		{
			if (response.startsWith("HTTP/1.1 204")) {
				Serial.println("String has been successfully send to Azure IoT Hub");
			}
			else {
				Serial.println("Error");
				Serial.println(response);
			}
		}

		for (int p = 0, pc = probe_celsius.size(); p < pc; ++p)
		{
			probe_celsius_prev[p] = probe_celsius[p];
		}
		//probe_celsius_prev[1] = probe_celsius[1];
		//delay(60 * 1000); // sleep one minute
	}

}
