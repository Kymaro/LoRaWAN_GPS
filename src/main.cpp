#include <ESP32_LoRaWAN.h>
#include <Arduino.h>
#include <TinyGPSPlus.h>

// GPS SETUP CODE 
static const int RXPin = 22, TXPin = 23;
static const uint32_t GPSBaud = 9600;

TinyGPSPlus gps;

/*license for Heltec ESP32 LoRaWan, quary your ChipID relevant license: http://resource.heltec.cn/search */
uint32_t  license[4] = {0xB46C55E0,0x184E9C8B,0x5B1C0411,0xA5284D3D};

/* OTAA para*/

uint8_t DevEui[] = { 0x9C, 0xCA, 0xDB, 0xF4, 0xD1, 0x52, 0xD0, 0x00 };
uint8_t AppEui[] = { 0x5D, 0xF7, 0x95, 0xEE, 0x5F, 0xB8, 0x4C, 0x00 };
uint8_t AppKey[] = { 0x7C, 0x7B, 0x5E, 0x92, 0x97, 0x96, 0xE4, 0x00, 0x66, 0x55, 0x6C, 0x61, 0x09, 0xD8, 0x74, 0x00 };

/* ABP para*/
uint8_t NwkSKey[] = { 0x15, 0xb1, 0xd0, 0xef, 0xa4, 0x63, 0xdf, 0xbe, 0x3d, 0x11, 0x18, 0x1e, 0x1e, 0xc7, 0xda,0x85 };
uint8_t AppSKey[] = { 0xd7, 0x2c, 0x78, 0x75, 0x8c, 0xdc, 0xca, 0xbf, 0x55, 0xee, 0x4a, 0x77, 0x8d, 0x16, 0xef,0x67 };
uint32_t DevAddr =  ( uint32_t )0x007e6ae1;

/*LoraWan channelsmask, default channels 0-7*/ 
uint16_t userChannelsMask[6]={ 0x00FF,0x0000,0x0000,0x0000,0x0000,0x0000 };

/*LoraWan Class, Class A and Class C are supported*/
DeviceClass_t  loraWanClass = CLASS_A;

/*the application data transmission duty cycle.  value in [ms].*/
uint32_t appTxDutyCycle = 15000;

/*OTAA or ABP*/
bool overTheAirActivation = true;

/*ADR enable*/
bool loraWanAdr = true;

/* Indicates if the node is sending confirmed or unconfirmed messages */
bool isTxConfirmed = true;

/* Application port */
uint8_t appPort = 2;

/*!
* Number of trials to transmit the frame, if the LoRaMAC layer did not
* receive an acknowledgment. The MAC performs a datarate adaptation,
* according to the LoRaWAN Specification V1.0.2, chapter 18.4, according
* to the following table:
*
* Transmission nb | Data Rate
* ----------------|-----------
* 1 (first)       | DR
* 2               | DR
* 3               | max(DR-1,0)
* 4               | max(DR-1,0)
* 5               | max(DR-2,0)
* 6               | max(DR-2,0)
* 7               | max(DR-3,0)
* 8               | max(DR-3,0)
*
* Note, that if NbTrials is set to 1 or 2, the MAC will not decrease
* the datarate, in case the LoRaMAC layer did not receive an acknowledgment
*/
uint8_t confirmedNbTrials = 8;

/*LoraWan debug level, select in arduino IDE tools.
* None : print basic info.
* Freq : print Tx and Rx freq, DR info.
* Freq && DIO : print Tx and Rx freq, DR, DIO0 interrupt and DIO1 interrupt info.
* Freq && DIO && PW: print Tx and Rx freq, DR, DIO0 interrupt, DIO1 interrupt, MCU sleep and MCU wake info.
*/
uint8_t debugLevel = LoRaWAN_DEBUG_LEVEL;

/*LoraWan region, select in arduino IDE tools*/
LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;

double LAT = 0;
double LNG = 0;
String LATStr;
String LNGStr;
int decimal = 6;
// String (double, nbDecimal)
// String.length() for len
unsigned char *puc;


