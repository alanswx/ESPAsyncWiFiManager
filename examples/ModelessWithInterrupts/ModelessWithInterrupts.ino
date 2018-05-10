#if defined(ESP8266)
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#else
#include <WiFi.h>
#endif

//needed for library
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>         //https://github.com/tzapu/WiFiManager

AsyncWebServer server(80);
DNSServer dns;
AsyncWiFiManager wifiManager(&server,&dns);

volatile boolean guard = true;
const uint32_t callCycleCount = ESP.getCpuFreqMHz() * 1024 / 8;

/*
 * If some other code was in the process of calling a method in
 * WiFi, ESP or EEPROM, and we do too, there is a very good chance
 * a reset will happen.
 */
void ICACHE_RAM_ATTR interruptFunction() {
	/*
	 * This is equivalent to:
	 * timer0_write(ESP.getCycleCount() + ESP.getCpuFreqMHz() * 1024 / 8)
	 * But that could clash with the main thread code.
	 */
    uint32_t ccount;
    __asm__ __volatile__("esync; rsr %0,ccount":"=a" (ccount));

	timer0_write(ccount + callCycleCount);

	if (!guard) {
		// An example call that would cause a reset if it happens
		// to be called while AsyncWiFiManager is also calling
		// methods that modify flash.
		ESP.getChipId();
	}
}

void setup() {
    // put your setup code here, to run once:
    Serial.begin(115200);

    //WiFiManager
    //Local intialization. Once its business is done, there is no need to keep it around
    //reset saved settings
    //wifiManager.resetSettings();

    //set custom ip for portal
    //wifiManager.setAPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

    //fetches ssid and pass from eeprom and tries to connect
    //if it does not connect it starts an access point with the specified name
    //here  "AutoConnectAP"
    //and goes into a blocking loop awaiting configuration
    //wifiManager.autoConnect("AutoConnectAP");
    //or use this for auto generated name ESP + ChipID
    //wifiManager.autoConnect();
    wifiManager.startConfigPortalModeless("ModelessAP", "Password");


 server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "Hello World");
  });

    // Set up some code that could cause a reset.
	guard = true;
	timer0_isr_init();
	timer0_attachInterrupt(interruptFunction);
	timer0_write(ESP.getCycleCount() + ESP.getCpuFreqMHz() * 1024);	// Wait 2 seconds before displaying
}

void loop() {
    // put your main code here, to run repeatedly:
	// This can be called with guarding interrupt routines - so
	// interrupt routines can interrupt it!
    wifiManager.safeLoop();

    guard = true;
	// This call can not be interrupted without possibly causing a reset.
	wifiManager.criticalLoop();
    guard = false;
}
