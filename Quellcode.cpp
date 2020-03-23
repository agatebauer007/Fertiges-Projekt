//----------------------------------------------------------------------------------------------------------------------------//
#include "Arduino.h"              //
#include "LiquidCrystal.h"        //
#include "DHT_U.h"                //
#include "DHT.h"                  //  Header-Dateien und Libraries für das Projekt 
#include "DS3231.h"               //
#include "Adafruit_BMP280.h"      //
#include "Adafruit_CCS811.h"      //
#include "SoftwareSerial.h"       //


//----------------------------------------------------------------------------------------------------------------------------//
#define DHTPIN 6 // Input des "Data"-Kabels auf Pin-6 --> Luftfeuchte
#define DHTTYPE DHT22 // DHT11, DHT21, DHT22 --> Typ des Sensors
#define BILDSCHIRM_LICHT A4 // Variable zum Ansteuern des Displays 

//----------------------------------------------------------------------------------------------------------------------------//
DHT dht(DHTPIN, DHTTYPE); // Initialisierung der Variable für den Luftfeuchte Sensor

DS3231  rtc(SDA, SCL); // Inititalisierung der Variable für die batteriebetriebene Uhr

LiquidCrystal lcd(13, 12, 5, 4, 3, 2); // Initialisieren der verwendeten Pins des Displays am Arduino Board

Adafruit_BMP280 bmp(34, 32, 30, 28); // Initialisieren der verwendeten Pins des Drucksensors am Arduino Board

Adafruit_CCS811 ccs; // Initialisierung der Variablen für den CO2 und TVOC Sensor

SoftwareSerial sim900(10, 11); // Konfiguration des Software Serial Ports für das SIM-Modul auf Pins 10 und 11

String incomingCharStr; // Erstellen einer neuen Variable vom Typ "String"

float sensorVolt = 0; // Deklaration und Definition der Variablen für die Messung der MQ9 Spannung

const int LED_GRUEN = 7; // Deklaration und Definition der Variablen für die verwendeten LEDs um später die Ports leichter anzusteuern
const int LED_ROT = 8;   //

int TVOC = 0;  /* Deklaration und Definition der Variablen für die Konzentrationen von TVOC und CO2 als "0" um der Variable einen */ 
int CO2 = 0;   /* festen Wert zu zu ordnen bevor diese im ersten Durchlauf wieder überschrieben wird */


//----------------------------------------------------------------------------------------------------------------------------//
bool sendStatusSMS() // Deklaration und Definition einer Funktion zum senden der Status SMS
{
  sim900.print("AT+CMGF=1\r");  // AT Kommando um sim900 in SMS Modus zu versetzen
  delay(100); // Verzögerung von 100ms um Kommando Zeit zu geben 

  sim900.println("AT + CMGS = \"+4915734064246\""); // At Kommando: "Sende SMS an folgende Nummer" --> Internationales Format
  delay(100); // Verzögerung von 100ms um Kommando Zeit zu geben 
  
  
  sim900.println("STATUS WOHNUNG"); // SMS Inhalt -- Zeilenumbrüche über neue println moeglich
  
  sim900.println(); // Leere Zeile

  sim900.print(rtc.getDOWStr(FORMAT_SHORT));            // Tag der Woche im kurzen Format ausgeben (Mo, Di, Mi, ...)
  sim900.print(". ");                                   // Punkt und Leerzeichen für bessere Übersicht
  sim900.print(rtc.getDateStr(FORMAT_LITTLEENDIAN));    // Datum mit "kurzem" Ende ("19" statt "2019") 
  sim900.print(", ");                                   // Komma und Leerzeichen für bessere Übersicht
  sim900.print(rtc.getTimeStr());                       // Zeit ausgeben
  sim900.print("\n");                                   // Zeilenumbruch
  
  sim900.println();                                     // Leere Zeile
   
  sim900.print("Temperatur: ");                         // Ausgabe von "Temperatur: "
  sim900.print(dht.readTemperature());                  // Auslesen der Temperatur von Sensor
  sim900.print(" *C\n");                                // "*C" und Zeilenumbruch
   
  sim900.print("Luftfeuchte: ");                        // Ausgabe von "Luftfeuchte: "
  sim900.print(dht.readHumidity());                     // Auslesen der Feuchtigkeit von Sensor
  sim900.print(" %\n");                                 // Ausgabe von "%" und Zeilenumbruch
   
  sim900.print("MQ9-Spannung: ");                       // Ausgabe von "MQ9-Spannung: "
  sim900.print(sensorVolt);                             // Auslesen der Spannung von Sensor
  sim900.print(" V\n");                                 // Ausgabe von "V" für Volt und Zeilenumbruch
  
  sim900.print("Luftdruck: ");                          // Ausgabe von "Luftdruck: "
  sim900.print(bmp.readPressure());                     // Auslesen des Luftdrucks von Sensor
  sim900.print(" Pa\n");                                // Ausgabe von "Pa" und Zeilenumbruch
  
  sim900.print("CO2: ");                                // Ausgabe von "CO2: "
  sim900.print(CO2);                                    // Auslesen des CO2 Gehalts von Sensor
  sim900.print(" ppm\n");                               // Ausgabe von " ppm" und Zeilenumbruch

  sim900.print("TVOC: ");                               // Ausgabe von "TVOC: "
  sim900.print(TVOC);                                   // Auslesen des TVOC Gehalts von Sensor
  sim900.print(" ppbb");                                // Ausgabe von "ppb"
  
  delay(100);                                           // Verzögerung von 100ms 

  
  sim900.println((char)26); // AT Kommandos beenden mit ^Z, ASCII Tabelle Nummer 26
  delay(100); // Verzögerung von 100ms
  
  return true;
}


