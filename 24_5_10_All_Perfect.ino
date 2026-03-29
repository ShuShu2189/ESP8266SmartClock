#include <NTPClient.h>
#include <ESP8266WiFi.h>  // 本程序使用 ESP8266WiFi库
#include <WiFiUdp.h>
#include <DNSServer.h>
#include <ESP8266WiFiMulti.h>  //  ESP8266WiFiMulti库
#include <ESP8266WebServer.h>  //  ESP8266WebServer库
#include <WiFiManager.h>
#include <Time.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>
#include <RTClib.h>
#include <stdio.h>
#include <stdlib.h>  //字符串转整形+数组转整形

int userPIN = 1234;             // 默认 PIN 码是 1234
boolean authenticated = false;  // 初始化认证状态为否

boolean RingStatus = true;                 // 默认开启响铃状态
boolean ManualTimeReport_status = false;  // 默认关闭手动响铃

WiFiUDP ntpUDP;
#define NTP_ADDRESS "ntp.aliyun.com"  // NTP服务器
#define NTP_OFFSET 8 * 60 * 60        // 时区偏移量8小时,60分钟,60秒
NTPClient timeClient(ntpUDP, NTP_OFFSET);

LiquidCrystal_I2C lcd(0x27, 16, 2);  // 显示屏初始化
char Time[] = "TIME 00:00:00";
char Date[] = "DATE 2000/01/01";
char DOW[] = "M1";

ThreeWire myWire(12, 13, 14);  // 初始化DS1302(DAT=12, CLK=13, RST=14)
RtcDS1302<ThreeWire> Rtc(myWire);


WiFiManager WiFiManager;      // 建立WiFiManager对象,对象名称是 'WiFiManager'
ESP8266WiFiMulti wifiMulti;   // 建立ESP8266WiFiMulti对象,对象名称是 'wifiMulti'
ESP8266WebServer server(80);  // 创建WebServer，监听80端口

void HandleRoot() {
  String html = "<html><body>";
  html += "<h1>ESP8266 Remote Control</h1>";
  html += "<form method='post' action='/login'>";
  html += "<input type='text' name='pin' placeholder='Enter PIN'>";
  html += "<input type='submit' value='Login'>";
  html += "</form>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void HandleLogin() {
  int pin = server.arg("pin").toInt();
  if (pin == userPIN) {
    authenticated = true;
    server.sendHeader("Location", "/Main");
    server.send(302);
  } else {
    server.send(200, "text/plain", "Incorrect PIN");
  }
}

//处理状态设定控制请求的函数'HandleMain'
void HandleMain() {
  if (!authenticated) {
    server.send(401, "text/plain", "Not authenticated");
    return;
  }
  String html = "<html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'><title>Esp8266远程控制</title></head><body>";
  html += "<h1>ESP8266 Remote Control</h1>";
  html += "<h2>你已连接至ESP8266智能时钟!</h2>";
  html += "<h3>tip:若内置LED亮起,则会整点报时</h3>";

  html += "<div class='indent'>";
  html += "<label>整点报时设定</label>";  //仅测试
  html += "<form method='post' action='/RingStatus'>";
  html += "<input type='submit' name='button' value='设定' class='button'>";
  html += "</form>";
  html += "</div>";

  html += "<div class='indent'>";
  html += "<label>时间状态设定</label>";
  html += "<form method='post' action='/TimeSet'>";
  html += "<input type='submit' name='button' value='设定' class='button'>";
  html += "</form>";
  html += "</div>";

  html += "<div class='indent'>";
  html += "<label>还原时间</label>";
  html += "<form method='post' action='/TimeReSet'>";
  html += "<input type='submit' name='button' value='还原' class='button'>";
  html += "</form>";
  html += "</div>";

  html += "<div class='indent'>";
  html += "<label>立即报时（仅小时）</label>";
  html += "<form method='post' action='/HandleManualTimeReport'>";
  html += "<input type='submit' name='button' value='报时' class='button'>";
  html += "</form>";
  html += "</div>";

  html += "<div class='indent'>";
  html += "<label>还原网络数据</label>";
  html += "<form method='post' action='/NetWorkReSet'>";
  html += "<input type='submit' name='button' value='还原' class='button'>";
  html += "</form>";
  html += "</div>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

//处理响铃控制请求的函数'HandleRingStatus'
void HandleRingStatus() {
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));  // 改变LED的点亮或者熄灭状态
  RingStatus = !RingStatus;
  // tone(buzzPin, frequency);  // 先响一声表示状态
  // delay(500);
  // noTone(buzzPin);
  // delay(500);
  server.sendHeader("Location", "/Main");  // 跳转回页面根目录
  server.send(302);                        // 发送Http相应代码303回应
}

