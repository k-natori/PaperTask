#include <Arduino.h>
#include <M5EPD.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <MD5.h>
#include "NJScanner.h"
#include "RTMRequest.h"

#define screenWidth 540
#define screenHeight 960
#define rowHeight 84

int fontSize = 36;
int smallFontSize = 24;
String fontName = "/font.ttf";
String pemFileName = "/root_ca.pem";
boolean loaded = false;
boolean loginScreen = false;
float timezone = 0;
String filter = "status:incomplete";

std::vector<String> tasks;

M5EPD_Canvas canvas(&M5.EPD);
tp_finger_t lastFingerItem;

// function declarations
void load();
void displayLoginScreen();
void continueAfterLoginScreen();
void displayTasksScreen();
void saveState();

void shutdownWithMessage(String message);
String dateStringFromHTTPDate(String httpDate, float toTimezone);
int minFromHTTPDate(String httpDate);

// functions
void setup()
{
  // Initialize M5Paper
  M5.begin();
  M5.EPD.SetRotation(90);
  M5.TP.SetRotation(90);
  M5.EPD.Clear(true);

  canvas.createCanvas(screenWidth, screenHeight);

  // Load settings from "settings.txt" in SD card
  String wifiIDString = "wifiID";
  String wifiPWString = "wifiPW";

  File settingFile = SD.open("/settings.txt");
  if (settingFile)
  {
    while (settingFile.available() > 0)
    {
      String line = settingFile.readStringUntil('\n');
      if (line.startsWith("//"))
        continue;
      int separatorLocation = line.indexOf(":");
      if (separatorLocation > -1)
      {
        String key = line.substring(0, separatorLocation);
        String content = line.substring(separatorLocation + 1);

        if (key == "SSID")
          wifiIDString = content;

        else if (key == "PASS")
          wifiPWString = content;

        else if (key == "fontSize")
          fontSize = content.toInt();

        else if (key == "smallFontSize")
          smallFontSize = content.toInt();

        else if (key == "fontName")
        {
          if (content.endsWith(".ttf"))
          {
            fontName = content;
            if (!fontName.startsWith("/"))
            {
              fontName = "/" + fontName;
            }
          }
        }
        else if (key == "pemFileName")
          pemFileName = content;
        
        else if (key == "timezone")
          timezone = content.toFloat();
        
        else if (key == "RTM_apiKey")
          RTMRequest::setRTM_apiKey(content);
        
        else if (key == "RTM_sharedSecret")
          RTMRequest::setRTM_sharedSecret(content);

        else if (key == "filter")
          filter = content;
      }
    }
    settingFile.close();

    // Start Wifi connection
    WiFi.begin(wifiIDString.c_str(), wifiPWString.c_str());
    Serial.println(wifiIDString);
    // Wait until wifi connected
    int i = 0;
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
      Serial.print(".");
      i++;
      if (i > 120)
        break;
    }
    Serial.print("\n");

    // load font
    canvas.loadFont(fontName, SD);
    canvas.createRender(fontSize, 256);
    canvas.createRender(smallFontSize, 256);
    canvas.setTextSize(fontSize);
    canvas.setTextColor(WHITE, BLACK);

    // Load PEM file in SD card
    File pemFile = SD.open(pemFileName.c_str());
    if (pemFile)
    {
      RTMRequest::setRTM_rootCA(pemFile.readString());
      pemFile.close();
      Serial.println("pem file loaded:" + pemFileName);
    }
    else
    {
      Serial.println("pem file not found");
    }
  }

  // Load cache file
  File cacheFile = SD.open("/RTM.txt");
  if (cacheFile)
  {
    // First line is auth_token
    String line = cacheFile.readStringUntil('\n');
    if (line.length() > 10)
    {
      line.trim();
      RTMRequest::setRTM_authToken(line);
    }

    // Other lines are task names
    while (cacheFile.available())
    {
      line = cacheFile.readStringUntil('\n');
      if (!line.isEmpty())
      {
        tasks.push_back(line);
      }
    }
    cacheFile.close();
  }
}