//----------------------------------------------------------------------------------------------------------------------------//
bool sendDefinedSMS(String smsText) //Funktion für das schnellere Erstellen von SMS
{
  sim900.print("AT+CMGF=1\r"); // AT Kommando um sim900 in SMS Modus zu versetzen
  delay(100); // Verzögerung von 100ms 

  sim900.println("AT + CMGS = \"+4915734064246\""); // At Kommando: "Sende SMS an folgende Nummer" --> Internationales Format
  delay(100); // Verzögerung von 100ms

  sim900.print(smsText); // Senden des Übergebenen Texts im String
  delay(100); // Verzögerung von 100ms

  sim900.println((char)26); // Ausgabe von ^Z zum Beenden des Kommandos
  delay(100); // Verzögerung von 100ms
  
  return true;
}



//----------------------------------------------------------------------------------------------------------------------------//
bool checkMQ9Voltage() // Funktion zur Überprüfung der MQ9-Spannung
{
  if (sensorVolt <= 0.94) // if-Schleife falls Spannung unter/gleich 0,94 Volt ist
  {
    digitalWrite (LED_GRUEN, HIGH); // Grünes LED anschalten
    digitalWrite (LED_ROT, LOW);  // Rotes LED ausschalten
    return true;
  }

  else // falls Spannung über 0,94 Volt
  {
    digitalWrite (LED_GRUEN, LOW); // Grünes LED ausschalten
    digitalWrite (LED_ROT, HIGH); // Rotes LED anschalten
    return true;
  }
}


