#include <Arduino.h>
#if not defined (_VARIANT_ARDUINO_DUE_X_) && not defined (_VARIANT_ARDUINO_ZERO_)
  #include <SoftwareSerial.h>
#endif
//REMOVED SPI ADD IF ERROR
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_UART.h"

#include "BluefruitConfig.h"

/*=========================================================================
    APPLICATION SETTINGS

    FACTORYRESET_ENABLE       Perform a factory reset when running this sketch
   
                              Enabling this will put your Bluefruit LE module
                              in a 'known good' state and clear any config
                              data set in previous sketches or projects, so
                              running this at least once is a good idea.
   
                              When deploying your project, however, you will
                              want to disable factory reset by setting this
                              value to 0.  If you are making changes to your
                              Bluefruit LE device via AT commands, and those
                              changes aren't persisting across resets, this
                              is the reason why.  Factory reset will erase
                              the non-volatile memory where config data is
                              stored, setting it back to factory default
                              values.
       
                              Some sketches that require you to bond to a
                              central device (HID mouse, keyboard, etc.)
                              won't work at all with this feature enabled
                              since the factory reset will clear all of the
                              bonding data stored on the chip, meaning the
                              central device won't be able to reconnect.
    MINIMUM_FIRMWARE_VERSION  Minimum firmware version to have some new features
    MODE_LED_BEHAVIOUR        LED activity, valid options are
                              "DISABLE" or "MODE" or "BLEUART" or
                              "HWUART"  or "SPI"  or "MANUAL"
    -----------------------------------------------------------------------*/
    #define FACTORYRESET_ENABLE         1
    #define MINIMUM_FIRMWARE_VERSION    "0.6.6"
    #define MODE_LED_BEHAVIOUR          "MODE"
/*=========================================================================*/

SoftwareSerial bluefruitSS = SoftwareSerial(BLUEFRUIT_SWUART_TXD_PIN, BLUEFRUIT_SWUART_RXD_PIN);

Adafruit_BluefruitLE_UART ble(bluefruitSS, BLUEFRUIT_UART_MODE_PIN,
	BLUEFRUIT_UART_CTS_PIN, BLUEFRUIT_UART_RTS_PIN);

#include <AFMotor.h>
#include <NewPing.h>

#define TRIGGER_PIN		6
#define ECHO_PIN 		5
#define MAX_DISTANCE	200

NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);

AF_DCMotor rightMotor(3, MOTOR34_64KHZ);
AF_DCMotor leftMotor(4, MOTOR34_64KHZ);

boolean prox;
int distance;

// A small helper
void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}

void setup() {
  // put your setup code here, to run once:
	Serial.begin(115200);
	rightMotor.setSpeed(200);
	leftMotor.setSpeed(200);
	Serial.println(F("Adafruit Bluefruit Command Mode Example"));
	Serial.println(F("---------------------------------------"));

	/* Initialise the module */
	Serial.print(F("Initialising the Bluefruit LE module: "));

	if ( !ble.begin(VERBOSE_MODE) )
	{
		error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
	}
	Serial.println( F("OK!") );

	if ( FACTORYRESET_ENABLE )
	{
		/* Perform a factory reset to make sure everything is in a known state */
		Serial.println(F("Performing a factory reset: "));
		if ( ! ble.factoryReset() ){
		  error(F("Couldn't factory reset"));
		}
	}

	/* Disable command echo from Bluefruit */
	ble.echo(false);

	Serial.println("Requesting Bluefruit info:");
	/* Print Bluefruit information */
	ble.info();

	Serial.println(F("Please use Adafruit Bluefruit LE app to connect in UART mode"));
	Serial.println(F("Then Enter characters to send to Bluefruit"));
	Serial.println();

	//ble.verbose(false);  // debug info is a little annoying after this point!

	/* Wait for connection */
	while (! ble.isConnected()) {
		delay(500);
	}

	// LED Activity command is only supported from 0.6.6
	if ( ble.isVersionAtLeast(MINIMUM_FIRMWARE_VERSION) )
	{
		// Change Mode LED Activity
		Serial.println(F("******************************"));
		Serial.println(F("Change LED activity to " MODE_LED_BEHAVIOUR));
		ble.sendCommandCheckOK("AT+HWModeLED=" MODE_LED_BEHAVIOUR);
		Serial.println(F("******************************"));
	}
}

void loop() {
  // put your main code here, to run repeatedly:
	// Check for incoming characters from Bluefruit
	ble.println("AT+BLEUARTRX");
	ble.readline();
	if (strcmp(ble.buffer, "OK") == 0) {
		// no data
		return;
	}
	// Some data was found, its in the buffer
	sensor_read();
	Serial.print(F("[Recv] ")); Serial.println(ble.buffer);
	String received = String(ble.buffer);
	if (prox == false) {
		rightMotor.run(RELEASE);
		rightMotor.run(RELEASE);
	} else if (received == "forward") {
		rightMotor.run(FORWARD);
		leftMotor.run(FORWARD);
	} else if (received == "backward") {
		rightMotor.run(BACKWARD);
		leftMotor.run(BACKWARD);
	} else if (received == "left") {
		rightMotor.run(FORWARD);
		leftMotor.run(BACKWARD);
		delay(250);
		rightMotor.run(RELEASE);
		leftMotor.run(RELEASE);
	} else if (received == "right") {
		rightMotor.run(BACKWARD);
		leftMotor.run(FORWARD);
		delay(250);
		rightMotor.run(RELEASE);
		leftMotor.run(RELEASE);
	} else if (received == "stop") {
		rightMotor.run(RELEASE);
		leftMotor.run(RELEASE);
	}
	ble.waitForOK();
}

void sensor_read() {
	delay(250);
	distance = sonar.ping_cm();
	Serial.println(distance);
	if (distance > 10 || distance == 0) {
		Serial.println("No Obstruction");
		prox = true;
	} else {
		Serial.println("Obstruction");
		prox = false;
	}
}