static void prepareTxFrame( uint8_t port )
{
    while (Serial2.available() > 0)
    {
        if (gps.encode(Serial2.read()))
        {
            Serial.print(F("Location: ")); 
            if (gps.location.isValid())
            {
                Serial.print(gps.location.lat(), 6);
                LAT = gps.location.lat();
                LATStr = String(LAT, decimal);
                Serial.print(F(","));
                Serial.print(gps.location.lng(), 6);
                LNG = gps.location.lng();
                LNGStr = String(LNG, decimal);

                appDataSize = LATStr.length() + LNGStr.length() + 2;//AppDataSize max value is 64
                appData[0] = 'H';
                puc = (unsigned char *)(&LATStr);
                for(int i=1; i < LATStr.length() + 1; i++)
                {
                  appData[i] = puc[i-1];
                }
                appData[LATStr.length() + 1] = 'L';
                puc = (unsigned char *)(&LNGStr);
                for(int i=LATStr.length() + 2; i < appDataSize; i++)
                {
                  appData[i] = puc[i - (LATStr.length() + 2)];
                }
            }
            else
            {
                Serial.print(F("INVALID"));
                appDataSize = 4;
                appData[0] = 'S';
                appData[1] = 'U';
                appData[2] = 'U';
                appData[3] = 'S';
            }

            Serial.print(F("  Date/Time: "));
            if (gps.date.isValid())
            {
                Serial.print(gps.date.month());
                Serial.print(F("/"));
                Serial.print(gps.date.day());
                Serial.print(F("/"));
                Serial.print(gps.date.year());
            }
            else
            {
                Serial.print(F("INVALID"));
            }

            Serial.print(F(" "));
            if (gps.time.isValid())
            {
                if (gps.time.hour() < 10) Serial.print(F("0"));
                Serial.print(gps.time.hour());
                Serial.print(F(":"));
                if (gps.time.minute() < 10) Serial.print(F("0"));
                Serial.print(gps.time.minute());
                Serial.print(F(":"));
                if (gps.time.second() < 10) Serial.print(F("0"));
                Serial.print(gps.time.second());
                Serial.print(F("."));
                if (gps.time.centisecond() < 10) Serial.print(F("0"));
                Serial.print(gps.time.centisecond());
            }
            else
            {
                Serial.print(F("INVALID"));
            }

            Serial.println();
        }
    }

    if (millis() > 5000 && gps.charsProcessed() < 10)
    {
        Serial.println(F("No GPS detected: check wiring."));
        while(true);
    }
}

// Add your initialization code here
void setup()
{
  if(mcuStarted==0)
  {
    LoRaWAN.displayMcuInit();
  }
  Serial.begin(115200);
  while (!Serial);
  Serial2.begin(GPSBaud, SERIAL_8N1, RXPin, TXPin);
  SPI.begin(SCK,MISO,MOSI,SS);
  Mcu.init(SS,RST_LoRa,DIO0,DIO1,license);
  deviceState = DEVICE_STATE_INIT;
}

// The loop function is called in an endless loop
void loop()
{
  switch( deviceState )
  {
    case DEVICE_STATE_INIT:
    {
      LoRaWAN.init(loraWanClass,loraWanRegion);
      break;
    }
    case DEVICE_STATE_JOIN:
    {
      LoRaWAN.displayJoining();
      LoRaWAN.join();
      break;
    }
    case DEVICE_STATE_SEND:
    {
      LoRaWAN.displaySending();
      prepareTxFrame( appPort );
      //displayInfo();
      LoRaWAN.send(loraWanClass);
      deviceState = DEVICE_STATE_CYCLE;
      break;
    }
    case DEVICE_STATE_CYCLE:
    {
      // Schedule next packet transmission
      txDutyCycleTime = appTxDutyCycle + randr( -APP_TX_DUTYCYCLE_RND, APP_TX_DUTYCYCLE_RND );
      LoRaWAN.cycle(txDutyCycleTime);
      deviceState = DEVICE_STATE_SLEEP;
      break;
    }
    case DEVICE_STATE_SLEEP:
    {
      LoRaWAN.displayAck();
      LoRaWAN.sleep(loraWanClass,debugLevel);
      break;
    }
    default:
    {
      deviceState = DEVICE_STATE_INIT;
      break;
    }
  }
}