//----------------------------------------------------------------------------------------------------------------------------//
void checkSensors() // Funktion zum Überprüfen der Sensoren und schicken einer SMS falls Abweichung zu groß
{

  if (dht.readTemperature()>=25)                  // if-Schleife, falls die ausgelesene Temperatur über oder gleich 25 ist 
  {
    sendDefinedSMS("Temperatur zu hoch!");        // Sende SMS mit: "Temperatur zu hoch" --> Aufruf der Funktion
  }

  if (dht.readTemperature()<=20)                  //  if-Schleife, falls die ausgelesene Temperatur unter oder gleich 20 ist
  {
    sendDefinedSMS("Temperatur zu niedrig!");     // Sende SMS mit: "Temperatur zu niedrig" --> Aufruf der Funktion
  }
  
  if (dht.readHumidity()>=60)                     // if-Schleife, falls die ausgelesene Luftfeuchte über oder gleich 60% ist
  {
    sendDefinedSMS("Luftfeuchte zu hoch!");       // Sende SMS mit: "Luftfeuchte zu hoch" --> Aufruf der Funktion
  }

  if (dht.readHumidity() <=40)                    // if-Schleife, falls die ausgelesene Luftfeuchte unter oder gleich 40% ist
  {
    sendDefinedSMS("Luftfeuchte zu niedrig");     // Sende SMS mit: "Luftfeuchte zu niedrig --> Aufruf der Funktion
  }
  
  if (sensorVolt > 0.94)                          // if-Schleife, falls die ausgelesene Spannung größer als 0,94V ist
  {
    sendDefinedSMS("Kohlenmonoxid kritisch");     // Sende SMS mit: "Kohlenmonoxid kritisch" --> Aufruf der Funktion
  }

  if (bmp.readPressure() <= 101000)               // if-Schleife, falls der ausgelesene Druck unter oder gleich 101000Pa ist
  {
    sendDefinedSMS("Niedriger Luftdruck");        // Sende SMS mit: "Niedriger Luftdruck" --> Aufruf der Funktion
  }

  if (bmp.readPressure() >= 101400)               // if-Schleife, falls der ausgelesene größer oder gleich 101400Pa ist
  {
    sendDefinedSMS("Hoher Luftdruck");            // Sende SMS mit: "Hoher Luftdruck" --> Aufruf der Funktion
  }
  
  if (ccs.geteCO2() >= 500)                       // if-Schleife, falls der ausgelesene CO2 Gehalt über oder gleich 500ppm ist
  {
    sendDefinedSMS("CO2 Gehalt zu hoch");         // Sende SMS mit: "CO2 Gehalt zu hoch" --> Aufruf der Funktion
  }

  if (ccs.getTVOC() >= 100)                       // if-Schleife, falls der ausgelesene TVOC Gehalt über oder gleich 100ppb ist
  {
    sendDefinedSMS("TVOC Gehalt zu hoch");        // Sende SMS mit: "TVOC Gehalt zu hoch" --> Aufruf der Funktion
  }
  
  else{}                                          // falls kein Wert abweicht passiert nichts
  
}


//----------------------------------------------------------------------------------------------------------------------------//
bool sim900Power() // Funktion zum starten des Sim Moduls über das Arduino 
{
  pinMode(9, OUTPUT); // Definiere Pin 9 als Output
  digitalWrite(9,LOW); // Stelle sicher, dass durch Pin 9 kein Strom fließt
  delay(1000); // Verzögerung von 1000ms
  digitalWrite(9,HIGH); // Strom auf Pin 9 anschalten
  delay(3000); // Verzögerung von 3000ms
  digitalWrite(9,LOW); // Strom auf Pin 9 ausschalten 
  delay(3000); // Verzögerung von 3000ms
  return true;
}


//----------------------------------------------------------------------------------------------------------------------------//
bool checkIncomingSMS() // Funktion die das SMS Modul auf eintreffende Nachrichten überprüft
{
  if(sim900.available() >0) // Überprüfen ob Modul angeschaltet ist
  { 
    
    incomingCharStr=sim900.readString(); // SIM wird ausgelesen und SMS in vorher deklarierte Variable abgespeichert

    incomingCharStr.toUpperCase(); // Konvertieren der Nachricht in eine Nachricht aus reinen Großbuchstaben
  
    if (incomingCharStr.indexOf("+CMT")>=0) // AT Kommando CMT steht für empfangene Nachrichten --> soll enthalten sein im String damit Schleife ausgeführt wird
    {
      if (incomingCharStr.indexOf("STATUS")>=0) // Nachricht soll "STATUS" enthalten, damit if Schleife ausgeführt wird
      {
        sendStatusSMS(); // Ausführen der Funktion zum Senden der Status SMS
        return true;
      }
      else{
        return true;} // Ansonsten passiert nichts
    }
    else{
      return true;} // Ansonsten passiert nichts
  }
  else{
    return true;} // Ansonsten passiert nichts
}