//处理时间设定控制请求的函数'handleTimeSet'
void HandleTimeSet() {
  //设置NTPClient
  WiFiUDP ntpUDP;  //UDP:不保证可靠交付
  //NTPClient timeClient(ntpUDP, NTP_ADDRESS, NTP_OFFSET);

  // 初始化timeClient
  timeClient.update();
  unsigned long epochTime = timeClient.getEpochTime();

  time_t mytime = epochTime;  // 时间戳，可以用time(&mytime);获取当前时间戳
  struct tm* timeinfo;
  char NTP_Year_[4], NTP_Month_[2], NTP_Day_[2];
  int NTP_Year, NTP_Month, NTP_Day;
  char buffer[30];

  timeinfo = localtime(&mytime);  // 转换
  strftime(buffer, sizeof(buffer), "%Y%m%d", timeinfo);
  //Serial.println(buffer);

  char str[10];  // 输入的字符串
  strcpy(str, buffer);
  char part1[5], part2[3], part3[3];  // 分别存放三个部分的字符串
  int len = strlen(str);              // 获取输入字符串的长度

  // 复制第一部分, 年, 共4个字符
  memcpy(part1, str, 4);
  part1[4] = '\0';
  int part1_ = atoi(part1);

  // 复制第二部分, 月, 共两个字符
  memcpy(part2, str + 4, 2);
  part2[2] = '\0';
  int part2_ = atoi(part2);

  // 复制第三部分, 日, 共两个字符
  memcpy(part3, str + 6, 2);
  part3[2] = '\0';
  int part3_ = atoi(part3);

  NTP_Year = part1_;
  NTP_Month = part2_;
  NTP_Day = part3_;

  RtcDateTime DStime_update(NTP_Year, NTP_Month, NTP_Day, timeClient.getHours(), timeClient.getMinutes(), timeClient.getSeconds());
  Rtc.SetDateTime(DStime_update);
  //delay(500);
  server.sendHeader("Location", "/Main");  // 跳转回页面根目录
  server.send(303);                        // 发送Http相应代码303 跳转
}

// 重设时间为2000/01/01 00:00:00
void HandleTimeReSet() {
  RtcDateTime DStime_update(2000, 1, 1, 0, 0, 0);
  Rtc.SetDateTime(DStime_update);
  // BuzzerRing();
  server.sendHeader("Location", "/Main");
  server.send(302);
}

// 清空所保存的网络数据
void HandleNetWorkReSet() {
  WiFiManager.resetSettings();  //仅测试：清除保存的密钥
  // BuzzerRing();
  server.sendHeader("Location", "/Main");
  server.send(302);
  ESP.reset();
}

// 发送HTTP状态404
void HandleNotFound() {
  server.send(404, "text/plain", "404: Not found");
}

void send_data(int addr) {  //语音模块的预声明部分
  int i;
  digitalWrite(16, LOW);
  delay(3);  //需要至少>2ms，因为NV080C需要2ms来启动
  for (i = 0; i < 8; i++) {
    digitalWrite(16, HIGH);
    if (addr & 1) {
      delayMicroseconds(2400);  //>2400us
      digitalWrite(16, LOW);
      delayMicroseconds(800);
    }  //>800us
    else {
      delayMicroseconds(800);  //>800us
      digitalWrite(16, LOW);
      delayMicroseconds(2400);
    }  //>2400us
    addr >>= 1;
  }  //地址值右移一位
  digitalWrite(16, HIGH);
}

void HandleManualTimeReport() {
  ManualTimeReport_status = true;
  server.sendHeader("Location", "/Main");
  server.send(302);
}

