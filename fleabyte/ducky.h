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

// The log is polled separately from the status so a four kilobyte string
// is not serialised into every status response. seq counts bytes ever
// appended: a client passes back what it has and receives only the rest.
String duckyGetLogSince(uint32_t since, uint32_t &seqOut);
uint32_t duckyClearLog();
