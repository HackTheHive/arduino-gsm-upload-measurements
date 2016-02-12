 /*
  * Dependencies:
  *
  * - Low-Power v1.4.0: https://github.com/rocketscream/Low-Power
  * - DHT-sensor-library v1.2.3: https://github.com/adafruit/DHT-sensor-library
  */

#include <GSM.h>
#include <LowPower.h>
#include <DHT.h>

char SIM_CARD_PIN[] =  "";

// Access Point (APN) configuration
char GPRS_APN[]      = "Eseye.com";
char GPRS_LOGIN[]    = "foo";
char GPRS_PASSWORD[] = "foo";

const char SERVER_IP[] = "217.148.42.116";
const char SECRET_DEVICE_ID[] = "<replace with this the secret!>";
const int SERVER_PORT = 11234;
const int SENSOR_1_PIN = 0;
const int SENSOR_2_PIN = 0;


GSM GSM_MODEM;         // handles the radio modem
GPRS GPRS_CONTROLLER;  // for getting a connection to the Internet
GSMClient IP_SESSION;  // for connecting & sending data over TCP/IP

DHT DHT_SENSOR_1(SENSOR_1_PIN, DHT22);
DHT DHT_SENSOR_2(SENSOR_2_PIN, DHT22);


struct Measurements {
  float temperature_1;
  float humidity_1;
  float temperature_2;
  float humidity_2;
};


void setup() {
  initialize_serial();
  initialize_gprs_data();
  initialize_sensors();
}

void loop() {
   
  if(upload_measurements(get_measurements())) {
    wait_10_minutes();
  } else {
    Serial.println("Failed to upload measurements. Restarting modem.");
    disconnect_gsm_client();
    initialize_gprs_data();
  }
}

void initialize_serial() {
  Serial.begin(9600);
}

boolean initialize_gprs_data() {
  boolean connected = false;
  int remainingRetries = 5;

  while (!connected && remainingRetries--) {
    Serial.println("Switching on GSM modem...");

    if(GSM_READY == GSM_MODEM.begin(SIM_CARD_PIN)) {
      Serial.println("OK, GSM ready. Attaching GPRS APN, login, password...");
      
      if(GPRS_READY == GPRS_CONTROLLER.attachGPRS(GPRS_APN, GPRS_LOGIN, GPRS_PASSWORD)) {
        Serial.println("OK, GPRS attached.");
        connected = true;
        break;
      } else {
         Serial.println("ERROR: failed to attach GPRS");
      }
    } else {
      Serial.println("ERROR: failed to begin GSM connection");
    }
    
    Serial.print(remainingRetries);
    Serial.println(" retries remaining");
    delay(1000);
  }

  if(!connected) {
    Serial.println("Ouch, failed many times to connect.");
  }
  return connected;
}

void initialize_sensors() {
  DHT_SENSOR_1.begin();
  DHT_SENSOR_2.begin();
}



Measurements get_measurements() {
  Serial.println("Measuring temperature...");
  return (Measurements){
    DHT_SENSOR_1.readTemperature(),
    DHT_SENSOR_1.readHumidity(),
    DHT_SENSOR_2.readTemperature(),
    DHT_SENSOR_2.readHumidity()
  };
}


void disconnect_gsm_client() {

  if(IP_SESSION.connected()) {
    Serial.println("Disconnecting GSM client from server.");
    IP_SESSION.stop(); // disconnect from server
  }

  Serial.println("Powering off GSM modem.");
  GSM_MODEM.shutdown(); // power off modem
}


boolean upload_measurements(const Measurements& measurements)
{
  int remainingRetries = 5;
  boolean success = false;

  while(remainingRetries--) {
    Serial.println("Connecting to server...");
    if (IP_SESSION.connect(SERVER_IP, SERVER_PORT)) {
      Serial.println("OK, connected. Sending measurements...");

      String message = String("version=1&secret_device_id=")
                     + String(SECRET_DEVICE_ID)
                     + String("&")
                     + String("temperature_1=")
                     + String(measurements.temperature_1)
                     + String("&")
                     + String("humidity_1=")
                     + String(measurements.humidity_1)
                     + String("&")
                     + String("temperature_2=")
                     + String(measurements.temperature_2)
                     + String("&")
                     + String("humidity_2=")
                     + String(measurements.humidity_2);
      IP_SESSION.println(message.c_str());
      IP_SESSION.stop();

      Serial.println("OK, sent measurements.");
      success = true;
      break;
      
    } else {
      Serial.println("ERROR: Failed to connect to server.");
    }
    
    Serial.print(remainingRetries);
    Serial.println(" retries remaining");
    delay(1000);
  }
  return success;
}

void wait_10_minutes() {
  const int numberOfWaits = 75; // (10 minutes * 60) / 8 seconds
  Serial.println("Sleeping 10 minutes.");

  for(int i = 0 ; i < numberOfWaits ; i++) {
    // Arduino Leonardo is an ATmega32U4, uses following function signature:

    LowPower.idle(
      SLEEP_8S,
      ADC_OFF,
      TIMER4_OFF,
      TIMER3_OFF,
      TIMER1_OFF,
      TIMER0_OFF, 
      SPI_OFF,
      USART1_OFF,
      TWI_OFF,
      USB_OFF);
    }
    
    Serial.println("Awake"); 
}
