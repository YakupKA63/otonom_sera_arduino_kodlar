#include <Arduino.h>
#include <DHT11.h>
#include <virtuabotixRTC.h>
#include <SoftwareSerial.h>


//HC-06 Bluetooth Modülü
int BTrxPin = 1;
int BTtxPin = 0;
char BTgidenVeri;
SoftwareSerial bluetooth(0, 1);
char bluetoothVeri = ' ';

//Gerçek Zamanlı Saat Modülü
int RTCclockPin = 6;
int RTCdataPin = 7;
int RTCrstPin = 8;
virtuabotixRTC myRTC(RTCclockPin,RTCdataPin,RTCrstPin);
int RTC_day;
int RTC_month;
int RTC_year;
int RTC_hours;
int RTC_minutes;
int RTC_seconds;

//HCSR-04 Ultrasonik Mesafe Sensörü
int HCSRtrigPin_1 = 9;
int HCSRechoPin_1 = 10;
int HCSRtrigPin_2 = 11;
int HCSRechoPin_2 = 12;
int HCSRuzaklik_1;
int HCSRuzaklik_2;

//DHT-11 Nem ve Sıcaklık Sensörü
int DHToutPin = 13;
int airTemperature = 0;
int airHumidity = 0;
DHT11 dht11(DHToutPin);
unsigned long lastDHTReadTime = 0;
const int DHT_READ_INTERVAL = 30000; // yarım dakikada bir DHT11 okuma


//IR Uzaklık Sensörü
int IRoutPin_1 = A0;
int IRoutPin_2 = A7;
int IRveri_1;
int IRveri_2;

//Su Seviye Sensörü
int WSpin = A6;

//Toprak Nem Sensörü
int SMoutPin = A2;
int SMveri;

//LDR sensörü
int LDR_1 = A3;
int LDR_2 = A4;
int LDRveri_1;
int LDRveri_2;

//Yağmur Sensörü
int RainPin = A5;
int RainVeri;
bool isRain;

//DC motor sürücü -L298N- pinleri (Shift Register Üzerinde)
#define IN1 5  // Sol motor - ileri
#define IN2 4  // Sol motor - geri
#define IN3 3  // Sağ motor - ileri (OUT1)
#define IN4 2  // Sağ motor - geri (OUT2)

//Shift Register Pinleri
int SRdataPin = 2;
int SRclockPin = 3;
int SRlatchPin = 4;

//Su Motoru Pinleri
int WMpin = 5;

bool isSeraWorking = true;

float currentVoltage;

bool otoHareketBool = true;

void setup() {
  myRTC.setDS1302Time(10,10,14,5,10,2,2025);// RTC'nin başlangıç saati

  //HCSR-04 Ultrasonik Mesafe Sensörü
  pinMode(HCSRechoPin_1,INPUT);
  pinMode(HCSRechoPin_2,INPUT);
  pinMode(HCSRtrigPin_1,OUTPUT);
  pinMode(HCSRtrigPin_2,OUTPUT);

  //Shift Register
  pinMode(SRdataPin,OUTPUT);
  pinMode(SRclockPin,OUTPUT);
  pinMode(SRlatchPin,OUTPUT);


  Serial.begin(9600);
  bluetooth.begin(9600); 
  
}

