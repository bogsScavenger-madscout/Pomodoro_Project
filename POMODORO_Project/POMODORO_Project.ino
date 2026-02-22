#include <Wire.h>
#include "RTClib.h"
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>


// ===== RTC And POMODORO SECTION ====================================================================================

RTC_DS3231 myRTC;

bool running = false;
bool onBreak = false;

DateTime endTime;

const int focusTime = 1;
const int shortBreakTime = 1;
const int longBreakTime = 1;

const int totalPomodoros = 12;
int pomodorosCompleted = 0;

DateTime ReviewEND(2026, 3, 8, 0, 0, 0);
int daysLeft;

void setupRTC() {
  if (!myRTC.begin()) {
    Serial.println("RTC module not connected");
    while (1);
  }

  if (myRTC.lostPower()) {
    myRTC.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
}

void updateDayCounter() {
  DateTime now = myRTC.now();
  TimeSpan remaining = ReviewEND - now;

  daysLeft = (remaining.totalseconds() + 86399) / 86400;
  if (remaining.totalseconds() <= 0) daysLeft = 0;
}

// ===== DFPLAYER SECTION ===============================================================================================

#define RX_PIN 10
#define TX_PIN 11

SoftwareSerial mp3Serial(RX_PIN, TX_PIN);
DFRobotDFPlayerMini mp3;

const int musicVolume = 15;

bool musicPlaying = false;
unsigned long lastMusicChange = 0;
const unsigned long musicInterval = 360000;

void setupDFPlayer() {
  mp3Serial.begin(9600);

  if (!mp3.begin(mp3Serial, true, false)) {
    Serial.println("MP3 init failed");
    while (true);
  }
}

void startBreakAudio(int reminderTrack) {
  mp3.stop();
  delay(200);

  mp3.play(reminderTrack);
  delay(3000);

  mp3.stop();
  delay(200);

  mp3.volume(25);
  int musicTrack = random(5, 10);
  mp3.play(musicTrack);

  lastMusicChange = millis();
  musicPlaying = true;
}

void stopBreakMusic() {
  mp3.stop();
  musicPlaying = false;
}

void updateBreakMusic() {
  if (musicPlaying && millis() - lastMusicChange > musicInterval) {
    mp3.stop();
    delay(150);

    int musicTrack = random(5, 10);
    mp3.play(musicTrack);

    lastMusicChange = millis();
  }
}


#define buttonPin 6
#define ledPin 8

LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  Serial.begin(9600);
  Wire.begin();

  lcd.begin();
  lcd.backlight();

  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  randomSeed(analogRead(A0));

  setupRTC();
  setupDFPlayer();
}

void loop() {

  DateTime now = myRTC.now();

  updateDayCounter();

  lcd.setCursor(12,1);
  lcd.print("D:");
  lcd.print(daysLeft);

  // ===== START BUTTON ========================================================================================================
  if (digitalRead(buttonPin) == LOW && !running) {
    delay(50);

    pomodorosCompleted = 0;
    endTime = myRTC.now() + TimeSpan(0,0,focusTime,0);
    running = true;
    onBreak = false;

    digitalWrite(ledPin, HIGH);
  }

  if (running) {

    updateBreakMusic();

    if (now >= endTime) {

      if (!onBreak) {

        pomodorosCompleted++;

        if (pomodorosCompleted >= totalPomodoros) {
          running = false;
          mp3.stop();
          lcd.clear();
          lcd.print("ALL DONE!");
          return;
        }

        if (pomodorosCompleted % 4 == 0) {
          endTime = myRTC.now() + TimeSpan(0,0,longBreakTime,0);
          startBreakAudio(3);
        } else {
          endTime = myRTC.now() + TimeSpan(0,0,shortBreakTime,0);
          startBreakAudio(1);
        }

        onBreak = true;

      } else {

        endTime = myRTC.now() + TimeSpan(0,0,focusTime,0);
        onBreak = false;

        stopBreakMusic();

        if (pomodorosCompleted % 4 == 0)
          mp3.play(4);
        else
          mp3.play(2);
      }

    } else {

      TimeSpan remaining = endTime - now;
      int pomodorosLeft = totalPomodoros - pomodorosCompleted;

      lcd.setCursor(0,0);
      lcd.print("POMO:");
      lcd.print(pomodorosLeft);
      lcd.print("   ");

      lcd.setCursor(10,0);
      lcd.print(onBreak ? "Break " : "Focus ");

      char buf[17];
      sprintf(buf,"TIMER:%02d:%02d",
              remaining.minutes(),
              remaining.seconds());

      lcd.setCursor(0,1);
      lcd.print(buf);
    }
  }
}