void loop()
{
  if (!loaded)
  { // First loop after boot
    if (WiFi.status() != WL_CONNECTED)
    {
      // Not connected
      shutdownWithMessage("WiFi not connected");
    }
    load();
  }

  // Touch detection
  if (M5.TP.available())
  {
    if (!M5.TP.isFingerUp())
    {
      M5.TP.update();
      tp_finger_t fingerItem = M5.TP.readFinger(0);
      if (lastFingerItem.x == fingerItem.x && lastFingerItem.y == fingerItem.y)
      {
        return;
      }
      lastFingerItem = fingerItem;

      if (fingerItem.x == 0 || fingerItem.y == 0)
      {
        return;
      }

      if (loginScreen) {
        if (fingerItem.y > 756 && fingerItem.y < (756 + rowHeight)
        && fingerItem.x > 40 && fingerItem.x < (screenWidth - 40)) {
          // Next button tapped. Invert button area
          canvas.ReversePartColor(40, 756, screenWidth - 80, rowHeight);
          canvas.pushCanvas(UPDATE_MODE_NONE);
          M5.EPD.UpdateArea(40, 756, screenWidth - 80, rowHeight, UPDATE_MODE_DU4);
          delay(500);
          continueAfterLoginScreen();
        }
      }
    }
  }
}

void load()
{
  loaded = true;

  if (RTMRequest::getRTM_authToken().length() > 10) {
    // Check token is valid
    RTMRequest request = RTMRequest("rtm.auth.checkToken", true);
    int result = request.getRequest();
    if (result == HTTP_CODE_OK && !request.getNextTagContent("token").isEmpty()) {
      Serial.println("Token is valid");
      displayTasksScreen();
    } else {
      shutdownWithMessage("HTTP error or token is invalid");
    }
  } else {
    // Not authorized yet. Show QR code to login
    displayLoginScreen();
  }
}

void displayLoginScreen()
{
  // get authorization url and show as QR code
  String authURL = RTMRequest::externalAuthURLString();
  Serial.println(authURL);
  canvas.qrcode(authURL, 40, 40, 460, 10);
  canvas.setCursor(40, 540);

  canvas.drawString("Scan the QR code and give access", 40, 674 + (rowHeight - fontSize) / 2);

  canvas.drawRect(40, 756, 460, rowHeight, WHITE);
  canvas.drawString("Next", 80, 756 + (rowHeight - fontSize) / 2);

  canvas.pushCanvas(0, 0, UPDATE_MODE_DU4);
  loginScreen = true;
}

void continueAfterLoginScreen() {
  // get auth_token using frob provided
  String frob = RTMRequest::getRTM_frob();
  if (frob.isEmpty()) {
    Serial.println("No frob");
    return;
  }
  RTMRequest request = RTMRequest("rtm.auth.getToken", false);
  request.setKeyValue("frob", frob);
  int result = request.getRequest();
  if (result == HTTP_CODE_OK) {
    String token = request.getNextTagContent("token");
    token.trim();
    Serial.println(token);
    RTMRequest::setRTM_authToken(token);
    saveState();
    loginScreen = false;
    delay(1000);
    canvas.fillCanvas(BLACK);
    displayTasksScreen();
  }
}

void displayTasksScreen() {
  // Request task list
  RTMRequest request = RTMRequest("rtm.tasks.getList", true);
  request.setKeyValue("filter", filter);
  int result = request.getRequest();
  if (result != HTTP_CODE_OK) {
    shutdownWithMessage("HTTP " + String(result));
  }

  // Parse XML
  tasks.clear();
  std::map<String, String> taskProperties = request.getNextTagProperties("taskseries");

  while (taskProperties.size() > 0)
  {
    String taskName = taskProperties["name"];
    if (!taskName.isEmpty()) {
      tasks.push_back(taskName);
    }
    taskProperties = request.getNextTagProperties("taskseries");
  }
  
    // Draw results
  int maxCount = tasks.size();
  if (maxCount > 10) maxCount = 10;
  canvas.setTextSize(fontSize);
  for (int i = 0; i < maxCount; i++)
  {
    String line = tasks[i];
    canvas.drawString(line, 16, i * rowHeight + (rowHeight - fontSize) / 2);
    canvas.drawFastHLine(8, (i+1) * rowHeight -1, screenWidth - 16, WHITE);
  }

  // Draw date
  String dateString = request.getDateString();
  canvas.setTextSize(smallFontSize);
  canvas.drawString(dateStringFromHTTPDate(dateString, timezone), 8, screenHeight - (smallFontSize + 8));

  // Draw battery
  float voltage = M5.getBatteryVoltage() / 1000.0;
  float battery = ((voltage - 3.2) / (4.25 - 3.2));
  canvas.fillRect(0, screenHeight - 10, screenWidth * battery, 10, WHITE);

  canvas.pushCanvas(0, 0, UPDATE_MODE_GC16);

  // Set wake-up timer on next O'clock
  int minutes = minFromHTTPDate(dateString);
  delay(500);
  M5.RTC.clearIRQ();
  M5.shutdown((60 - minutes) * 60);
}