void loop() {

  akim();

  if(currentVoltage <= 6.5){
    isSeraWorking = false;
  }else{
    isSeraWorking = true;
  }

  if(isSeraWorking) {

    //Sensör Veri Tanımlaması
    SMveri = analogRead(SMoutPin);
    RainVeri = analogRead(RainPin);
    LDRveri_1 = analogRead(LDR_1);
    LDRveri_2 = analogRead(LDR_2);
    IRveri_1 = analogRead(IRoutPin_1);
    IRveri_2 = analogRead(IRoutPin_2);

    //RTC Modül-Zaman Ayarlaması
    myRTC.updateTime();
    RTC_day = myRTC.dayofmonth;
    RTC_month = myRTC.month;
    RTC_year = myRTC.year;
    RTC_hours = myRTC.hours;
    RTC_minutes = myRTC.minutes;
    RTC_seconds = myRTC.seconds;

    //Yağmur Yağıyor mu / Yağmıyor mu?
    if (RainVeri >= 50)
    {
      isRain = true;
    }
    else{
      isRain = false;
    }

    DHTread();

    mesafe1();
    mesafe2();

    if (bluetooth.available()) {
        char veri = bluetooth.read(); // Gelen veriyi oku
        

        if (veri == '~'){
          otoHareketBool = false;
        }else if (veri == '~~'){
          otoHareketBool = true;
        }

        if((int)veri < 6){
          bluetoothVeri = veri;
        }
        else{
          bluetoothVeri = ' ';
        }
    }

    if(otoHareketBool){
      bluetoothVeri = ' ';
      otoMovement();

    }else{
      if(HCSRuzaklik_1 <= 15 && HCSRuzaklik_2 <= 15){
        otoSaga(1000);
      }else if (HCSRuzaklik_1 <= 15 && HCSRuzaklik_2 > 15){
        if(bluetoothVeri == '1'){
          geri();
        }else if(bluetoothVeri == '2'){
          geri();
        }
        
      }else if(HCSRuzaklik_1 > 15 && HCSRuzaklik_2 <= 15){
        if(bluetoothVeri == '1'){
          ileri();
        }else if(bluetoothVeri == '2'){
          ileri();
        }
        
      }else if(HCSRuzaklik_1 > 15 && HCSRuzaklik_2 > 15){
        if(bluetoothVeri == '1'){
          ileri();
        }else if(bluetoothVeri == '2'){
          geri();
        }
      }
      if(bluetoothVeri == '3'){
        saga();
      }
      else if(bluetoothVeri == '4'){
        sola();
      }
      else if(bluetoothVeri == '5'){
        //Manuel Sulama Kodları
        manSulama();
      }
      else if(bluetoothVeri == ' '){
        stop();
      }

    }

  }
    
    //Bluetooth İletişimi

    //Mobil Uygulamaya Giden Veriler (Bluetooth Kodu, Hava Nemi, Hava Sıcaklığı, Toprak Nemi, Yağmur Verisi, LDR Verisi 1, LDR Verisi 2)
    BTgidenVeri = (char)airHumidity + ' ' + (char)airTemperature + ' ' + (char)SMveri + ' ' + (char)RainVeri + ' ' + (char)LDR_1 + ' ' + (char)LDR_2;

    if(bluetoothVeri == ' '){
      Serial.write(BTgidenVeri);
    }
    

}

// HCSR-04 (1) Uzaklık Ölçümü
void mesafe1(){
  int sure;
  digitalWrite(HCSRtrigPin_1, LOW); //sensör pasif hale getirildi
  delayMicroseconds(5);
  digitalWrite(HCSRtrigPin_1, HIGH); //Sensore ses dalgasının üretmesi için emir verildi
  delayMicroseconds(10);
  digitalWrite(HCSRtrigPin_1, LOW); //Yeni dalgaların üretilmemesi için trig pini LOW konumuna getirildi
  sure = pulseIn(HCSRechoPin_1, HIGH); //ses dalgasının geri dönmesi için geçen sure ölçülüyor
  HCSRuzaklik_1 = sure / 29.1 / 2; //ölçülen süre uzaklığa çevriliyor
  if(HCSRuzaklik_1 == 0){
    HCSRuzaklik_1 = 400;
  }
}

// HCSR-04 (2) Uzaklık Ölçümü
void mesafe2(){
  int sure;
  digitalWrite(HCSRtrigPin_2, LOW); //sensör pasif hale getirildi
  delayMicroseconds(5);
  digitalWrite(HCSRtrigPin_2, HIGH); //Sensore ses dalgasının üretmesi için emir verildi
  delayMicroseconds(10);
  digitalWrite(HCSRtrigPin_2, LOW); //Yeni dalgaların üretilmemesi için trig pini LOW konumuna getirildi
  sure = pulseIn(HCSRechoPin_2, HIGH); //ses dalgasının geri dönmesi için geçen sure ölçülüyor
  HCSRuzaklik_2 = sure / 29.1 / 2; //ölçülen süre uzaklığa çevriliyor
  if(HCSRuzaklik_2 == 0){
    HCSRuzaklik_2 = 400;
  }
}

// İleri Gitme Kodları
void ileri() {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
  
}

void otoIleri(const long interval){
  static unsigned long previousMillis = 0;  // Son çalıştırma zamanını tutan değişken

  unsigned long currentMillis = millis();   // Şu anki zamanı al

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;  // Zamanı güncelle

    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);

  }
  stop();
}

// Geri Gitme Kodları
void geri() {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
}

void otoGeri(const long interval) {
  static unsigned long previousMillis = 0;  // Son çalıştırma zamanını tutan değişken

  unsigned long currentMillis = millis();   // Şu anki zamanı al

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;  // Zamanı güncelle

    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);

  }
  stop();
}

// Sağa Dönme Kodları
void saga() {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
}

void otoSaga(const long interval) {
  static unsigned long previousMillis = 0;  // Son çalıştırma zamanını tutan değişken

  unsigned long currentMillis = millis();   // Şu anki zamanı al

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;  // Zamanı güncelle

    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);

  }
  stop();
}

// Sola Dönme Kodları
void sola() {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
}

