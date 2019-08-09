//----------------------------------------------------------------------------------------------------------------------------//
#include "Arduino.h"
#include "LiquidCrystal.h"
#include "DHT_U.h"
#include "DHT.h"
#include "DS3231.h"
#include "Adafruit_BMP280.h"
#include "Adafruit_CCS811.h"
#include "SoftwareSerial.h"


//----------------------------------------------------------------------------------------------------------------------------//
#define DHTPIN 6 //Input des "Data"-Kabels auf Pin-6 --> Luftfeuchte
#define DHTTYPE DHT22 // DHT11, DHT21, DHT22 --> Typ des Sensors
#define BILDSCHIRM_LICHT A4

//----------------------------------------------------------------------------------------------------------------------------//
DHT dht(DHTPIN, DHTTYPE); // Initialisierung des Luftfeuchte Sensors

DS3231  rtc(SDA, SCL);

LiquidCrystal lcd(13, 12, 5, 4, 3, 2); // Initialisieren mit den verwendeten Pins des Displays am Arduino Board

Adafruit_BMP280 bmp(34, 32, 30, 28);

Adafruit_CCS811 ccs; // Initialisierung

SoftwareSerial sim900(10, 11); // Konfiguration des Software Serial Ports auf Pins 10 und 11

String incomingCharStr;

float sensorVolt = 0; // Deklaration der nötigen Variablen

const int LED_GRUEN = 7;
const int LED_ROT = 8;

int TVOC = 0;
int CO2 = 0;


//----------------------------------------------------------------------------------------------------------------------------//
void sendStatusSMS() 
{
  sim900.print("AT+CMGF=1\r");  // AT Kommando um sim900 in SMS Modus zu versetzen
  delay(100);

  sim900.println("AT + CMGS = \"+4915734064246\""); // Sende SMS an folgende Nummer --> Internationales Format
  delay(100);
  
  
  sim900.println("STATUS WOHNUNG"); // SMS Inhalt -- Zeilenumbrüche über neue println moeglich
  
  sim900.println();

  sim900.print(rtc.getDOWStr(FORMAT_SHORT));
  sim900.print(". ");
  sim900.print(rtc.getDateStr(FORMAT_LITTLEENDIAN));
  sim900.print(", ");
  sim900.print(rtc.getTimeStr());
  sim900.print("\n");
  
  sim900.println();
  
  sim900.print("Temperatur: ");
  sim900.print(dht.readTemperature());
  sim900.print(" *C\n");
   
  sim900.print("Luftfeuchte: ");
  sim900.print(dht.readHumidity());
  sim900.print(" %\n");
   
  sim900.print("MQ9-Spannung: ");
  sim900.print(sensorVolt);
  sim900.print(" V\n");
  
  sim900.print("Luftdruck: ");
  sim900.print(bmp.readPressure());
  sim900.print(" Pa\n");
  
  sim900.print("CO2: ");
  sim900.print(CO2);
  sim900.print(" ppm\n");

  sim900.print("TVOC: ");
  sim900.print(TVOC);
  sim900.print(" ppbb");

  delay(100);

  
  sim900.println((char)26); // AT Kommandos beenden mit ^Z, ASCII Tabelle Nummer 26
  delay(100);

  
  //delay(5000); // Zeit für das Modul um Nachricht zu senden
}


//----------------------------------------------------------------------------------------------------------------------------//
void sendDefinedSMS(String smsText)
{
  sim900.print("AT+CMGF=1\r");
  delay(100);

  sim900.println("AT + CMGS = \"+4915734064246\"");
  delay(100);

  sim900.print(smsText);
  delay(100);

  sim900.println((char)26);
  delay(100);
}



//----------------------------------------------------------------------------------------------------------------------------//
void checkMQ9Voltage()
{
  if (sensorVolt <= 0.94)
  {
    digitalWrite (LED_GRUEN, HIGH);
    digitalWrite (LED_ROT, LOW);
  }

  else
  {
    digitalWrite (LED_GRUEN, LOW);
    digitalWrite (LED_ROT, HIGH);
  }
}


