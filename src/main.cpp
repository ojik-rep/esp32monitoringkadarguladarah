#include <Arduino.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <FirebaseESP32.h>

#define FIREBASE_HOST "https://glucoesp32-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define FIREBASE_AUTH "5PrNU6D2w9MqcG6djwoFtgmgwphHhINvnkq53o1d"

const char* WIFI_SSID = "ojiks";
const char* WIFI_PASSWORD = "12345678";

FirebaseAuth auth;
FirebaseData fbdo ;
FirebaseConfig config;
FirebaseJson json;

String userID = "";
String control = "/Alat/TargetUser";
String status = "/Alat/Status";
String database;

int analPin = 34;
float gluc = 0.0;
float SugarLevel;
String tanggal, waktu;

WiFiUDP ntp;
NTPClient waktu32(ntp, "pool.ntp.org");
const char* bulan[] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

void koneksi(){
  Serial.print("Menghubungkan ke Wi-Fi :");
  Serial.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Terhubung");
}

void getWaktu(){
  waktu32.update();
  time_t epochTime = waktu32.getEpochTime();
  struct tm *ptm = gmtime((time_t*)&epochTime);
  int tgl = ptm->tm_mday;
  int bln = ptm->tm_mon;
  int tahun = ptm->tm_year +1900;
  waktu = waktu32.getFormattedTime();
  tanggal = String(tgl) + " " + String(bulan[bln]) + " " + String(tahun);
}

void prosesSampel(){
  Serial.println("Mulai Pengukuran ...");

  long total = 0;
  int jmlSampel = 10;

  for (int i = 0; i < jmlSampel; i++)
  {
    gluc = analogRead(analPin);
    total += gluc ;

    delay(1000);
  }

  float hasil = (float)total /jmlSampel;
  Serial.print("Hasil Analog : ");
  Serial.println(hasil);
  
  SugarLevel = (1.3798* hasil) - 38.53;

  if (SugarLevel < 0)
  {
    SugarLevel = 0;
  }
  Serial.print("Hasil Pengukuran : ");
  Serial.println(SugarLevel);
  
}

void setup() {
  Serial.begin(115200);
  koneksi();

  pinMode (analPin, INPUT);
  gluc = analogRead(analPin);

  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin (&config, &auth);
  Firebase.reconnectWiFi(true);

  waktu32.begin();
  waktu32.setTimeOffset(8 * 3600);

  Firebase.setString(fbdo, control, "");
  Firebase.setString(fbdo, status, "STANDBY");
  
  
  Serial.println("-----------------------------------");
  Serial.println("Menunggu perintah dari Aplikasi...");
  Serial.println("-----------------------------------");

}


void loop() {
  if (Firebase.getString(fbdo, control))
  {
    userID = fbdo.stringData();
    if (userID != "" &&userID != "null")
    {
      Serial.print("User : ");
      Serial.println(userID);

      Firebase.setString(fbdo, control, "");
    
      Serial.println("Pengukuran Ready");
      Firebase.setString(fbdo, status, "Ready");
      delay(3000);
      prosesSampel();
      getWaktu();

      json.set("SugarLevel", String(SugarLevel));
      json.set("Waktu", String(waktu));
      json.set("Tanggal", String(tanggal));

      database = "/Data/", userID;

      Serial.println("Mengirim ke: ");
      Serial.println(database); 

      if (Firebase.pushJSON(fbdo, database.c_str(), json))
      {
        Serial.println("Sukses");
        Firebase.setString(fbdo, control, "");

        Firebase.setString(fbdo, status, "Selesai");

      } else {
        Serial.println("Gagal");
        Serial.println(fbdo.errorReason());
      }
    } 
  } else {
      Serial.print(".");
      delay(500);
  }

  delay(100);
}

