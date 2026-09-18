#pragma once
#include <Arduino.h>

enum DuckyState : uint8_t {
  DUCKY_IDLE,
  DUCKY_ARMED,
  DUCKY_RUNNING,
  DUCKY_DONE,
  DUCKY_ABORTED,
  DUCKY_ERROR,
};

struct DuckyStatus {
  DuckyState state;
  int line;
  int total;
  int countdown;
  String message;
};

void duckyBegin();

bool duckyRun(const String &script, const String &origin, uint16_t delaySeconds);

void duckyAbort();

DuckyStatus duckyGetStatus();
bool duckyIsRunning();

bool duckySetLayout(const String &code);
String duckyGetLayout();

String duckyGetArrangement();

struct LayoutInfo {
  const char *code;
  const char *name;
  const char *arrangement;
};
size_t duckyLayoutCount();
LayoutInfo duckyLayoutAt(size_t i);
bool duckyLayoutExists(const String &code);

void duckyLog(const String &line);

String duckyGetLog();
void duckyClearLog();
