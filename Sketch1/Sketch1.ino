#include <WiFiUdp.h>
#include <WiFiServer.h>
#include <WiFiClient.h>
#include <WiFi.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>

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


void setup(void) {
	Serial.begin(74880);
	delay(100);
	Serial.println();
	Serial.println("-=[ Start ]=-");
	Serial.println();

	lcd.init();
	lcd.noAutoscroll();
	lcd.backlight();// Включаем подсветку дисплея

	lcd.print("Welcome!!!");

	delay(10);


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

}

#define NUM_PROBES 2

byte probe_addresses[NUM_PROBES][8] = {
	{ 0x28, 0xFF, 0xBC, 0xCA, 0x88, 0x16, 0x3, 0xE7 }, 
	{ 0x28, 0xFF, 0x2, 0x11, 0x88, 0x16, 0x3, 0x9F } };

float probe_celsius[NUM_PROBES] = { 0 };

void loop(void) {
	byte i;
	byte present = 0;
	byte type_s;
	byte data[12];
	float celsius, fahrenheit;
	//ROM = 28 FF BC CA 88 16 3 E7
	//ROM = 28 FF 2 11 88 16 3 9F

	Serial.printf("[%d]\n", step++);

	//if (!ds.search(addr)) {
	//	Serial.println("No more addresses.");
	//	//Serial.println();
	//	ds.reset_search();
	//	delay(1000);
	//	return;
	//}

	for (int p = 0; p < NUM_PROBES; ++p)
	{
		byte* addr = probe_addresses[p];

		Serial.print("ROM =");
		for (i = 0; i < 8; i++) {
			Serial.write(' ');
			//Serial.print(addr[i], HEX);
			Serial.printf("%02X", addr[i]);
		}

		if (OneWire::crc8(addr, 7) != addr[7]) {
			Serial.println("CRC is not valid!");
			return;
		}
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
			return;
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
		probe_celsius[p] = celsius;
		fahrenheit = celsius * 1.8 + 32.0;
		Serial.print(celsius);
		Serial.print(" Celsius");
		//Serial.print(fahrenheit);
		//Serial.println(" Fahrenheit");
		Serial.println();
	}

	lcd.setCursor(0, 0);
	int w = 0;
	w += lcd.print("T1: ");
	w += lcd.print(probe_celsius[0], 2);
	w += lcd.write(byte(0)); // degrees symbol
	w += lcd.print("");

	for (int i = w; i < DISPLAY_WIDTH; ++i)
		lcd.print(" ");

	lcd.setCursor(0, 1);
	w += lcd.print("T2: ");
	w += lcd.print(probe_celsius[1], 2);
	w += lcd.write(byte(0)); // degrees symbol
	w += lcd.print("");

	for (int i = w; i < DISPLAY_WIDTH; ++i)
		lcd.print(" ");
}