//----------------------------------------------------------------------------------------------------------------------------//
void setup() // Setup des Arduinos
{
  pinMode (BILDSCHIRM_LICHT, OUTPUT); // Pin "BILDSCHRIM LICHT" wird als Output definiert
  digitalWrite(BILDSCHIRM_LICHT, HIGH); // Pin "BILDSCHIRM LICHT" wird mit Strom versorgt


 //----------------------------------------------------------------------------------------------------------------------------//
  lcd.begin(16, 2); // Setup des Displays mit Größe des Displays
  dht.begin(); // Setup des Feuchte Sensors
  rtc.begin(); // Setup der Uhr
  ccs.begin(); // Setup des CO2/TVOC Gehalts
  sim900.begin(19200); // Arduino kommuniziert bei einer Baud Rate von 19200

 //----------------------------------------------------------------------------------------------------------------------------//
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
    Adafruit_BMP280::SAMPLING_X2,                     /* Temp. oversampling */
    Adafruit_BMP280::SAMPLING_X16,                    /* Pressure oversampling */
    Adafruit_BMP280::FILTER_X16,                      /* Filtering. */
    Adafruit_BMP280::STANDBY_MS_500);                 /* Standby time. */

 //----------------------------------------------------------------------------------------------------------------------------//      
  if (!bmp.begin()) // if-Schleife falls bmp Sensor nicht gefunden wird
  {
    lcd.setCursor(0,0); // Cursor des LCD Displays auf 0,0 (Ecke oben links im Display) setzen
    lcd.print("  BMP280 nicht"); // 
    lcd.setCursor(0,1);          // Sollte der BMP280-Sensor nicht gefunden werden Fehlermeldung auf Display ausgeben 
    lcd.print("    gefunden");   //
    while (1); // Solange ausführen bis der Sensor gefunden wird
  }

 //----------------------------------------------------------------------------------------------------------------------------//
  if(!ccs.begin()) // if-Schleife falls ccs Sensor nicht gefunden wird
  {
    lcd.setCursor(0,0); // Cursor des LCD Displays auf 0,0 (Ecke oben links im Display) setzen
    lcd.print("  CCS811 nicht"); // 
    lcd.setCursor(0,1);          // Sollte der CCS811-Sensor nicht gefunden werden Fehlermeldung ausgeben 
    lcd.print("    gefunden");   //
    while(1); // Solange ausführen bis der Sensor gefunden wird
  }
  delay (100); //Verzögerung von 100ms

 //----------------------------------------------------------------------------------------------------------------------------//
 
  pinMode (LED_GRUEN, OUTPUT); // Pin "LED_GRUEN" als Output definieren
  pinMode (LED_ROT, OUTPUT); // Pin "LED_ROT" als Output definieren

  sim900Power(); // Aufrufen der Funktion zum Starten des SIM Moduls
}