void GetTime(const RtcDateTime& dt) {      //LCD1602显示时间
  unsigned long currentMillis = millis();  // 获取当前时间
  unsigned long lastPrint = 0;             // 记录上次打印的时间
  char datestring[20];
  snprintf_P(datestring, countof(datestring), PSTR("%04u/%02u/%02u %02u:%02u:%02u"), dt.Year(), dt.Month(), dt.Day(), dt.Hour(), dt.Minute(), dt.Second());  //PSTR=PointerString
  Date[7] = datestring[2];
  Date[8] = datestring[3];
  Date[10] = datestring[5];
  Date[11] = datestring[6];
  Date[13] = datestring[8];
  Date[14] = datestring[9];
  Time[5] = datestring[11];
  Time[6] = datestring[12];
  Time[8] = datestring[14];
  Time[9] = datestring[15];
  Time[11] = datestring[17];
  Time[12] = datestring[18];

  char TimeClientGetDay = timeClient.getDay();  // Day Of Week, 星期几
  if (TimeClientGetDay == 0) {
    DOW[0] = 'S';
    DOW[1] = '7';
  } else if (TimeClientGetDay == 1) {
    DOW[0] = 'M';
    DOW[1] = '1';
  } else if (TimeClientGetDay == 2) {
    DOW[0] = 'T';
    DOW[1] = '2';
  } else if (TimeClientGetDay == 3) {
    DOW[0] = 'W';
    DOW[1] = '3';
  } else if (TimeClientGetDay == 4) {
    DOW[0] = 'T';
    DOW[1] = '4';
  } else if (TimeClientGetDay == 5) {
    DOW[0] = 'F';
    DOW[1] = '5';
  } else if (TimeClientGetDay == 6) {
    DOW[0] = 'S';
    DOW[1] = '6';
  } else {
    DOW[0] = '?';
    DOW[1] = '?';
  }

  //lcd.clear();  //清屏
  if (currentMillis - lastPrint >= 1000) {  // 如果已经过去了1秒
    lcd.setCursor(0, 0);
    lcd.print(Time);
    lcd.setCursor(0, 1);
    lcd.print(Date);
    lcd.setCursor(14, 0);
    lcd.print(DOW);
    lastPrint = currentMillis;  // 更新上次打印的时间
  }

  // if (RingStatus == true && dt.Minute() == 0 && dt.Second() == 0) {  // 整点报时
  //   BuzzerRing();
  // }

  if (RingStatus == true && dt.Minute() == 0 && dt.Second() == 0) {  // 整点报时
    if (dt.Hour() == 0) {
      send_data(0x14);
      delay(1500);  //您好

      send_data(0x33);
      delay(1000);  //现在

      send_data(0x34);
      delay(1000);  //是

      send_data(0x0b);
      delay(1000);  //上午

      send_data(0x2c);
      delay(1000);  //0

      send_data(0x1d);
      delay(1000);  //点
    } else if (dt.Hour() == 1) {
      send_data(0x14);
      delay(1500);  //您好

      send_data(0x33);
      delay(1000);  //现在

      send_data(0x34);
      delay(1000);  //是

      send_data(0x0b);
      delay(1000);  //上午

      send_data(0x23);
      delay(1000);  //1

      send_data(0x1d);
      delay(1000);  //点
    } else if (dt.Hour() == 2) {
      send_data(0x14);
      delay(1500);  //您好

      send_data(0x33);
      delay(1000);  //现在

      send_data(0x34);
      delay(1000);  //是

      send_data(0x0b);
      delay(1000);  //上午

      send_data(0x24);
      delay(1000);  //2

      send_data(0x1d);
      delay(1000);  //点
    } else if (dt.Hour() == 3) {
      send_data(0x14);
      delay(1500);  //您好

      send_data(0x33);
      delay(1000);  //现在

      send_data(0x34);
      delay(1000);  //是

      send_data(0x0b);
      delay(1000);  //上午

      send_data(0x25);
      delay(1000);  //3

      send_data(0x1d);
      delay(1000);  //点
    } else if (dt.Hour() == 4) {
      send_data(0x14);
      delay(1500);  //您好

      send_data(0x33);
      delay(1000);  //现在

      send_data(0x34);
      delay(1000);  //是

      send_data(0x0b);
      delay(1000);  //上午

      send_data(0x26);
      delay(1000);  //4

      send_data(0x1d);
      delay(1000);  //点
    } else if (dt.Hour() == 5) {
      send_data(0x14);
      delay(1500);  //您好

      send_data(0x33);
      delay(1000);  //现在

      send_data(0x34);
      delay(1000);  //是

      send_data(0x0b);
      delay(1000);  //上午

      send_data(0x27);
      delay(1000);  //5

      send_data(0x1d);
      delay(1000);  //点
    } else if (dt.Hour() == 6) {
      send_data(0x14);
      delay(1500);  //您好

      send_data(0x33);
      delay(1000);  //现在

      send_data(0x34);
      delay(1000);  //是

      send_data(0x0b);
      delay(1000);  //上午

      send_data(0x28);
      delay(1000);  //6

      send_data(0x1d);
      delay(1000);  //点
    } else if (dt.Hour() == 7) {
      send_data(0x14);
      delay(1500);  //您好

      send_data(0x33);
      delay(1000);  //现在

      send_data(0x34);
      delay(1000);  //是

      send_data(0x0b);
      delay(1000);  //上午

      send_data(0x29);
      delay(1000);  //7

      send_data(0x1d);
      delay(1000);  //点
    } else if (dt.Hour() == 8) {
      send_data(0x14);
      delay(1500);  //您好

      send_data(0x33);
      delay(1000);  //现在

      send_data(0x34);
      delay(1000);  //是

      send_data(0x0b);
      delay(1000);  //上午

      send_data(0x2a);
      delay(1000);  //8

      send_data(0x1d);
      delay(1000);  //点
    } else if (dt.Hour() == 9) {
      send_data(0x14);
      delay(1500);  //您好

      send_data(0x33);
      delay(1000);  //现在

      send_data(0x34);
      delay(1000);  //是

      send_data(0x0b);
      delay(1000);  //上午

      send_data(0x2b);
      delay(1000);  //9

      send_data(0x1d);
      delay(1000);  //点
    } else if (dt.Hour() == 10) {
      send_data(0x14);
      delay(1500);  //您好

      send_data(0x33);
      delay(1000);  //现在

      send_data(0x34);
      delay(1000);  //是

      send_data(0x0b);
      delay(1000);  //上午

      send_data(0x22);
      delay(1000);  //十

      send_data(0x1d);
      delay(1000);  //点
    } else if (dt.Hour() == 11) {
      send_data(0x14);
      delay(1500);  //您好

      send_data(0x33);
      delay(1000);  //现在

      send_data(0x34);
      delay(1000);  //是

      send_data(0x0b);
      delay(1000);  //上午

      send_data(0x22);
      delay(1000);  //十

      send_data(0x23);
      delay(1000);  //1

      send_data(0x1d);
      delay(1000);  //点
    } else if (dt.Hour() == 12) {
      send_data(0x14);
      delay(1500);  //您好

      send_data(0x33);
      delay(1000);  //现在

      send_data(0x34);
      delay(1000);  //是

      send_data(0x0c);
      delay(1000);  //下午

      send_data(0x22);
      delay(1000);  //十

      send_data(0x24);
      delay(1000);  //2

      send_data(0x1d);
      delay(1000);  //点
    } else if (dt.Hour() == 13) {
      send_data(0x14);
      delay(1500);  //您好

      send_data(0x33);
      delay(1000);  //现在

      send_data(0x34);
      delay(1000);  //是

      send_data(0x0c);
      delay(1000);  //下午

      send_data(0x22);
      delay(1000);  //十

      send_data(0x25);
      delay(1000);  //3

      send_data(0x1d);
      delay(1000);  //点
    } else if (dt.Hour() == 14) {
      send_data(0x14);
      delay(1500);  //您好

      send_data(0x33);
      delay(1000);  //现在

      send_data(0x34);
      delay(1000);  //是

      send_data(0x0c);
      delay(1000);  //下午

      send_data(0x22);
      delay(1000);  //十

      send_data(0x26);
      delay(1000);  //4

      send_data(0x1d);
      delay(1000);  //点
    } else if (dt.Hour() == 15) {
      send_data(0x14);
      delay(1500);  //您好

      send_data(0x33);
      delay(1000);  //现在

      send_data(0x34);
      delay(1000);  //是

      send_data(0x0c);
      delay(1000);  //下午

      send_data(0x22);
      delay(1000);  //十

      send_data(0x27);
      delay(1000);  //5

      send_data(0x1d);
      delay(1000);  //点
    } else if (dt.Hour() == 16) {
      send_data(0x14);
      delay(1500);  //您好

      send_data(0x33);
      delay(1000);  //现在

      send_data(0x34);
      delay(1000);  //是

      send_data(0x0c);
      delay(1000);  //下午

      send_data(0x22);
      delay(1000);  //十

      send_data(0x28);
      delay(1000);  //6

      send_data(0x1d);
      delay(1000);  //点
    } else if (dt.Hour() == 17) {
      send_data(0x14);
      delay(1500);  //您好

      send_data(0x33);
      delay(1000);  //现在

      send_data(0x34);
      delay(1000);  //是

      send_data(0x0c);
      delay(1000);  //下午

      send_data(0x22);
      delay(1000);  //十

      send_data(0x29);
      delay(1000);  //7

      send_data(0x1d);
      delay(1000);  //点
    } else if (dt.Hour() == 18) {
      send_data(0x14);
      delay(1500);  //您好

      send_data(0x33);
      delay(1000);  //现在

      send_data(0x34);
      delay(1000);  //是

      send_data(0x0c);
      delay(1000);  //下午

      send_data(0x22);
      delay(1000);  //十

      send_data(0x2a);
      delay(1000);  //8

      send_data(0x1d);
      delay(1000);  //点
    } else if (dt.Hour() == 19) {
      send_data(0x14);
      delay(1500);  //您好

      send_data(0x33);
      delay(1000);  //现在

      send_data(0x34);
      delay(1000);  //是

      send_data(0x0c);
      delay(1000);  //下午

      send_data(0x22);
      delay(1000);  //十

      send_data(0x2b);
      delay(1000);  //9

      send_data(0x1d);
      delay(1000);  //点
    } else if (dt.Hour() == 20) {
      send_data(0x14);
      delay(1500);  //您好

      send_data(0x33);
      delay(1000);  //现在

      send_data(0x34);
      delay(1000);  //是

      send_data(0x0c);
      delay(1000);  //下午

      send_data(0x24);
      delay(1000);  //2

      send_data(0x22);
      delay(1000);  //十

      send_data(0x1d);
      delay(1000);  //点
    } else if (dt.Hour() == 21) {
      send_data(0x14);
      delay(1500);  //您好

      send_data(0x33);
      delay(1000);  //现在

      send_data(0x34);
      delay(1000);  //是

      send_data(0x0c);
      delay(1000);  //下午

      send_data(0x24);
      delay(1000);  //2

      send_data(0x22);
      delay(1000);  //十

      send_data(0x23);
      delay(1000);  //1

      send_data(0x1d);
      delay(1000);  //点
    } else if (dt.Hour() == 22) {
      send_data(0x14);
      delay(1500);  //您好

      send_data(0x33);
      delay(1000);  //现在

      send_data(0x34);
      delay(1000);  //是

      send_data(0x0c);
      delay(1000);  //下午

      send_data(0x24);
      delay(1000);  //2

      send_data(0x22);
      delay(1000);  //十

      send_data(0x24);
      delay(1000);  //2

      send_data(0x1d);
      delay(1000);  //点
    } else if (dt.Hour() == 23) {
      send_data(0x14);
      delay(1500);  //您好

      send_data(0x33);
      delay(1000);  //现在

      send_data(0x34);
      delay(1000);  //是

      send_data(0x0c);
      delay(1000);  //下午

      send_data(0x24);
      delay(1000);  //2

      send_data(0x22);
      delay(1000);  //十

      send_data(0x25);
      delay(1000);  //3

      send_data(0x1d);
      delay(1000);  //点
    } else if (dt.Hour() == 24) {
      send_data(0x14);
      delay(1500);  //您好

      send_data(0x33);
      delay(1000);  //现在

      send_data(0x34);
      delay(1000);  //是

      send_data(0x0c);
      delay(1000);  //下午

      send_data(0x24);
      delay(1000);  //2

      send_data(0x22);
      delay(1000);  //十

      send_data(0x26);
      delay(1000);  //4

      send_data(0x1d);
      delay(1000);  //点
    } else {
      send_data(0x10);
      delay(1000);  //如果出错：新年快乐
    }
  }

  if (ManualTimeReport_status == true) {  // 整点报时
    ManualTimeReport_status = false;
    if (dt.Hour() == 0) {
      send_data(0x14);
      delay(1500);  //您好

      send_data(0x33);
      delay(1000);  //现在

      send_data(0x34);
      delay(1000);  //是

      send_data(0x0b);
      delay(1000);  //上午

      send_data(0x2c);
      delay(1000);  //0

      send_data(0x1d);
      delay(1000);  //点
    } else if (dt.Hour() == 1) {
      send_data(0x14);
      delay(1500);  //您好

      send_data(0x33);
      delay(1000);  //现在

      send_data(0x34);
      delay(1000);  //是

      send_data(0x0b);
      delay(1000);  //上午

      send_data(0x23);
      delay(1000);  //1

      send_data(0x1d);
      delay(1000);  //点
    } else if (dt.Hour() == 2) {
      send_data(0x14);
      delay(1500);  //您好

      send_data(0x33);
      delay(1000);  //现在

      send_data(0x34);
      delay(1000);  //是

      send_data(0x0b);
      delay(1000);  //上午

      send_data(0x24);
      delay(1000);  //2

      send_data(0x1d);
      delay(1000);  //点
    } else if (dt.Hour() == 3) {
      send_data(0x14);
      delay(1500);  //您好

      send_data(0x33);
      delay(1000);  //现在

      send_data(0x34);
      delay(1000);  //是

      send_data(0x0b);
      delay(1000);  //上午

      send_data(0x25);
      delay(1000);  //3

      send_data(0x1d);
      delay(1000);  //点
    } else if (dt.Hour() == 4) {
      send_data(0x14);
      delay(1500);  //您好

      send_data(0x33);
      delay(1000);  //现在

      send_data(0x34);
      delay(1000);  //是

      send_data(0x0b);
      delay(1000);  //上午

      send_data(0x26);
      delay(1000);  //4

      send_data(0x1d);
      delay(1000);  //点
    } else if (dt.Hour() == 5) {
      send_data(0x14);
      delay(1500);  //您好

      send_data(0x33);
      delay(1000);  //现在

      send_data(0x34);
      delay(1000);  //是

      send_data(0x0b);
      delay(1000);  //上午

      send_data(0x27);
      delay(1000);  //5

      send_data(0x1d);
      delay(1000);  //点
    } else if (dt.Hour() == 6) {
      send_data(0x14);
      delay(1500);  //您好

      send_data(0x33);
      delay(1000);  //现在

      send_data(0x34);
      delay(1000);  //是

      send_data(0x0b);
      delay(1000);  //上午

      send_data(0x28);
      delay(1000);  //6

      send_data(0x1d);
      delay(1000);  //点
    } else if (dt.Hour() == 7) {
      send_data(0x14);
      delay(1500);  //您好

      send_data(0x33);
      delay(1000);  //现在

      send_data(0x34);
      delay(1000);  //是

      send_data(0x0b);
      delay(1000);  //上午

      send_data(0x29);
      delay(1000);  //7

      send_data(0x1d);
      delay(1000);  //点
    } else if (dt.Hour() == 8) {
      send_data(0x14);
      delay(1500);  //您好

      send_data(0x33);
      delay(1000);  //现在

      send_data(0x34);
      delay(1000);  //是

      send_data(0x0b);
      delay(1000);  //上午

      send_data(0x2a);
      delay(1000);  //8

      send_data(0x1d);
      delay(1000);  //点
    } else if (dt.Hour() == 9) {
      send_data(0x14);
      delay(1500);  //您好

      send_data(0x33);
      delay(1000);  //现在

      send_data(0x34);
      delay(1000);  //是

      send_data(0x0b);
      delay(1000);  //上午

      send_data(0x2b);
      delay(1000);  //9

      send_data(0x1d);
      delay(1000);  //点
    } else if (dt.Hour() == 10) {
      send_data(0x14);
      delay(1500);  //您好

      send_data(0x33);
      delay(1000);  //现在

      send_data(0x34);
      delay(1000);  //是

      send_data(0x0b);
      delay(1000);  //上午

      send_data(0x22);
      delay(1000);  //十

      send_data(0x1d);
      delay(1000);  //点
    } else if (dt.Hour() == 11) {
      send_data(0x14);
      delay(1500);  //您好

      send_data(0x33);
      delay(1000);  //现在

      send_data(0x34);
      delay(1000);  //是

      send_data(0x0b);
      delay(1000);  //上午

      send_data(0x22);
      delay(1000);  //十

      send_data(0x23);
      delay(1000);  //1

      send_data(0x1d);
      delay(1000);  //点
    } else if (dt.Hour() == 12) {
      send_data(0x14);
      delay(1500);  //您好

      send_data(0x33);
      delay(1000);  //现在

      send_data(0x34);
      delay(1000);  //是

      send_data(0x0c);
      delay(1000);  //下午

      send_data(0x22);
      delay(1000);  //十

      send_data(0x24);
      delay(1000);  //2

      send_data(0x1d);
      delay(1000);  //点
    } else if (dt.Hour() == 13) {
      send_data(0x14);
      delay(1500);  //您好

      send_data(0x33);
      delay(1000);  //现在

      send_data(0x34);
      delay(1000);  //是

      send_data(0x0c);
      delay(1000);  //下午

      send_data(0x22);
      delay(1000);  //十

      send_data(0x25);
      delay(1000);  //3

      send_data(0x1d);
      delay(1000);  //点
    } else if (dt.Hour() == 14) {
      send_data(0x14);
      delay(1500);  //您好

      send_data(0x33);
      delay(1000);  //现在

      send_data(0x34);
      delay(1000);  //是

      send_data(0x0c);
      delay(1000);  //下午

      send_data(0x22);
      delay(1000);  //十

      send_data(0x26);
      delay(1000);  //4

      send_data(0x1d);
      delay(1000);  //点
    } else if (dt.Hour() == 15) {
      send_data(0x14);
      delay(1500);  //您好

      send_data(0x33);
      delay(1000);  //现在

      send_data(0x34);
      delay(1000);  //是

      send_data(0x0c);
      delay(1000);  //下午

      send_data(0x22);
      delay(1000);  //十

      send_data(0x27);
      delay(1000);  //5

      send_data(0x1d);
      delay(1000);  //点
    } else if (dt.Hour() == 16) {
      send_data(0x14);
      delay(1500);  //您好

      send_data(0x33);
      delay(1000);  //现在

      send_data(0x34);
      delay(1000);  //是

      send_data(0x0c);
      delay(1000);  //下午

      send_data(0x22);
      delay(1000);  //十

      send_data(0x28);
      delay(1000);  //6

      send_data(0x1d);
      delay(1000);  //点
    } else if (dt.Hour() == 17) {
      send_data(0x14);
      delay(1500);  //您好

      send_data(0x33);
      delay(1000);  //现在

      send_data(0x34);
      delay(1000);  //是

      send_data(0x0c);
      delay(1000);  //下午

      send_data(0x22);
      delay(1000);  //十

      send_data(0x29);
      delay(1000);  //7

      send_data(0x1d);
      delay(1000);  //点
    } else if (dt.Hour() == 18) {
      send_data(0x14);
      delay(1500);  //您好

      send_data(0x33);
      delay(1000);  //现在

      send_data(0x34);
      delay(1000);  //是

      send_data(0x0c);
      delay(1000);  //下午

      send_data(0x22);
      delay(1000);  //十

      send_data(0x2a);
      delay(1000);  //8

      send_data(0x1d);
      delay(1000);  //点
    } else if (dt.Hour() == 19) {
      send_data(0x14);
      delay(1500);  //您好

      send_data(0x33);
      delay(1000);  //现在

      send_data(0x34);
      delay(1000);  //是

      send_data(0x0c);
      delay(1000);  //下午

      send_data(0x22);
      delay(1000);  //十

      send_data(0x2b);
      delay(1000);  //9

      send_data(0x1d);
      delay(1000);  //点
    } else if (dt.Hour() == 20) {
      send_data(0x14);
      delay(1500);  //您好

      send_data(0x33);
      delay(1000);  //现在

      send_data(0x34);
      delay(1000);  //是

      send_data(0x0c);
      delay(1000);  //下午

      send_data(0x24);
      delay(1000);  //2

      send_data(0x22);
      delay(1000);  //十

      send_data(0x1d);
      delay(1000);  //点
    } else if (dt.Hour() == 21) {
      send_data(0x14);
      delay(1500);  //您好

      send_data(0x33);
      delay(1000);  //现在

      send_data(0x34);
      delay(1000);  //是

      send_data(0x0c);
      delay(1000);  //下午

      send_data(0x24);
      delay(1000);  //2

      send_data(0x22);
      delay(1000);  //十

      send_data(0x23);
      delay(1000);  //1

      send_data(0x1d);
      delay(1000);  //点
    } else if (dt.Hour() == 22) {
      send_data(0x14);
      delay(1500);  //您好

      send_data(0x33);
      delay(1000);  //现在

      send_data(0x34);
      delay(1000);  //是

      send_data(0x0c);
      delay(1000);  //下午

      send_data(0x24);
      delay(1000);  //2

      send_data(0x22);
      delay(1000);  //十

      send_data(0x24);
      delay(1000);  //2

      send_data(0x1d);
      delay(1000);  //点
    } else if (dt.Hour() == 23) {
      send_data(0x14);
      delay(1500);  //您好

      send_data(0x33);
      delay(1000);  //现在

      send_data(0x34);
      delay(1000);  //是

      send_data(0x0c);
      delay(1000);  //下午

      send_data(0x24);
      delay(1000);  //2

      send_data(0x22);
      delay(1000);  //十

      send_data(0x25);
      delay(1000);  //3

      send_data(0x1d);
      delay(1000);  //点
    } else if (dt.Hour() == 24) {
      send_data(0x14);
      delay(1500);  //您好

      send_data(0x33);
      delay(1000);  //现在

      send_data(0x34);
      delay(1000);  //是

      send_data(0x0c);
      delay(1000);  //下午

      send_data(0x24);
      delay(1000);  //2

      send_data(0x22);
      delay(1000);  //十

      send_data(0x26);
      delay(1000);  //4

      send_data(0x1d);
      delay(1000);  //点
    } else {
      send_data(0x10);
      delay(1000);  //如果出错：新年快乐
    }
  }
}

