#pragma once
#include <Arduino.h>
#include "ducky.h"

struct DisplayInfo {
  String ssid;
  String ip;
  String arrangement;
  String name;
  int clients;
  bool sdPresent;
  bool sdExposed;
  DuckyStatus ducky;
};

void displayBegin();

void displayShowJoin(const String &ssid, const String &password);

void displayShowMessage(const String &title, const String &detail);

void displayUpdate(const DisplayInfo &info);

void displayTick();

void displaySetRotation(uint8_t rotation);
uint8_t displayGetRotation();

void displaySetScreenOn(bool on);

void displayWake();

void displaySetLed(bool on);

// No device on the access point yet: the LED breathes red until one joins.
void displaySetWaiting(bool waiting);
