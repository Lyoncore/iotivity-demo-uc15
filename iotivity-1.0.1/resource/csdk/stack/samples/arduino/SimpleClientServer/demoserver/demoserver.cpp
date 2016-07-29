//******************************************************************
//
// Copyright 2014 Intel Mobile Communications GmbH All Rights Reserved.
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// Do not remove the include below
#include "Arduino.h"

#include "logger.h"
#include "ocstack.h"
#include "ocpayload.h"
#include <string.h>

#ifdef ARDUINOWIFI
// Arduino WiFi Shield
#include <SPI.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#else
// Arduino Ethernet Shield
#include <EthernetServer.h>
#include <Ethernet.h>
#include <Dns.h>
#include <EthernetClient.h>
#include <util.h>
#include <EthernetUdp.h>
#include <Dhcp.h>
#endif

#include <Wire.h>
#include <rgb_lcd.h>

const char *getResult(OCStackResult result);

#define TAG "ArduinoServer"

int gLightUnderObservation = 0;
void createDemoResource();

/* Structure to represent a sensor resource */
typedef struct SENSORRESOURCE{
	OCResourceHandle handle;
	double temp;
	int light;
	int sound;
} SensorResource;

static SensorResource sensor;

/* Structure to represent a LED resource */
typedef struct LEDRESOURCE{
	OCResourceHandle handle;
	int status;
} LedResource;

static LedResource led;

/* Structure to represent a LCD resource */
typedef struct LCDRESOURCE{
	OCResourceHandle handle;
	char *str;
	int red;
	int green;
	int blue;
} LcdResource;

static LcdResource lcd;

/* Structure to represent a buzzer resource */
typedef struct BUZZERRESOURCE{
	OCResourceHandle handle;
	int tone;
} BuzzerResource;

static BuzzerResource buzzer;

/* Structure to represent a button resource */
typedef struct BUTTONRESOURCE{
	OCResourceHandle handle;
	int observer;
	bool button;
	bool touch;
	bool button_old;
	bool touch_old;
} ButtonResource;

static ButtonResource button;


/* Pin configuration */
const int pinTemp = A0;
const int B = 3950;

const int pinLight = A2;
const int pinSound = A1;

const int pinLed    = 7;

rgb_lcd grove_lcd;

int pinBuzzer = 2;

int pinButton = 3;

int pinTouch = 8;

int pinSerno = 7;


void sensor_init()
{
	pinMode(pinTemp, INPUT);
}

void sensor_get()
{
	// Get the (raw) value of the temperature sensor.
	int val = analogRead(pinTemp);

	// Determine the current resistance of the thermistor based on the sensor value.
	float resistance = (float)(1023-val)*10000/val;

	// Calculate the temperature based on the resistance value.
	float temperature = 1/(log(resistance/10000)/B+1/298.15)-273.15;
	// Print the temperature to the serial console.
	sensor.temp = (double)temperature;
	OC_LOG_V(INFO, TAG, "Temperature: %f", sensor.temp);

	sensor.light = analogRead(pinLight);
	OC_LOG_V(INFO, TAG, "Light: %d", sensor.light);
	
	sensor.sound = analogRead(pinSound);
	OC_LOG_V(INFO, TAG, "Sound: %d", sensor.sound);

	return;	
}

void led_init()
{
	pinMode(pinLed, OUTPUT);
	analogWrite(pinLed, led.status);
}

void led_put()
{
	analogWrite(pinLed, led.status);
}

void lcd_print(char *str)
{
	int len1 = strlen(str);
	int len2 = 0;
	char line1[17] = "                ";
	char line2[17] = "                ";

	if(len1 > 32)
		len2 = 16;
	else if(len1 > 16)
		len2 = len1 - 16;

	strncpy(line1, str, len1);
	grove_lcd.setCursor(0, 0);
	grove_lcd.print(line1);

	if(len2) {
		strncpy(line2, str+16, len2);
	}
	grove_lcd.setCursor(0, 1);
	grove_lcd.print(line2);
	
}

