#include <TimeLib.h>
#define BLYNK_PRINT Serial    
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <WidgetRTC.h>
WidgetLED PUMP(V0);  // Đèn trạng thái 
WidgetLED LAMP(V5);  // Đèn trạng thái đèn sưởi
#define SENSOR_FIRE A0 // Chân PE4 nối với cảm biến độ lửa
#define PUMP_PIN 12  //Bơm
#define LAMP_PIN 13  //Đèn
#define FIRE          20

#include "DHT.h" //thư viện Sensor
#define DHTTYPE DHT22   // DHT 22 
#define DHTPIN 00     // Chân DATA nối với D3
//TIMER
#define READ_BUTTONS_TM   1L   // Tương ứng với giây
#define READ_FIRE  5L  //Đọc cảm biến cháy
#define READ_AIR_DATA_TM  2L  //Đọc DHT
#define AUTO_CTRL_TM      20L  //Chế độ tư động
#define DISPLAY_DATA_TM   10L  //Gửi dữ liệu lên terminal
#define SEND_UP_DATA_TM   5L   //Gửi dữ liệu lên blynk
#define TIME              100L

bool timeOnOff      = false;
bool oldtimeOnOff;
bool isFirstConnect;
unsigned int TimeStart, TimeStop ;
byte dayStartSelect = 0;
byte dayStopSelect = 0 ;

int oldSecond, nowSecond;
#define timeShow V1
#define timeInput V2
#define Pinout   13
WidgetRTC rtc;


//Token Blynk và wifi
char auth[] = "ZrMwqtaIdKFmPxCp-xWqbAiNW4qlfMwG"; // Blynk token
char ssid[] = "Mất Wifi rồi"; //Tên wifi
char pass[] = "kiduynhat"; //Mật khẩu

// Biến lưu các giá trị cảm biến

int Datafire = 0;
float humDHT = 0;
float tempDHT = 0;
// Biến lưu trạng thái bơm
boolean pumpStatus = 0;




// Biến cho timer
long sampleTimingSeconds = 500; // ==> Thời gian đọc cảm biến (s)
long startTiming = 0;
long elapsedTime = 0;
// Khởi tạo timer
SimpleTimer timer;

// Khởi tạo cảm biến
DHT dht(DHTPIN, DHTTYPE);
void setup() {
    pinMode(PUMP_PIN, OUTPUT);
    pinMode(LAMP_PIN, OUTPUT);
    digitalWrite(Pinout, !timeOnOff);
    
    // Khởi tạo cổng serial baud 115200
    Serial.begin(115200);
    Blynk.begin(auth, ssid, pass);
    aplyCmd();
    PUMP.off();
    startTimers();
     dht.begin();
}
BLYNK_CONNECTED()
{
  Blynk.syncAll();
  rtc.begin();
}

BLYNK_WRITE(timeInput) { 
  Serial.println("Time Input"); 
  TimeInputParam t(param); 
  // Process start time 
  if (t.hasStartTime())
  { 
    TimeStart = t.getStartHour()*60 + t.getStartMinute();
  }
  else TimeStart = 0;

  if (t.hasStopTime()) { 
    TimeStop = t.getStopHour()*60 + t.getStopMinute(); 
  }
  else TimeStop = 0;

  dayStartSelect = 0;
  dayStopSelect  = 0;
  for (int i = 1; i <= 7; i++) 
    if (t.isWeekdaySelected(i)) 
      if (i == 7) { 
        bitWrite(dayStartSelect, 0, 1); 
        bitWrite(dayStopSelect, 1, 1); 
      }
      else { 
        bitWrite(dayStartSelect, i, 1);
      bitWrite(dayStopSelect, i+1, 1); 
      } 
  }
  String twoDigits(int digits) {
  if(digits < 10) return "0" + String(digits); 
else return String(digits); }


 void TimeAuto() { //so sánh để tính toán ngày giờ hiện thời và thời gian hẹn
  
  unsigned int times = hour()*60 + minute();
  byte today = weekday(); //the weekday now (Sunday is day 1, Monday is day 2) 
  if (TimeStart == TimeStop) { timeOnOff = false; } 
  else if (TimeStart < TimeStop)
  if (bitRead(dayStartSelect, today - 1))
  if ((TimeStart <= times) && (times < TimeStop)) timeOnOff = true; 
      else timeOnOff = false; 
    else timeOnOff = false; 
  else if (TimeStart > TimeStop) 
  { 
    if ((TimeStop <= times) && (times < TimeStart)) timeOnOff = false;
    else if ((TimeStart <= times) && bitRead(dayStartSelect, today - 1)) timeOnOff = true; 
    else if ((TimeStop > times) && bitRead(dayStopSelect, today)) timeOnOff = true; } }