//----------------------------------------------------------------------------------------------------------------------------//
void checkSensors()
{

  if (dht.readTemperature()>=25)
  {
    sendDefinedSMS("Temperatur zu hoch!");
  }

  if (dht.readTemperature()<=20)
  {
    sendDefinedSMS("Temperatur zu niedrig!");
  }
  
  if (dht.readHumidity()>=60)
  {
    sendDefinedSMS("Luftfeuchte zu hoch!");
  }

  if (dht.readHumidity() <=40)
  {
    sendDefinedSMS("Luftfeuchte zu niedrig");
  }
  
  if (sensorVolt > 0.94)
  {
    sendDefinedSMS("Kohlenmonoxid kritisch");
  }

  if (bmp.readPressure() <= 101000)
  {
    sendDefinedSMS("Niedriger Luftdruck");
  }

  if (bmp.readPressure() >= 101400)
  {
    sendDefinedSMS("Hoher Luftdruck");
  }
  
  if (ccs.geteCO2() >= 500)
  {
    sendDefinedSMS("CO2 Gehalt zu hoch");
  }

  if (ccs.getTVOC() >= 100)
  {
    sendDefinedSMS("TVOC Gehalt zu hoch");
  }
  
  else{}
  
}


//----------------------------------------------------------------------------------------------------------------------------//
void sim900Power() // Funktion zum starten des Shields über des Arduino 
{
  pinMode(9, OUTPUT); 
  digitalWrite(9,LOW);
  delay(1000);
  digitalWrite(9,HIGH); // Strom für 5 Sekunden, davor und danach wieder aus
  delay(3000);
  digitalWrite(9,LOW);
  delay(3000);
}


//----------------------------------------------------------------------------------------------------------------------------//
void checkIncomingSMS()
{
  if(sim900.available() >0) { // Zeige alle einkommenden Nachrichten auf dem Serial Monitor
    
    incomingCharStr=sim900.readString(); // SIM wird ausgelesen und in vorher deklarierte Variable abgespeichert

    incomingCharStr.toUpperCase(); // Konvertieren der Nachricht in eine Nachricht aus reinen Großbuchstaben
  
    
    if (incomingCharStr.indexOf("+CMT")>=0) // AT Kommando CMT steht für empfangene Nachrichten --> soll enthalten sein im String damit Schleife ausgeführt wird
    {
      if (incomingCharStr.indexOf("STATUS")>=0) // Nachricht soll "STATUS" enthalten, damit if Schleife ausgeführt wird
      {
        sendStatusSMS();
      }
      else{}
    }
    else{}
  }
  else{}
}





//----------------------------------------------------------------------------------------------------------------------------//
void setup() 
{
  pinMode (BILDSCHIRM_LICHT, OUTPUT);
  digitalWrite(BILDSCHIRM_LICHT, HIGH);


 //----------------------------------------------------------------------------------------------------------------------------//
  lcd.begin(16, 2); // Setup des Displays mit Größe
  dht.begin(); // Setup des Feuchte Sensors
  rtc.begin();
  ccs.begin();
  sim900.begin(19200); // Arduino kommuniziert bei einer Baud Rate von 19200

 //----------------------------------------------------------------------------------------------------------------------------//
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
    Adafruit_BMP280::SAMPLING_X2,                     /* Temp. oversampling */
    Adafruit_BMP280::SAMPLING_X16,                    /* Pressure oversampling */
    Adafruit_BMP280::FILTER_X16,                      /* Filtering. */
    Adafruit_BMP280::STANDBY_MS_500);                 /* Standby time. */

 //----------------------------------------------------------------------------------------------------------------------------//      
  if (!bmp.begin()) 
  {
    lcd.setCursor(0,0);
    lcd.print("  BMP280 nicht"); // 
    lcd.setCursor(0,1);          // Sollte der BMP280-Sensor nicht gefunden werden Fehlermeldung ausgeben 
    lcd.print("    gefunden");   //
    while (1);
  }

 //----------------------------------------------------------------------------------------------------------------------------//
  if(!ccs.begin())
  {
    lcd.setCursor(0,0);
    lcd.print("  CCS811 nicht"); // 
    lcd.setCursor(0,1);          // Sollte der CCS811-Sensor nicht gefunden werden Fehlermeldung ausgeben 
    lcd.print("    gefunden");   //
    while(1);
  }

 //----------------------------------------------------------------------------------------------------------------------------//
  // delay(15000);
  // sim900.print("AT+CMGF=1\r"); // AT Kommando um Shield in SMS Modus zu setzen
  delay(100);

 //----------------------------------------------------------------------------------------------------------------------------//
  pinMode (LED_GRUEN, OUTPUT);
  pinMode (LED_ROT, OUTPUT);

  // sim900Power();
}