// Save auth_token and task names on RTM.txt in SD card
void saveState()
{

  File cacheFile = SD.open("/RTM.txt", FILE_WRITE, true);
  if (cacheFile)
  {
    // Write token on first line
    String token = RTMRequest::getRTM_authToken();
    cacheFile.println(token);

    for (auto it = tasks.begin(); it != tasks.end(); ++it)
    {
      String line = *it;
      if (!line.isEmpty()) {
        cacheFile.println(line);
      }
    }

    cacheFile.close();
  }
}

void shutdownWithMessage(String message)
{
  // Display message and shutdown
  canvas.drawString(message, 8, (rowHeight - fontSize) / 2);
  canvas.pushCanvas(0, 0, UPDATE_MODE_DU4);
  delay(500);
  M5.RTC.clearIRQ();
  M5.shutdown();
}

// Convert the date in HTTP response to local timezone
String dateStringFromHTTPDate(String httpDate, float toTimezone) {
  // Mon, 07 Aug 2023 14:14:37 GMT
  // to
  // Mon, 07 Aug 2023 23:14:37 +0900

  const String dayOfWeekStringArray = "MonTueWedThuFriSatSan";
  const String monthStringArray = "JanFebMarAprMayJunJulAugSepOctNovDec";
  int numberOfDaysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

  String dayOfWeekString = httpDate.substring(0,3);
  int dayOfWeek = dayOfWeekStringArray.indexOf(dayOfWeekString) / 3 + 1;
  int day = httpDate.substring(5,7).toInt();
  String monthString = httpDate.substring(8,11);
  int month = monthStringArray.indexOf(monthString) / 3 + 1;
  int year = httpDate.substring(12,16).toInt();
  int hour = httpDate.substring(17,19).toInt();
  int minute = httpDate.substring(20,22).toInt();
  int second = httpDate.substring(23,25).toInt();

  if (year % 4 == 0 && year % 100 != 0 || year % 400 == 0) {
    numberOfDaysInMonth[1] = 29;
  }

  int secondsInDay = hour * 3600 + minute * 60 + second;
  int convertedSecondsInDay = secondsInDay + toTimezone * 3600;

  if (convertedSecondsInDay < 0) { // previous day
    if (day == 1) {
      if (month == 1) {
        if (year > 1) {
          year--;
        }
        month = 12;
      } else {
        month--;
      }
      day = numberOfDaysInMonth[month-1];
    } else {
      day--;
    }
    if (dayOfWeek == 1) {
      dayOfWeek = 7;
    } else {
      dayOfWeek--;
    }
    convertedSecondsInDay += 24 * 3600;

  } else if (convertedSecondsInDay >= 24 * 3600) { // next day
    if (day == numberOfDaysInMonth[month-1]) {
      if (month == 12) {
        year++;
        month = 1;
      } else {
        month++;
      }
      day = 1;
    } else {
      day++;
    }
    if (dayOfWeek == 7) {
      dayOfWeek = 1;
    } else {
      dayOfWeek++;
    }
    convertedSecondsInDay -= 24 * 3600;
  }
  hour = convertedSecondsInDay / 3600;
  minute = (convertedSecondsInDay % 3600) / 60;
  second = (convertedSecondsInDay % 3600) % 60;

  dayOfWeekString = dayOfWeekStringArray.substring((dayOfWeek-1)*3, dayOfWeek*3);
  monthString = monthStringArray.substring((month-1)*3, month*3);


  int timezoneMinute = abs((toTimezone - (int)toTimezone) * 60);
  char result[32];
  snprintf(result, 32, "%s, %02d %s %04d %02d:%02d:%02d %+03d%02d",
  dayOfWeekString.c_str(), day, monthString.c_str(), year, hour, minute, second, (int)toTimezone, timezoneMinute);
  return String(result);
}

// Extract minutes from HTTP date string
int minFromHTTPDate(String httpDate)
{ 
  return httpDate.substring(20, 22).toInt();
}