void lcd_init()
{
	IPAddress ip = Ethernet.localIP();
	char line[16];

	lcd.str = (char *)malloc(32);
	strcpy(lcd.str, "IP Address      ");
	sprintf(line, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
	strcat(lcd.str, line);


	// set up the LCD's number of columns and rows:
	grove_lcd.begin(16, 2); 
	grove_lcd.setRGB(0, 255, 0);

	lcd_print(lcd.str);
}

void lcd_put()
{
	lcd_print(lcd.str);
}

void buzzer_init()
{
	pinMode(pinBuzzer, OUTPUT);
	digitalWrite(pinBuzzer, LOW);
}

void buzzer_put()
{
	digitalWrite(pinBuzzer, HIGH);
	delayMicroseconds(buzzer.tone);
	delay(1000);
	digitalWrite(pinBuzzer, LOW);
	delayMicroseconds(buzzer.tone);
}

void button_init()
{
	pinMode(pinButton, INPUT);
	pinMode(pinTouch, INPUT);
}

void button_get()
{
	button.button = digitalRead(pinButton);
	button.touch = digitalRead(pinTouch);
}

#ifdef ARDUINOWIFI
// Arduino WiFi Shield
// Note : Arduino WiFi Shield currently does NOT support multicast and therefore
// this server will NOT be listening on 224.0.1.187 multicast address.

static const char ARDUINO_WIFI_SHIELD_UDP_FW_VER[] = "1.1.0";

/// WiFi Shield firmware with Intel patches
static const char INTEL_WIFI_SHIELD_FW_VER[] = "1.2.0";

/// WiFi network info and credentials
char ssid[] = "Canonical-2.4GHz-g";
char pass[] = "adroitreliable";

int ConnectToNetwork()
{
	char *fwVersion;
	int status = WL_IDLE_STATUS;
	// check for the presence of the shield:
	if (WiFi.status() == WL_NO_SHIELD) {
		OC_LOG(ERROR, TAG, ("WiFi shield not present"));
		return -1;
	}

	// Verify that WiFi Shield is running the firmware with all UDP fixes
	fwVersion = WiFi.firmwareVersion();
	OC_LOG_V(INFO, TAG, "WiFi Shield Firmware version %s", fwVersion);
	if ( strncmp(fwVersion, ARDUINO_WIFI_SHIELD_UDP_FW_VER, sizeof(ARDUINO_WIFI_SHIELD_UDP_FW_VER)) !=0 ) {
		OC_LOG(DEBUG, TAG, ("!!!!! Upgrade WiFi Shield Firmware version !!!!!!"));
		return -1;
	}

	// attempt to connect to Wifi network:
	while (status != WL_CONNECTED) {
		OC_LOG_V(INFO, TAG, "Attempting to connect to SSID: %s", ssid);
		status = WiFi.begin(ssid,pass);

		// wait 10 seconds for connection:
		delay(10000);
	}
	OC_LOG(DEBUG, TAG, ("Connected to wifi"));

	IPAddress ip = WiFi.localIP();
	OC_LOG_V(INFO, TAG, "IP Address:  %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
	return 0;
}
#else
// Arduino Ethernet Shield
int ConnectToNetwork()
{
	// Note: ****Update the MAC address here with your shield's MAC address****
	uint8_t ETHERNET_MAC[] = {0x90, 0xA2, 0xDA, 0x0E, 0xC4, 0x05};
	uint8_t error = Ethernet.begin(ETHERNET_MAC);
	if (error  == 0) {
		OC_LOG_V(ERROR, TAG, "error is: %d", error);
		return -1;
	}

	IPAddress ip = Ethernet.localIP();
	OC_LOG_V(INFO, TAG, "IP Address:  %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);

	return 0;
}
#endif //ARDUINOWIFI


// This is the entity handler for the registered resource.
// This is invoked by OCStack whenever it recevies a request for this resource.
OCEntityHandlerResult SensorOCEntityHandlerCb(OCEntityHandlerFlag flag, OCEntityHandlerRequest * entityHandlerRequest,
                                        void *callbackParam)
{
	OCEntityHandlerResult ehRet = OC_EH_OK;
	OCEntityHandlerResponse response = {0};
	OCRepPayload* payload = OCRepPayloadCreate();
	if(!payload) {
		OC_LOG(ERROR, TAG, ("Failed to allocate Payload"));
		return OC_EH_ERROR;
	}

	if(entityHandlerRequest && (flag & OC_REQUEST_FLAG)) {
		OC_LOG (INFO, TAG, ("Flag includes OC_REQUEST_FLAG"));

		if(OC_REST_GET == entityHandlerRequest->method) {
			//sensor_get();
			OCRepPayloadSetUri(payload, "/grove/sensor");
			OCRepPayloadSetPropDouble(payload, "temperature", sensor.temp);
			OCRepPayloadSetPropInt(payload, "light", sensor.light);
			OCRepPayloadSetPropInt(payload, "sound", sensor.sound);
		} else if(OC_REST_PUT == entityHandlerRequest->method) {
			OC_LOG(ERROR, TAG, ("Un-supported request for Sensor: PUT"));
		}

		if (ehRet == OC_EH_OK) {
			// Format the response.  Note this requires some info about the request
			response.requestHandle = entityHandlerRequest->requestHandle;
			response.resourceHandle = entityHandlerRequest->resource;
			response.ehResult = ehRet;
			response.payload = (OCPayload*) payload;
			response.numSendVendorSpecificHeaderOptions = 0;
			memset(response.sendVendorSpecificHeaderOptions, 0, sizeof response.sendVendorSpecificHeaderOptions);
			memset(response.resourceUri, 0, sizeof response.resourceUri);
			// Indicate that response is NOT in a persistent buffer
			response.persistentBufferFlag = 0;

			// Send the response
			if (OCDoResponse(&response) != OC_STACK_OK) {
				OC_LOG(ERROR, TAG, "Error sending response");
				ehRet = OC_EH_ERROR;
			}
		}
	}

	if (entityHandlerRequest && (flag & OC_OBSERVE_FLAG)) {
		if (OC_OBSERVE_REGISTER == entityHandlerRequest->obsInfo.action) {
			OC_LOG (INFO, TAG, ("Received OC_OBSERVE_REGISTER from client"));
			gLightUnderObservation = 1;
		} else if (OC_OBSERVE_DEREGISTER == entityHandlerRequest->obsInfo.action) {
			OC_LOG (INFO, TAG, ("Received OC_OBSERVE_DEREGISTER from client"));
			gLightUnderObservation = 0;
		}
	}
	OCRepPayloadDestroy(payload);
	return ehRet;
}

OCEntityHandlerResult LedOCEntityHandlerCb(OCEntityHandlerFlag flag, OCEntityHandlerRequest * entityHandlerRequest,
                                        void *callbackParam)
{
	OCEntityHandlerResult ehRet = OC_EH_OK;
	OCEntityHandlerResponse response = {0};
	OCRepPayload* payload = OCRepPayloadCreate();
	if(!payload) {
		OC_LOG(ERROR, TAG, ("Failed to allocate Payload"));
		return OC_EH_ERROR;
	}

	if(entityHandlerRequest && (flag & OC_REQUEST_FLAG)) {
		OC_LOG (INFO, TAG, ("Flag includes OC_REQUEST_FLAG"));

		if(OC_REST_GET == entityHandlerRequest->method) {
			OCRepPayloadSetUri(payload, "/grove/led");
			OCRepPayloadSetPropInt(payload, "status", led.status);
		} else if(OC_REST_PUT == entityHandlerRequest->method) {
			int64_t status;
			OCRepPayload *rep = (OCRepPayload *)entityHandlerRequest->payload;
			OC_LOG(INFO, TAG, ("PUT request"));
			OCRepPayloadGetPropInt(rep, "status", &status);
			if(status > 255)
				status = 255;
			else if(status < 0)
				status = 0;
			led.status = (int)status;
			led_put();
			OCRepPayloadSetPropInt(payload, "status", led.status);
		}

		if (ehRet == OC_EH_OK) {
			// Format the response.  Note this requires some info about the request
			response.requestHandle = entityHandlerRequest->requestHandle;
			response.resourceHandle = entityHandlerRequest->resource;
			response.ehResult = ehRet;
			response.payload = (OCPayload*) payload;
			response.numSendVendorSpecificHeaderOptions = 0;
			memset(response.sendVendorSpecificHeaderOptions, 0, sizeof response.sendVendorSpecificHeaderOptions);
			memset(response.resourceUri, 0, sizeof response.resourceUri);
			// Indicate that response is NOT in a persistent buffer
			response.persistentBufferFlag = 0;

			// Send the response
			if (OCDoResponse(&response) != OC_STACK_OK) {
				OC_LOG(ERROR, TAG, "Error sending response");
				ehRet = OC_EH_ERROR;
			}
		}
	}

	if (entityHandlerRequest && (flag & OC_OBSERVE_FLAG)) {
		if (OC_OBSERVE_REGISTER == entityHandlerRequest->obsInfo.action) {
			OC_LOG (INFO, TAG, ("Received OC_OBSERVE_REGISTER from client"));
			gLightUnderObservation = 1;
		} else if (OC_OBSERVE_DEREGISTER == entityHandlerRequest->obsInfo.action) {
			OC_LOG (INFO, TAG, ("Received OC_OBSERVE_DEREGISTER from client"));
			gLightUnderObservation = 0;
		}
	}
	OCRepPayloadDestroy(payload);
	return ehRet;
}

OCEntityHandlerResult LcdOCEntityHandlerCb(OCEntityHandlerFlag flag, OCEntityHandlerRequest * entityHandlerRequest,
                                        void *callbackParam)
{
	OCEntityHandlerResult ehRet = OC_EH_OK;
	OCEntityHandlerResponse response = {0};
	OCRepPayload* payload = OCRepPayloadCreate();
	if(!payload) {
		OC_LOG(ERROR, TAG, ("Failed to allocate Payload"));
		return OC_EH_ERROR;
	}

	if(entityHandlerRequest && (flag & OC_REQUEST_FLAG)) {
		OC_LOG (INFO, TAG, ("Flag includes OC_REQUEST_FLAG"));

		if(OC_REST_GET == entityHandlerRequest->method) {
			OCRepPayloadSetUri(payload, "/grove/lcd");
			OCRepPayloadSetPropString(payload, "lcd", (const char *)lcd.str);
		} else if(OC_REST_PUT == entityHandlerRequest->method) {
			OC_LOG(INFO, TAG, ("PUT request"));
			OCRepPayload *rep = (OCRepPayload *)entityHandlerRequest->payload;
			OCRepPayloadGetPropString(rep, "lcd", &lcd.str);
			OC_LOG_V(INFO, TAG, "LCD string: %s", lcd.str);
			lcd_put();
			OCRepPayloadSetPropString(payload, "lcd", (const char *)lcd.str);
		}

		if (ehRet == OC_EH_OK) {
			// Format the response.  Note this requires some info about the request
			response.requestHandle = entityHandlerRequest->requestHandle;
			response.resourceHandle = entityHandlerRequest->resource;
			response.ehResult = ehRet;
			response.payload = (OCPayload*) payload;
			response.numSendVendorSpecificHeaderOptions = 0;
			memset(response.sendVendorSpecificHeaderOptions, 0, sizeof response.sendVendorSpecificHeaderOptions);
			memset(response.resourceUri, 0, sizeof response.resourceUri);
			// Indicate that response is NOT in a persistent buffer
			response.persistentBufferFlag = 0;

			// Send the response
			if (OCDoResponse(&response) != OC_STACK_OK) {
				OC_LOG(ERROR, TAG, "Error sending response");
				ehRet = OC_EH_ERROR;
			}
		}
	}

	if (entityHandlerRequest && (flag & OC_OBSERVE_FLAG)) {
		if (OC_OBSERVE_REGISTER == entityHandlerRequest->obsInfo.action) {
			OC_LOG (INFO, TAG, ("Received OC_OBSERVE_REGISTER from client"));
			gLightUnderObservation = 1;
		} else if (OC_OBSERVE_DEREGISTER == entityHandlerRequest->obsInfo.action) {
			OC_LOG (INFO, TAG, ("Received OC_OBSERVE_DEREGISTER from client"));
			gLightUnderObservation = 0;
		}
	}
	OCRepPayloadDestroy(payload);
	return ehRet;
}

OCEntityHandlerResult BuzzerOCEntityHandlerCb(OCEntityHandlerFlag flag, OCEntityHandlerRequest * entityHandlerRequest,
                                        void *callbackParam)
{
	OCEntityHandlerResult ehRet = OC_EH_OK;
	OCEntityHandlerResponse response = {0};
	OCRepPayload* payload = OCRepPayloadCreate();
	if(!payload) {
		OC_LOG(ERROR, TAG, ("Failed to allocate Payload"));
		return OC_EH_ERROR;
	}

	if(entityHandlerRequest && (flag & OC_REQUEST_FLAG)) {
		OC_LOG (INFO, TAG, ("Flag includes OC_REQUEST_FLAG"));

		if(OC_REST_GET == entityHandlerRequest->method) {
			OCRepPayloadSetUri(payload, "/grove/buzzer");
			OCRepPayloadSetPropInt(payload, "tone", buzzer.tone);
		} else if(OC_REST_PUT == entityHandlerRequest->method) {
			int64_t tone;
			OC_LOG(INFO, TAG, ("PUT request"));
			OCRepPayload *rep = (OCRepPayload *)entityHandlerRequest->payload;
			OCRepPayloadGetPropInt(rep, "tone", &tone);
			if(tone > 1915)
				tone = 1915;
			else if(tone < 956)
				tone = 956;
			buzzer.tone = (int)tone;
			OC_LOG_V(INFO, TAG, "Buzzer tone: %d", buzzer.tone);
			buzzer_put();
			OCRepPayloadSetPropInt(payload, "tone", buzzer.tone);
		}

		if (ehRet == OC_EH_OK) {
			// Format the response.  Note this requires some info about the request
			response.requestHandle = entityHandlerRequest->requestHandle;
			response.resourceHandle = entityHandlerRequest->resource;
			response.ehResult = ehRet;
			response.payload = (OCPayload*) payload;
			response.numSendVendorSpecificHeaderOptions = 0;
			memset(response.sendVendorSpecificHeaderOptions, 0, sizeof response.sendVendorSpecificHeaderOptions);
			memset(response.resourceUri, 0, sizeof response.resourceUri);
			// Indicate that response is NOT in a persistent buffer
			response.persistentBufferFlag = 0;

			// Send the response
			if (OCDoResponse(&response) != OC_STACK_OK) {
				OC_LOG(ERROR, TAG, "Error sending response");
				ehRet = OC_EH_ERROR;
			}
		}
	}

	if (entityHandlerRequest && (flag & OC_OBSERVE_FLAG)) {
		if (OC_OBSERVE_REGISTER == entityHandlerRequest->obsInfo.action) {
			OC_LOG (INFO, TAG, ("Received OC_OBSERVE_REGISTER from client"));
			gLightUnderObservation = 1;
		} else if (OC_OBSERVE_DEREGISTER == entityHandlerRequest->obsInfo.action) {
			OC_LOG (INFO, TAG, ("Received OC_OBSERVE_DEREGISTER from client"));
			gLightUnderObservation = 0;
		}
	}
	OCRepPayloadDestroy(payload);
	return ehRet;
}

OCEntityHandlerResult ButtonOCEntityHandlerCb(OCEntityHandlerFlag flag, OCEntityHandlerRequest * entityHandlerRequest,
                                        void *callbackParam)
{
	OCEntityHandlerResult ehRet = OC_EH_OK;
	OCEntityHandlerResponse response = {0};
	OCRepPayload* payload = OCRepPayloadCreate();
	if(!payload) {
		OC_LOG(ERROR, TAG, ("Failed to allocate Payload"));
		return OC_EH_ERROR;
	}

	if(entityHandlerRequest && (flag & OC_REQUEST_FLAG)) {
		OC_LOG (INFO, TAG, ("Flag includes OC_REQUEST_FLAG"));

		if(OC_REST_GET == entityHandlerRequest->method) {
			button_get();
			OCRepPayloadSetUri(payload, "/grove/button");
			OCRepPayloadSetPropInt(payload, "button", button.button);
			OCRepPayloadSetPropInt(payload, "touch", button.touch);
		} else if(OC_REST_PUT == entityHandlerRequest->method) {
			OC_LOG(ERROR, TAG, ("Un-supported request for Sensor: PUT"));
		}

		if (ehRet == OC_EH_OK) {
			// Format the response.  Note this requires some info about the request
			response.requestHandle = entityHandlerRequest->requestHandle;
			response.resourceHandle = entityHandlerRequest->resource;
			response.ehResult = ehRet;
			response.payload = (OCPayload*) payload;
			response.numSendVendorSpecificHeaderOptions = 0;
			memset(response.sendVendorSpecificHeaderOptions, 0, sizeof response.sendVendorSpecificHeaderOptions);
			memset(response.resourceUri, 0, sizeof response.resourceUri);
			// Indicate that response is NOT in a persistent buffer
			response.persistentBufferFlag = 0;

			// Send the response
			if (OCDoResponse(&response) != OC_STACK_OK) {
				OC_LOG(ERROR, TAG, "Error sending response");
				ehRet = OC_EH_ERROR;
			}
		}
	}

	if (entityHandlerRequest && (flag & OC_OBSERVE_FLAG)) {
		if (OC_OBSERVE_REGISTER == entityHandlerRequest->obsInfo.action) {
			OC_LOG (INFO, TAG, ("Received OC_OBSERVE_REGISTER from client"));
			button.observer = 1;
		} else if (OC_OBSERVE_DEREGISTER == entityHandlerRequest->obsInfo.action) {
			OC_LOG (INFO, TAG, ("Received OC_OBSERVE_DEREGISTER from client"));
			button.observer = 0;
		}
	}
	OCRepPayloadDestroy(payload);
	return ehRet;
}



// This method is used to display 'Observe' functionality of OC Stack.
static uint8_t modCounter = 0;
void *ChangeLightRepresentation (void *param)
{
#if 0
	(void)param;
	OCStackResult result = OC_STACK_ERROR;
	modCounter += 1;
	// Matching the timing that the Linux Sample Server App uses for the same functionality.
	if(modCounter % 10 == 0) {
		Light.power += 5;
		if (gLightUnderObservation) {
			OC_LOG_V(INFO, TAG, " =====> Notifying stack of new power level %d\n", Light.power);
			result = OCNotifyAllObservers (Light.handle, OC_NA_QOS);
			if (OC_STACK_NO_OBSERVERS == result) {
				gLightUnderObservation = 0;
			}
		}
	}
#endif
    return NULL;
}

void button_observer()
{
	OCStackResult result = OC_STACK_ERROR;

	if(button.observer) {
		button_get();

		if(button.button != button.button_old || button.touch != button.touch_old) {
			result = OCNotifyAllObservers(button.handle, OC_NA_QOS);
			button.button_old = button.button;
			button.touch_old = button.touch;

			if(result == OC_STACK_NO_OBSERVERS)
				button.observer = 0;
		}
	}
}

//The setup function is called once at startup of the sketch
void setup()
{
	// Add your initialization code here
	// Note : This will initialize Serial port on Arduino at 115200 bauds
	OC_LOG_INIT();
	OC_LOG(DEBUG, TAG, ("Demoserver is starting..."));

	// Connect to Ethernet or WiFi network
	if (ConnectToNetwork() != 0) {
		OC_LOG(ERROR, TAG, ("Unable to connect to network"));
		return;
	}

	// Initialize the OC Stack in Server mode
	if (OCInit(NULL, 0, OC_SERVER) != OC_STACK_OK) {
		OC_LOG(ERROR, TAG, ("OCStack init error"));
		return;
	}

	// Initialize Grove related Devices
	sensor_init();
	led_init();
	lcd_init();
	buzzer_init();
	button_init();

	// Declare and create the resource: grove
	createDemoResource();
}

// The loop function is called in an endless loop
void loop()
{
	// This artificial delay is kept here to avoid endless spinning
	// of Arduino microcontroller. Modify it as per specific application needs.
	delay(600);

	// Give CPU cycles to OCStack to perform send/recv and other OCStack stuff
	if (OCProcess() != OC_STACK_OK) {
		OC_LOG(ERROR, TAG, ("OCStack process error"));
		return;
	}

	sensor_get();
	button_observer();
}

void createDemoResource()
{
	sensor.temp = 0;
	sensor.light = 0;
	sensor.sound = 0;

	led.status = 0;

	buzzer.tone = 0;

	button.observer = 0;
	button.button = 0;
	button.touch = 0;
	button.button_old = 0;
	button.touch_old = 0;

	OCStackResult res = OCCreateResource(&sensor.handle,
		"grove.sensor",
		OC_RSRVD_INTERFACE_DEFAULT,
		"/grove/sensor",
		SensorOCEntityHandlerCb,
		NULL,
		OC_DISCOVERABLE|OC_OBSERVABLE);
	OC_LOG_V(INFO, TAG, "Created sensor resource with result: %s", getResult(res));

	res = OCCreateResource(&led.handle,
		"grove.led",
		OC_RSRVD_INTERFACE_DEFAULT,
		"/grove/led",
		LedOCEntityHandlerCb,
		NULL,
		OC_DISCOVERABLE|OC_OBSERVABLE);
	OC_LOG_V(INFO, TAG, "Created LED resource with result: %s", getResult(res));

	res = OCCreateResource(&lcd.handle,
		"grove.lcd",
		OC_RSRVD_INTERFACE_DEFAULT,
		"/grove/lcd",
		LcdOCEntityHandlerCb,
		NULL,
		OC_DISCOVERABLE|OC_OBSERVABLE);
	OC_LOG_V(INFO, TAG, "Created LCD resource with result: %s", getResult(res));

	res = OCCreateResource(&buzzer.handle,
		"grove.buzzer",
		OC_RSRVD_INTERFACE_DEFAULT,
		"/grove/buzzer",
		BuzzerOCEntityHandlerCb,
		NULL,
		OC_DISCOVERABLE|OC_OBSERVABLE);
	OC_LOG_V(INFO, TAG, "Created buzzer resource with result: %s", getResult(res));

	res = OCCreateResource(&button.handle,
		"grove.button",
		OC_RSRVD_INTERFACE_DEFAULT,
		"/grove/button",
		ButtonOCEntityHandlerCb,
		NULL,
		OC_DISCOVERABLE|OC_OBSERVABLE);
	OC_LOG_V(INFO, TAG, "Created button resource with result: %s", getResult(res));
}

const char *getResult(OCStackResult result) {
	switch (result) {
		case OC_STACK_OK:
			return "OC_STACK_OK";
		case OC_STACK_INVALID_URI:
			return "OC_STACK_INVALID_URI";
		case OC_STACK_INVALID_QUERY:
			return "OC_STACK_INVALID_QUERY";
		case OC_STACK_INVALID_IP:
			return "OC_STACK_INVALID_IP";
		case OC_STACK_INVALID_PORT:
			return "OC_STACK_INVALID_PORT";
		case OC_STACK_INVALID_CALLBACK:
			return "OC_STACK_INVALID_CALLBACK";
		case OC_STACK_INVALID_METHOD:
			return "OC_STACK_INVALID_METHOD";
		case OC_STACK_NO_MEMORY:
			return "OC_STACK_NO_MEMORY";
		case OC_STACK_COMM_ERROR:
			return "OC_STACK_COMM_ERROR";
		case OC_STACK_INVALID_PARAM:
			return "OC_STACK_INVALID_PARAM";
		case OC_STACK_NOTIMPL:
			return "OC_STACK_NOTIMPL";
		case OC_STACK_NO_RESOURCE:
			return "OC_STACK_NO_RESOURCE";
		case OC_STACK_RESOURCE_ERROR:
			return "OC_STACK_RESOURCE_ERROR";
		case OC_STACK_SLOW_RESOURCE:
			return "OC_STACK_SLOW_RESOURCE";
		case OC_STACK_NO_OBSERVERS:
			return "OC_STACK_NO_OBSERVERS";
		case OC_STACK_ERROR:
			return "OC_STACK_ERROR";
		default:
			return "UNKNOWN";
	}
}

