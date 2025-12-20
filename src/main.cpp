#include <Arduino.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <FirebaseESP32.h>

#define FIREBASE_HOST "https://glucoesp32-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define FIREBASE_AUTH "5PrNU6D2w9MqcG6djwoFtgmgwphHhINvnkq53o1d"

const char* WIFI_SSID = "LAB Adikarsa 14";
const char* WIFI_PASSWORD = "adikarsa2025";

FirebaseAuth auth;
FirebaseData fbdoControl; 
FirebaseData fbdoWrite; 
FirebaseConfig config;
FirebaseJson json;

String userID = "";
String control = "/Alat/TargetUser";
String status = "/Alat/Status";
String database;

int analPin = 34;
float gluc = 0.0;
float SugarLevel, hasil;
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
    total += gluc;              
    
  
    Serial.print("Sampel ke-");
    Serial.print(i + 1); 
    Serial.print(": ");
    Serial.println(gluc);
    
    delay(1000); 
  }

  hasil = (float)total /jmlSampel;
  SugarLevel = (1.3798* hasil) - 38.53;

  Serial.print("Rata-rata (x)    : "); 
  Serial.println(hasil);
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

  fbdoControl.setResponseSize(1024);
  fbdoWrite.setResponseSize(1024);

  Firebase.begin (&config, &auth);
  Firebase.reconnectWiFi(true);

  waktu32.begin();
  waktu32.setTimeOffset(8 * 3600);

}

void loop() {
    
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[LOOP] WiFi Terputus! Mencoba reconnect...");
    WiFi.reconnect();
    delay(2000); 
    return; 
  }
  Serial.print(".");

  if (Firebase.getString(fbdoControl, control)) {
    String tempUID = fbdoControl.stringData();
    tempUID.trim();

    if (tempUID != "" && tempUID != "null" && tempUID.length() > 5) {
      Serial.println("\n[LOOP] Perintah Diterima!");
      Serial.print("UID: "); Serial.println(tempUID);

      if (Firebase.deleteNode(fbdoWrite, control)) {
        Serial.println("[LOOP] Perintah dihapus. Mulai proses...");

        userID = tempUID;

        Firebase.setString(fbdoWrite, status, "SIAP_UKUR");
        Serial.println("Status: SIAP_UKUR (Menunggu user 3 detik...)");
        
        delay(3000); 
        
        prosesSampel();
        getWaktu();

        json.set("SugarLevel", String(SugarLevel));
        json.set("ADC", String(hasil)); 
        json.set("Waktu", waktu);
        json.set("Tanggal", tanggal);

        database = "/Data/" + userID;
        
        if (Firebase.pushJSON(fbdoWrite, database.c_str(), json)) {
          Serial.println("[LOOP] Kirim SUKSES!");
          Firebase.setString(fbdoWrite, status, "SELESAI");
        } else {
          Serial.print("[LOOP] Kirim GAGAL: ");
          Serial.println(fbdoWrite.errorReason());
          Firebase.setString(fbdoWrite, status, "STANDBY");
        }

        delay(2000);
        Firebase.setString(fbdoWrite, status, "STANDBY");
        Serial.println("Selesai. Kembali ke STANDBY.");
      } else {
        Serial.println("Gagal menghapus perintah (Mungkin koneksi sibuk). Coba lagi nanti.");
      }
    }
  } else {
    Serial.print(".");
  }
  
  delay(500);
}