/* Hàm hiển thị thời gian thực */
 void showTime() { 
 nowSecond = second(); 
 if (oldSecond != nowSecond) 
 { 
  oldSecond = nowSecond; 
  String currentTime;
 if (isPM()) currentTime = twoDigits(hourFormat12()) + ":" + twoDigits(minute()) + ":" + twoDigits(second()) + " PM"; //hiển thị thời gian chiều tối
 else  currentTime = twoDigits(hourFormat12()) + ":" + twoDigits(minute()) + ":" + twoDigits(second()) + " AM"; //hiển thị thời gian sáng
 String currentDate = String(day()) + "/" + month() + "/" + year(); 
 Blynk.virtualWrite(timeShow, currentTime);
 Blynk.virtualWrite(V6, currentDate);

  if (oldtimeOnOff != timeOnOff) 
{ 
  if (timeOnOff) { //hẹn giờ
    LAMP.on();
    digitalWrite(Pinout, !timeOnOff); 
  Serial.println("Time schedule is ON"); } 
  else { LAMP.off(); 
  digitalWrite(Pinout, !timeOnOff); 
  Serial.println("Time schedule is OFF"); 
}  
    oldtimeOnOff = timeOnOff;
    }
  }
}

void loop() {
    timer.run(); // Bắt đầu SimpleTimer
    Blynk.run();  
    TimeAuto();
  showTime();
}
BLYNK_WRITE(3) // Điều khiển bơm trên Blynk
{
    int i = param.asInt();//hàm đọc giá trị analog trên Blynk
    if (i == 1) 
    {
      pumpStatus = !pumpStatus;
      aplyCmd();
    }
}

void getfire(void) //hàm đọc cảm biến lửa
{
    int i = 0;
    Datafire = 0;
    for (i = 0; i < 10; i++) 
    { 
      Datafire += analogRead(SENSOR_FIRE); //Đọc giá trị cảm biến 
      delay(50);   // Đợi đọc giá trị ADC
    }
    Datafire = Datafire / (i);
    Datafire = map(Datafire, 1023, 0, 0, 100); //Ít :0%  ==> Nhiều 100%
}
//hàm đọc giá trị cảm biến nhiệt độ độ ẩm
void getDhtData(void) 
{

    tempDHT = dht.readTemperature();//đọc nhiệt độ
    humDHT = dht.readHumidity();//đọc độ ẩm
    if (isnan(humDHT)||isnan(tempDHT))   // Kiểm tra kết nối lỗi thì thông báo.
    {
      Serial.println("Chua doc duoc gia tri sensor");
      return;
    }
}
void printData(void)
{
    // IN thông tin ra màn hình
    Serial.print("Do am: ");
    Serial.print(humDHT);
    Serial.print(" %\t");
    Serial.print("Nhiet do: ");
    Serial.print(tempDHT);
    Serial.print(" *C\t");
    Serial.print(" %\t");
     Serial.print("Tỷ lệ có cháy: ");
    Serial.print(Datafire);
    Serial.println(" %");
}


/***************************************************
  Thực hiện điều khiển bơm 
****************************************************/
void aplyCmd()
{
  if (pumpStatus == 1) 
  {
    digitalWrite(PUMP_PIN, LOW);//bật máy bơm
    PUMP.on();
  }
  else {
    digitalWrite(PUMP_PIN, HIGH);//tắt máy bơm
    PUMP.off();
  }
}
 /***************************************************
 * Hàm tự động
 **************************************************/

void autoControlPlantation(void)
{
    if (Datafire > FIRE) //nếu giá trị cảm biến lớn hơn giá trị ban đầu => có cháy
    {
   
      turnPumpOn(); //bật máy bơm
    }
    if(Datafire < FIRE){ //nếu giá trị cảm biến nhỏ hơn giá trị ban đầu =>hết cháy
      pumpStatus = 0; //tắt máy bơm
      aplyCmd(); 
    }
}

void turnPumpOn()
{
    pumpStatus = 1;
    aplyCmd();
   
}
/***************************************************
 * Khởi tạo timer
 **************************************************/
void startTimers(void)
{  
  timer.setInterval(READ_FIRE * 1000, getfire);
    timer.setInterval(AUTO_CTRL_TM * 1000, autoControlPlantation);
    timer.setInterval(READ_AIR_DATA_TM * 1000, getDhtData); 
    timer.setInterval(SEND_UP_DATA_TM * 1000, sendUptime);
    timer.setInterval(DISPLAY_DATA_TM * 500,printData);
}
/***************************************************
 * Gửi dữ liệu lên Blynk
 **************************************************/
 
void sendUptime()
{
    Blynk.virtualWrite(10, tempDHT); //Nhiệt độ với pin V10
    Blynk.virtualWrite(11, humDHT); // Độ ẩm với pin V11
    Blynk.virtualWrite(12, Datafire); // Báo cháy với V12
}