void otoSola(const long interval) {
  static unsigned long previousMillis = 0;  // Son çalıştırma zamanını tutan değişken

  unsigned long currentMillis = millis();   // Şu anki zamanı al

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;  // Zamanı güncelle

    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);

  }
  stop();
}

void stop(){
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

//Manuel Sulama Kodları
void manSulama(){
  while (SMveri <= 50)
  {
    digitalWrite(WMpin,HIGH);
  }if(SMveri > 50){
    digitalWrite(WMpin,LOW);
  }
  
}

//Otomatik Sulama Kodları
void otoSulama(){
  while (SMveri <= 25)
  {
    digitalWrite(WMpin,HIGH);
  }if(SMveri > 50){
    digitalWrite(WMpin,LOW);
  }
}

//Otomatik Hareket Kodları
void otoMovement() {

   // Mesafeleri güncelle
  mesafe1();
  mesafe2();
  

  if(!isRain){
    if (LDRveri_1 >= 50 || LDRveri_2 >= 50){
      stop(); 
    }else{
      // Eğer önde engel yoksa ileri git ve sağa dön
      if (HCSRuzaklik_1 >= 10 && HCSRuzaklik_2 >= 1) {
        otoIleri(1000);
        otoSaga(500);
      } 
      // Eğer sağda engel varsa geri git ve yön değiştir
      else if (HCSRuzaklik_1 < 10 && HCSRuzaklik_2 >= 10) {
        otoGeri(1000);
        otoSaga(500);
        otoGeri(1000);
        otoSaga(500);
      }
      // Eğer solda engel varsa sola dönüp devam et
      else if (HCSRuzaklik_1 >= 10 && HCSRuzaklik_2 < 10) {
        otoSola(500);
        otoIleri(1000);
      }
      // Eğer her iki tarafta da engel varsa sola dönüp devam et
      else if (HCSRuzaklik_1 < 10 && HCSRuzaklik_2 < 10) {
        otoSola(500);
        otoIleri(1000);
      }
    }
  }else{
    // Eğer önde engel yoksa ileri git ve sağa dön
    if (HCSRuzaklik_1 >= 10 && HCSRuzaklik_2 >= 1) {
      otoIleri(1000);
      otoSaga(500);
     }  
     // Eğer sağda engel varsa geri git ve yön değiştir
    else if (HCSRuzaklik_1 < 10 && HCSRuzaklik_2 >= 10) {
       otoGeri(1000);
      otoSaga(500);
      otoGeri(1000);
      otoSaga(500);
    }
    // Eğer solda engel varsa sola dönüp devam et
    else if (HCSRuzaklik_1 >= 10 && HCSRuzaklik_2 < 10) {
      otoSola(500);
      otoIleri(1000);
    }
    // Eğer her iki tarafta da engel varsa sola dönüp devam et
    else if (HCSRuzaklik_1 < 10 && HCSRuzaklik_2 < 10) {
      otoSola(500);
      otoIleri(1000);
    }

    
  }

  
}


//DHT11 Verilerini Okuma Kodları
void DHTread() {
    if (millis() - lastDHTReadTime > DHT_READ_INTERVAL) {
        lastDHTReadTime = millis();  // Son okuma zamanını güncelle
        
        int temperature = 0;
        int humidity = 0;
        int result = dht11.readTemperatureHumidity(temperature, humidity);

        if (result == 0) {
            airTemperature = temperature;
            airHumidity = humidity;
        } else {
            airTemperature = 0;
            airHumidity = 0;
        }
    }
}

//Shift Register Veri Gönderimi
void shiftOutData(byte data) {
  // Latch pinini LOW yaparak veri girişine hazırlık yapılır
  digitalWrite(SRlatchPin, LOW);

  // Veriler 74HC595'e gönderilir
  shiftOut(SRdataPin, SRclockPin, MSBFIRST, data);

  // Latch pinini HIGH yaparak veri çıkışı aktive edilir
  digitalWrite(SRlatchPin, HIGH);
}

void akim() {
  static unsigned long previousMillis = 0;  // Son çalıştırma zamanını tutan değişken
  const long interval = 1000;               // 1 saniye (1000 ms)

  unsigned long currentMillis = millis();   // Şu anki zamanı al

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;  // Zamanı güncelle
    
    int sensorValue = analogRead(A5);  // Voltaj bölücüsünden değeri oku
    float voltage = sensorValue * (5.0 / 1023.0);  // Okunan değeri voltaja çevir
  
    // Gerilim hesaplaması (Voltaj bölücü formülü)
    currentVoltage = voltage * ((10000.0 + 1000.0) / 1000.0); 
    float pilYuzde = currentVoltage / 8.4 * 100;


    // Gerilimi seri monitörde yazdır
    Serial.println ("Pil Gerilimi: " + (String)currentVoltage + " Pil Yüzdesi: %"+(String)pilYuzde);

    
  }
}