//----------------------------------------------------------------------------------------------------------------------------//
void loop() // Beginn der Loop-Funktion
{

  if (rtc.getTime().hour < 22 && rtc.getTime().hour > 5) // if Schleife falls es zwischen 5 und 22 Uhr ist
  {
    digitalWrite (BILDSCHIRM_LICHT, HIGH); // Pin mit Strom versorgen sodass Bildschirm angeschaltet wird
  
  
   //----------------------------------------------------------------------------------------------------------------------------//
    lcd.clear(); // Bildschirm leeren --> Folge ist ein komplett leerer Bildschirm
    lcd.setCursor(0, 0); // Cursor des LCD Displays auf 0,0 (Ecke oben links im Display) setzen

    lcd.println("  Temperatur:   "); // Ausgabe von "Temperatur: " auf dem LCD Display
    lcd.setCursor(0,1); // Cursor des LCD Displays auf 0,1 (Ecke unten links im Display) setzen
    lcd.print("    "); // Leerzeichen zur schöneren Darstellung auf dem LCD Display
    lcd.print(dht.readTemperature()); // Gebe die ausgelesene Temperatur auf dem Display aus
    lcd.print(" *C"); // Ausgabe von "*C" auf dem LCD Display

    checkIncomingSMS(); // Aufrufen der Funktion zur Überprüfung auf eintreffende SMS 
    checkMQ9Voltage(); // Aufrufen der Funktion zur Überprüfung der MQ9-Spannung

    delay(3000); // Verzögerung von 3000ms


   //----------------------------------------------------------------------------------------------------------------------------//
    lcd.clear(); // Bildschirm leeren --> Folge ist ein komplett leerer Bildschirm
    lcd.setCursor(0, 0);  // Cursor des LCD Displays auf 0,0 (Ecke oben links im Display) setzen
    lcd.println("  Luftfeuchte:  "); // Ausgabe von "Luftfeuchte" auf dem LCD Display 
    lcd.setCursor(0, 1); // Cursor des LCD Displays auf 0,1 (Ecke unten links im Display) setzen
    lcd.print("    "); // Leerzeichen zur schöneren Darstellung auf dem LCD Display
    lcd.print(dht.readHumidity()); // Auslesen und Ausgeben der Luftfeuchte auf dem Display
    lcd.print(" %");

    checkIncomingSMS(); // Aufrufen der Funktion zur Überprüfung auf eintreffende SMS
    checkMQ9Voltage(); // Aufrufen der Funktion zur Überprüfung der MQ9-Spannung

    delay(3000); // Verzögerung von 3000ms


   //----------------------------------------------------------------------------------------------------------------------------//
    lcd.clear(); // Bildschirm leeren --> Folge ist ein komplett leerer Bildschirm
    lcd.setCursor(0, 0); // Cursor auf erstes Feld des Display --> oben links
    lcd.print("  ");  // Leerzeichen zur schöneren Darstellung auf dem LCD Display
    lcd.print(rtc.getDOWStr(FORMAT_SHORT)); // Ausgabe des Wochentags im kurzen Format --> siehe library
    lcd.print(".   "); // Leerzeichen zur schöneren Darstellung auf dem LCD Display
    lcd.print(rtc.getDateStr(FORMAT_LITTLEENDIAN)); // Ausgabe des Datums im kurzen Format --> siehe library
  
    lcd.setCursor(0, 1);  // Cursor des LCD Displays auf 0,1 (Ecke unten links im Display) setzen
    lcd.print("    ");  // Leerzeichen zur schöneren Darstellung auf dem LCD Display
    lcd.print(rtc.getTimeStr(FORMAT_SHORT)); // Ausgabe der Zeit im Standart Format auf DIsplay

    checkIncomingSMS(); // Aufrufen der Funktion zur Überprüfung auf eintreffende SMS
    checkMQ9Voltage(); // Aufrufen der Funktion zur Überprüfung der MQ9-Spannung

    delay(3000); // Verzögerung von 3000ms
  

   //----------------------------------------------------------------------------------------------------------------------------//
    int sensorValue = analogRead(A0); // Definition der nötigen Variablen
    sensorVolt = ((float)sensorValue / 1024) * 5.0; // Definition einer bereits deklarierten Variable
    lcd.clear(); // Bildschirm leeren --> Folge ist ein komplett leerer Bildschirm
    lcd.setCursor(0,0); // Cursor des LCD Displays auf 0,0 (Ecke oben links im Display) setzen
    lcd.print(" MQ9-Spannung:"); // Ausgabe von "MQ9 Spannung" auf dem LCD Display
    lcd.setCursor(0,1); // Cursor des LCD Displays auf 0,1 (Ecke unten links im Display) setzen
    lcd.print("      "); // Leerzeichen zur schöneren Darstellung auf dem LCD Display
    lcd.print(sensorVolt); // Ausgabe der Spannung auf dem Display
    lcd.print("V"); // Ausgabe von "V" auf dem LCD Display
    
    checkIncomingSMS(); // Aufrufen der Funktion zur Überprüfung auf eintreffende SMS
    checkMQ9Voltage(); // Aufrufen der Funktion zur Überprüfung der MQ9-Spannung

    delay(3000); // Verzögerung von 3000ms
  

   //----------------------------------------------------------------------------------------------------------------------------//
    lcd.clear(); // Bildschirm leeren --> Folge ist ein komplett leerer Bildschirm
    lcd.setCursor(0,0); // Cursor des LCD Displays auf 0,0 (Ecke oben links im Display) setzen
    lcd.print("   Luftdruck:"); // Ausgabe von "Luftdruck" auf dem LCD Display
    lcd.setCursor(0,1); // Cursor des LCD Displays auf 0,1 (Ecke unten links im Display) setzen
    lcd.print("   "); // Leerzeichen zur schöneren Darstellung auf dem LCD Display
    lcd.print(bmp.readPressure()); // Auslesen und Ausgeben des Drucks auf dem Display
    lcd.print("Pa"); // Ausgabe von "Pa" auf dem LCD Display

    checkIncomingSMS(); // Aufrufen der Funktion zur Überprüfung auf eintreffende SMS
    checkMQ9Voltage(); // Aufrufen der Funktion zur Überprüfung der MQ9-Spannung

    delay(3000); // Verzögerung von 3000ms
 

   //----------------------------------------------------------------------------------------------------------------------------//
    if(ccs.available()) // Falls Sensor gefunden, Block ausführen
    { 

      if(!ccs.readData()) // Empfange Daten
      { 
        lcd.clear(); // Bildschirm leeren --> Folge ist ein komplett leerer Bildschirm
        lcd.setCursor(0,0);  // Cursor des LCD Displays auf 0,0 (Ecke oben links im Display) setzen
        lcd.print("      CO2:"); // Ausgabe von "CO2:" auf dem LCD Display
        lcd.setCursor(0,1); // Cursor des LCD Displays auf 0,1 (Ecke unten links im Display) setzen
        lcd.print("     "); // Leerzeichen zur schöneren Darstellung auf dem LCD Display
        lcd.print(ccs.geteCO2()); // Ausgabe des CO2 Gehalts auf dem LCD Display
        lcd.print("ppm"); // Ausgabe von "ppm" auf dem LCD Display
        CO2 = ccs.geteCO2(); // Auslesen des C02 Gehalts und speichern in einer zuvor deklarierten Variable
      }

      else
      {
        lcd.clear(); // Bildschirm leeren --> Folge ist ein komplett leerer Bildschirm
        lcd.setCursor(0,0); // Cursor des LCD Displays auf 0,0 (Ecke oben links im Display) setzen
        lcd.print("   Keine Daten"); // Fehlermeldung für den Fall, dass Sensor gefunden wurde aber Daten nicht ausgelesen werden konnen
      }
    }

    else
    {
      lcd.clear(); // Bildschirm leeren --> Folge ist ein komplett leerer Bildschirm
      lcd.setCursor(0,0); // Cursor des LCD Displays auf 0,0 (Ecke oben links im Display) setzen
      lcd.print("Nicht erreichbar"); // Fehlermeldung für den Fall, dass Sensor nicht gefunden wird
    }

    checkIncomingSMS(); // Aufrufen der Funktion zur Überprüfung auf eintreffende SMS
    checkMQ9Voltage(); // Aufrufen der Funktion zur Überprüfung der MQ9-Spannung

    delay(3000); // Verzögerung von 3000ms
 

   //----------------------------------------------------------------------------------------------------------------------------//
    if(ccs.available()) // Falls Sensor gefunden, Block ausführen
    { 

      if(!ccs.readData()) // Empfange Daten
      { 
        lcd.clear(); // Bildschirm leeren --> Folge ist ein komplett leerer Bildschirm
        lcd.setCursor(0,0);  // Cursor des LCD Displays auf 0,0 (Ecke oben links im Display) setzen
        lcd.print("     TVOC:"); // Ausgabe von "TVOC: " auf dem LCD Display
        lcd.setCursor(0,1); // Cursor des LCD Displays auf 0,1 (Ecke unten links im Display) setzen
        lcd.print("     "); // Leerzeichen zur schöneren Darstellung auf dem LCD Display
        lcd.print(ccs.getTVOC()); // Ausgabe des CO2 Gehalts
        lcd.print("ppb"); // Ausgabe von "ppb" auf dem LCD Display
        TVOC = ccs.getTVOC(); // Auslesen des TVOC Gehalts und speichern in einer zuvor deklarierten Variable
      }
    }

    else
    {
      lcd.clear(); // Bildschirm leeren --> Folge ist ein komplett leerer Bildschirm
      lcd.setCursor(0,0); // Cursor des LCD Displays auf 0,0 (Ecke oben links im Display) setzen
      lcd.print("  Keine Daten!"); // Fehlermeldung für den Fall, dass Sensor gefunden wurde aber Daten nicht ausgelesen werden konnen
    }

    checkIncomingSMS(); // Aufrufen der Funktion zur Überprüfung auf eintreffende SMS
    checkMQ9Voltage(); // Aufrufen der Funktion zur Überprüfung der MQ9-Spannung

    delay(3000); // Verzögerung von 3000ms
  }


 //----------------------------------------------------------------------------------------------------------------------------//
  else // falls es zwischen 22 und 5 Uhr ist wird folgendes ausgegeben 
  {
    lcd.noDisplay(); // Display ausschalten
    digitalWrite(BILDSCHIRM_LICHT, LOW); // Strom für Display ausschalten
    digitalWrite (LED_GRUEN, LOW); // Grünes LED ausschalten
    digitalWrite (LED_ROT, LOW); // Rotes LED ausschalten
    checkIncomingSMS(); // Aufrufen der Funktion zur Überprüfung auf eintreffende SMS
    delay(10000); // Verzögerung von 10000ms
  }
}
