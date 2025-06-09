#include <HardwareSerial.h>
#include <WiFi.h>              // 添加WiFi库
#include "time.h"              // 添加时间库

// 使用硬件串口2与HC-12通信
HardwareSerial SerialHC12(2);
// 接受飞控信号的UART引脚
HardwareSerial SerialHC13(1);

#define Ctr_RX_PIN 18  
#define Ctr_TX_PIN 19


// 定义HC-12的TX和RX引脚
#define HC12_RX_PIN 16  // ESP32的RX2引脚连接HC-12的TX
#define HC12_TX_PIN 17  // ESP32的TX2引脚连接HC-12的RX
#define time 30
int now = 0;

// NTP服务器配置
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 8 * 3600; // 北京时间UTC+8
const int   daylightOffset_sec = 0;   // 不使用夏令时

// 声明timeinfo结构体
struct tm timeinfo;

String checkWord = "11011101";// 发送信号指令

void setup() {
  // 启动调试串口
  Serial.begin(115200);
  
  // 启动与HC-12通信的串口
  SerialHC12.begin(9600, SERIAL_8N1, HC12_RX_PIN, HC12_TX_PIN);

  SerialHC13.begin(9600, SERIAL_8N1, Ctr_RX_PIN, Ctr_TX_PIN);
  
  // 设置HC-12模块到473MHz
  delay(100); // 等待模块初始化
  SerialHC12.print("AT+FU3"); // 设置频率为473MHz (FU3对应473MHz)
  delay(100); // 等待命令执行
  
  // 连接WiFi并获取时间
  Serial.println("连接WiFi获取时间...");
  WiFi.begin("SolarSS", "123456aa"); // 修改为你的WiFi信息
  while (WiFi.status() != WL_CONNECTED) {

    delay(500);
    Serial.print(".");
  }
  Serial.println(" WiFi连接成功");
  
  // 初始化并获取NTP时间
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  while(!getLocalTime(&timeinfo)){
    Serial.println("获取时间失败，重试中...");
    delay(1000);
  }
  
  Serial.println("ESP32 HC-12发送端初始化完成");
  Serial.println("准备发送日期时间数据...");


}

void loop() {
  if(!getLocalTime(&timeinfo)){
    Serial.println("获取时间失败");
    return;
  }
  
  // 格式化日期时间字符串
  char timeString[64];
  strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", &timeinfo);
  String tmp = "Team 54 mission accomplished";


  if (SerialHC13.available()) {
    String receivedWord = SerialHC13.readStringUntil('\n');
    receivedWord.trim(); // 去除可能的空白字符
    if (receivedWord== checkWord){// 收到指令

      Serial.println(receivedWord);
      
     SerialHC12.println(tmp);
     SerialHC12.println(timeString);
    // // Serial.print("已发送: ");
    // Serial.println(tmp);
    // Serial.println(timeString);
    // delay(1000);
    }
  }

  delay(2000);
  
  // 短暂延迟以减少处理负载
  // delay(100);

  // SerialHC12.println(tmp);
  // SerialHC12.println(timeString);
  // // Serial.print("已发送: ");
  // Serial.println(tmp);
  
  // Serial.println(timeString);
  // delay(1000);


  
}