//----------------------------------------------------------------------------------------------------------------------------//
void loop()
{

  if (rtc.getTime().hour < 22 && rtc.getTime().hour > 5)
  {
    digitalWrite (BILDSCHIRM_LICHT, HIGH);
  
  
   //----------------------------------------------------------------------------------------------------------------------------//
    lcd.clear();
    lcd.setCursor(0, 0);

    lcd.println("  Temperatur:   ");
    lcd.setCursor(0,1);
    lcd.print("    ");
    lcd.print(dht.readTemperature());
    lcd.print(" *C");

    checkIncomingSMS();
    checkMQ9Voltage();

    delay(3000); 


   //----------------------------------------------------------------------------------------------------------------------------//
    lcd.clear(); 
    lcd.setCursor(0, 0); 
    lcd.println("  Luftfeuchte:  "); 
    lcd.setCursor(0, 1);
    lcd.print("    ");
    lcd.print(dht.readHumidity());
    lcd.print(" %");

    checkIncomingSMS();
    checkMQ9Voltage();

    delay(3000);


   //----------------------------------------------------------------------------------------------------------------------------//
    lcd.clear();
    lcd.setCursor(0, 0); // Cursor auf erstes Feld des Display --> oben links
    lcd.print("  "); // Leerzeichen weil schön
    lcd.print(rtc.getDOWStr(FORMAT_SHORT)); // Ausgabe des Wochentags im kurzen Format --> siehe library
    lcd.print(".   "); // Punkt und Leerzeichen weil schön
    lcd.print(rtc.getDateStr(FORMAT_LITTLEENDIAN)); // Ausgabe des Datums im kurzen Format --> siehe library
  
    lcd.setCursor(0, 1); // Cursor auf erstes Feld der zweiten Zeile
    lcd.print("    "); // Leerzeichen weil schön
    lcd.print(rtc.getTimeStr(FORMAT_SHORT)); // Ausgabe der Zeit im Standart Format

    checkIncomingSMS();
    checkMQ9Voltage();

    delay(3000);
  

   //----------------------------------------------------------------------------------------------------------------------------//
    int sensorValue = analogRead(A0); // Definition der nötigen Variablen
    sensorVolt = ((float)sensorValue / 1024) * 5.0; // Definition einer bereits deklarierten Variable
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(" MQ9-Spannung:");
    lcd.setCursor(0,1);
    lcd.print("      ");
    lcd.print(sensorVolt); // Ausgabe der Spannung auf dem Display
    lcd.print("V"); // Ungefährer Richtwert für ein Ausschlagen des Alarms wären etwa 0,65V

    checkIncomingSMS();
    checkMQ9Voltage();

    delay(3000);
  

   //----------------------------------------------------------------------------------------------------------------------------//
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("   Luftdruck:");
    lcd.setCursor(0,1);
    lcd.print("   ");
    lcd.print(bmp.readPressure());
    lcd.print("Pa");

    checkIncomingSMS();
    checkMQ9Voltage();

    delay(3000);
 

   //----------------------------------------------------------------------------------------------------------------------------//
    if(ccs.available()) // Falls Sensor gefunden, Block ausführen
    { 

      if(!ccs.readData()) // Empfange Daten
      { 
        lcd.clear();
        lcd.setCursor(0,0); 
        lcd.print("      CO2:");
        lcd.setCursor(0,1);
        lcd.print("     ");
        lcd.print(ccs.geteCO2()); // Ausgabe des CO2 Gehalts
        lcd.print("ppm");
        CO2 = ccs.geteCO2();
      }

      else
      {
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("   Keine Daten"); // Fehlermeldung für den Fall, dass Sensor gefunden wurde aber Daten nicht ausgelesen werden konnen
      }
    }

    else
    {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Nicht erreichbar"); // Fehlermeldung für den Fall, dass Sensor nicht gefunden wird
    }

    checkIncomingSMS();
    checkMQ9Voltage();

    delay(3000);
 

   //----------------------------------------------------------------------------------------------------------------------------//
    if(ccs.available()) // Falls Sensor gefunden, Block ausführen
    { 

      if(!ccs.readData()) // Empfange Daten
      { 
        lcd.clear();
        lcd.setCursor(0,0); 
        lcd.print("     TVOC:");
        lcd.setCursor(0,1);
        lcd.print("     ");
        lcd.print(ccs.getTVOC()); // Ausgabe des CO2 Gehalts
        lcd.print("ppb");
        TVOC = ccs.getTVOC();
      }
    }

    else
    {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("  Keine Daten!"); // Fehlermeldung für den Fall, dass Sensor gefunden wurde aber Daten nicht ausgelesen werden konnen
    }

    checkIncomingSMS();
    checkMQ9Voltage();

    delay(3000);
  }


 //----------------------------------------------------------------------------------------------------------------------------//
  else
  {
    lcd.noDisplay();
    digitalWrite(BILDSCHIRM_LICHT, LOW);
    digitalWrite (LED_GRUEN, LOW);
    digitalWrite (LED_ROT, LOW);
    checkIncomingSMS();
    delay(10000);
  }
}
