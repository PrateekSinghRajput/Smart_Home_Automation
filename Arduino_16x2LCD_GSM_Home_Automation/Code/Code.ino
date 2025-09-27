//Prateek
//www.justdoelectronics.com

#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
SoftwareSerial GSM(3, 2);

String phone_no1 = "+918830584864";
String phone_no2 = "+918830584864";
String phone_no3 = "";

String RxString = "";
char RxChar = ' ';
int Counter = 0;
String GSM_Nr = "";
String GSM_Msg = "";

#define Relay1 10  // Load1 Pin Out
#define Relay2 11  // Load2 Pin Out

int load1, load2;

void setup() {

  pinMode(Relay1, OUTPUT);
  digitalWrite(Relay1, 1);
  pinMode(Relay2, OUTPUT);
  digitalWrite(Relay2, 1);

  Serial.begin(9600);
  GSM.begin(9600);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("  Welcome To");
  lcd.setCursor(0, 1);
  lcd.print("JustDoElectronic");

  Serial.println("Initializing....");
  initModule("AT", "OK", 1000);
  initModule("AT+CPIN?", "READY", 1000);
  initModule("AT+CMGF=1", "OK", 1000);
  initModule("AT+CNMI=2,2,0,0,0", "OK", 1000);
  Serial.println("Initialized Successfully");

  load1 = EEPROM.read(1);
  load2 = EEPROM.read(2);
  relays();
  delay(100);
  lcd.clear();
}

void loop() {


  lcd.setCursor(0, 0);
  lcd.print("FAN");
  lcd.setCursor(8, 0);
  lcd.print("LIGHT");
  RxString = "";
  Counter = 0;
  while (GSM.available()) {
    delay(1);
    RxChar = char(GSM.read());
    if (Counter < 200) {
      RxString.concat(RxChar);
      Counter = Counter + 1;
    }
  }

  if (Received(F("CMT:"))) GetSMS();

  if (GSM_Nr == phone_no1 || GSM_Nr == phone_no2 || GSM_Nr == phone_no3) {

    if (GSM_Msg == "Fanon") {
      load1 = 0;
      sendSMS(GSM_Nr, "Ok fan is On");
      lcd.setCursor(0, 1);
      lcd.print("ON.");
    }
    if (GSM_Msg == "Fanoff") {
      load1 = 1;
      sendSMS(GSM_Nr, "Ok fan is Off");
      lcd.setCursor(0, 1);
      lcd.print("OFF");
    }

    if (GSM_Msg == "Lighton") {
      load2 = 0;
      sendSMS(GSM_Nr, "Ok light is On");
      lcd.setCursor(8, 1);
      lcd.print("ON.");
    }
    if (GSM_Msg == "Lightoff") {
      load2 = 1;
      sendSMS(GSM_Nr, "Ok light is Off");
      lcd.setCursor(8, 1);
      lcd.print("OFF");
    }

    if (GSM_Msg == "Allon") {
      load1 = 0, load2 = 0;
      sendSMS(GSM_Nr, "Ok All Device is On");
      lcd.setCursor(0, 1);
      lcd.print("ON.");
      lcd.setCursor(8, 1);
      lcd.print("ON.");
    }
    if (GSM_Msg == "Alloff") {
      load1 = 1, load2 = 1;
      sendSMS(GSM_Nr, "Ok All Device is Off");
      lcd.setCursor(0, 1);
      lcd.print("OFF");
      lcd.setCursor(8, 1);
      lcd.print("OFF");
    }

    if (GSM_Msg == "Loadstatus") {
      String loadst = "";

      if (load1 == 0) {
        loadst = "Fan On\r\n";
      } else {
        loadst = "Fan Off\r\n";
      }

      if (load2 == 0) {
        loadst = loadst + "Light On\r\n";
      } else {
        loadst = loadst + "Light Off\r\n";
      }

      sendSMS(GSM_Nr, loadst);
    }

    eeprom_write();
    relays();
  }

  GSM_Nr = "";
  GSM_Msg = "";
}

void eeprom_write() {
  EEPROM.write(1, load1);
  EEPROM.write(2, load2);
}

void relays() {
  digitalWrite(Relay1, load1);
  digitalWrite(Relay2, load2);
}

// Send SMS
void sendSMS(String number, String msg) {
  GSM.print("AT+CMGS=\"");
  GSM.print(number);
  GSM.println("\"\r\n");
  delay(500);
  GSM.println(msg);
  delay(500);
  GSM.write(byte(26));
  delay(5000);
}

void GetSMS() {

  GSM_Nr = RxString;
  int t1 = GSM_Nr.indexOf('"');
  GSM_Nr.remove(0, t1 + 1);
  t1 = GSM_Nr.indexOf('"');
  GSM_Nr.remove(t1);

  GSM_Msg = RxString;
  t1 = GSM_Msg.indexOf('"');
  GSM_Msg.remove(0, t1 + 1);
  t1 = GSM_Msg.indexOf('"');
  GSM_Msg.remove(0, t1 + 1);
  t1 = GSM_Msg.indexOf('"');
  GSM_Msg.remove(0, t1 + 1);
  t1 = GSM_Msg.indexOf('"');
  GSM_Msg.remove(0, t1 + 1);
  t1 = GSM_Msg.indexOf('"');
  GSM_Msg.remove(0, t1 + 1);
  t1 = GSM_Msg.indexOf('"');
  GSM_Msg.remove(0, t1 + 1);
  GSM_Msg.remove(0, 1);
  GSM_Msg.trim();

  Serial.print("Number:");
  Serial.println(GSM_Nr);
  Serial.print("SMS:");
  Serial.println(GSM_Msg);
}

boolean Received(String S) {
  if (RxString.indexOf(S) >= 0) return true;
  else return false;
}

void initModule(String cmd, char *res, int t) {
  while (1) {
    Serial.println(cmd);
    GSM.println(cmd);
    delay(100);
    while (GSM.available() > 0) {
      if (GSM.find(res)) {
        Serial.println(res);
        delay(t);
        return;
      } else {
        Serial.println("Error");
      }
    }
    delay(t);
  }
}