void setup() {
  //初始化屏幕，显示开机欢迎界面
  char Welcome1[] = "SMART WIFI CLOCK";
  char Welcome2[] = "INITIALIZING...";
  lcd.init();
  lcd.clear();
  lcd.backlight();      //确保LCD背光已启动
  lcd.setCursor(0, 0);  //第0行第0个字符显示
  lcd.print(Welcome1);
  lcd.setCursor(0, 1);  //第1行第0个字符显示
  lcd.print(Welcome2);

  //启动RTC实时时钟模块
  Rtc.Begin();

  //初始化TTS文字转语音模块
  pinMode(16, OUTPUT);
  send_data(0x43);  //输出“世界这么大......”
  //volume control 0xE0-E7;

  //启动timeClient模块，准备与ntp校时
  timeClient.begin();

  // WiFiManager WiFiManager;
  IPAddress _ip = IPAddress(192, 168, 4, 1);  //设定网页：192.169.4.1
  IPAddress _gw = IPAddress(192, 168, 1, 1);
  IPAddress _sn = IPAddress(255, 255, 255, 0);
  WiFiManager.setSTAStaticIPConfig(_ip, _gw, _sn);
  WiFiManager.autoConnect("Smart WiFi Clock", "12345678");  //密码1-8

  // 处理用户连接和设置网页
  server.on("/", HandleRoot);
  server.on("/login", HandleLogin);                              //  设置处理登陆请求的函数'HandleLogin'
  server.on("/Main", HandleMain);                                //  设置跳转到主要控制界面请求的函数'HandleMain'
  server.on("/RingStatus", HTTP_POST, HandleRingStatus);         //  设置处理RingStatus控制请求的函数'HandleRingStatus'
  server.on("/TimeSet", HandleTimeSet);                          //  设置处理设定时间请求的函数'HandleTimeSet'
  server.on("/TimeReSet", HandleTimeReSet);                      //  设置处理还原时间设定请求的函数'HandleTimeReSet'
  server.on("/HandleManualTimeReport", HandleManualTimeReport);  //  设置处理立即报时请求的函数'HandleManualTimeReport'
  server.on("/NetWorkReSet", HandleNetWorkReSet);                //  设置处理还原网络设定请求的函数'HandleNetWorkReSet'
  server.onNotFound(HandleNotFound);                             //  设置处理404情况的函数'HandleNotFound'
  server.begin();                                                //  启动网站服务

  //设置内置LED引脚为输出模式以便控制LED
  pinMode(LED_BUILTIN, OUTPUT);

  //防止写入怪东西
  if (!Rtc.IsDateTimeValid()) {
    //新建RtcDateTime 对象，并写入日期时间，参数顺序为Year,Month,Day,Hour,Minutes,Seconds
    //新建RTC时间对象 - 2000年1月1日0点0分0秒（默认24小时制）
    RtcDateTime DStime_update(2000, 1, 1, 0, 0, 0);

    //调用SetDateTime函数为RTC模块写入时间
    Rtc.SetDateTime(DStime_update);
  }

  //网络详情界面
  lcd.init();
  lcd.clear();
  lcd.setCursor(0, 0);  //第0行第0个字符显示网络名称
  lcd.print("Local ip is");
  lcd.setCursor(0, 1);  //第1行第0个字符显示ip地址
  lcd.print(WiFi.localIP());
  delay(2000);

  //清空屏幕，准备显示时间
  lcd.clear();
}

void loop() {
  // 调用实时时钟模块显示时间
  RtcDateTime now = Rtc.GetDateTime();  // 获取当前时间
  GetTime(now);                         // 调用 GetTime() 函数显示时间

  // 支持http服务器访问
  server.handleClient();

  //显示WiFi状态
  // if WiFi.status() != WL_CONNECTED {
  //   lcd.setCursor(1, 15);  //第2行第16个字符显示
  //   lcd.print("?");
  // }
}
