#include "Adafruit_SSD1306.h"
#include "PietteTech_DHT.h"
#include "httpclient.h"
#include "dnsclient.h"
#include "tokens.h"

//
// Don't require cloud connection / wifi
//

// SYSTEM_MODE(SEMI_AUTOMATIC);


//
// OLED DEFINES
// use hardware SPI
#define OLED_MOSI   D0
#define OLED_CLK    D1
#define OLED_DC     D2
#define OLED_CS     D3
#define OLED_RESET  D4
Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

//
// DHT22 LIB DEFINES
//
#define DHTTYPE  DHT22
#define DHTPIN   6
#define DHT_SAMPLE_INTERVAL   3000  // Sample every minute

//declaration
void dht_wrapper(); // must be declared before the lib initialization
PietteTech_DHT DHT(DHTPIN, DHTTYPE, dht_wrapper);
// globals
unsigned int DHTnextSampleTime;	    // Next time we want to start sample
bool bDHTstarted;		    // flag to indicate we started acquisition
int n;                              // counter


HttpClient http;
int lightLevel = 0;

http_header_t headers[] = {
      { "Content-Type", "application/json" },
      { "X-Auth-Token" , TOKEN },
    { NULL, NULL } // NOTE: Always terminate headers will NULL
};

IPAddress dnsServerIP(8,8,8,8);
IPAddress remote_addr;
DNSClient dns;
char serverName[] = "things.ubidots.com";

http_request_t request;
http_response_t response;


double humid;
double temp;

void setup()   {

  Serial.begin(9600);
  delay(1000);

  // initialize with the I2C addr 0x3D (for the 128x64)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3D);

  // init done
  display.setTextSize(1);
  display.setTextColor(WHITE);

  // set the time for next sample
  DHTnextSampleTime = 0;

  dns.begin(dnsServerIP);
  dns.getHostByName(serverName, remote_addr);
  request.ip = remote_addr;

  request.hostname = "things.ubidots.com";
  request.port = 80;

  // Publish some variables
  Particle.variable("m1", humid);
  Particle.variable("m2", temp);

}

// This wrapper is in charge of calling
// must be defined like this for the lib work
void dht_wrapper() {
    DHT.isrCallback();
}


void loop() {

  // Check if we need to start the next sample
  if (millis() > DHTnextSampleTime) {
  	if (!bDHTstarted) {		// start the sample
  	    DHT.acquire();
  	    bDHTstarted = true;
  	}

  	if (!DHT.acquiring()) {		// has sample completed?

  	    // get DHT status
  	    int result = DHT.getStatus();

  	    switch (result) {
    		case DHTLIB_OK:
          break;
    		case DHTLIB_ERROR_CHECKSUM:
    		case DHTLIB_ERROR_ISR_TIMEOUT:
    		case DHTLIB_ERROR_RESPONSE_TIMEOUT:
    		case DHTLIB_ERROR_DATA_TIMEOUT:
    		case DHTLIB_ERROR_ACQUIRING:
    		case DHTLIB_ERROR_DELTA:
    		case DHTLIB_ERROR_NOTSTARTED:
    		default:
  		    Particle.publish("err", "Unknown error");
  		    break;
  	    }

        display.clearDisplay();
        display.setCursor(0,0);

        humid = DHT.getHumidity();
        temp = DHT.getCelsius();

        //graph.plot(millis(), temp, streaming_tokens[0]);
        //graph.plot(millis(), humid, streaming_tokens[0]);

        display.print("Humidity:    ");
        display.print(humid, 2);
        display.println("\%");

        display.print("Temperature: ");
  	    display.println(temp, 2);

        display.println();

        display.print("Connected:   ");
        if (Particle.connected()) {
          display.println("yes");
        } else {
          display.println("no");
        }

        display.display();

        request.path = "/api/v1.6/variables/"VARIABLE_ID"/values";
        request.body = "{\"value\":" + String(temp) + "}";
        http.post(request, response, headers);

        Serial.println("temp");
        Serial.println(response.body);

        request.path = "/api/v1.6/variables/"VARIABLE_ID2"/values";
        request.body = "{\"value\":" + String(humid) + "}";
        http.post(request, response, headers);

        Serial.println("humid");
        Serial.println(response.body);

  	    n++;  // increment counter
  	    bDHTstarted = false;  // reset the sample flag so we can take another
  	    DHTnextSampleTime = millis() + DHT_SAMPLE_INTERVAL;  // set the time for next sample
  	}
  }

}
