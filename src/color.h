#include <Arduino.h>

class ColorQueue;
class ColorQueue {

public:
uint8_t _r;
uint8_t _g;
uint8_t _b;
uint8_t _duration;
uint16_t _messageId;

public:
  ColorQueue(uint8_t r, uint8_t g, uint8_t b, uint8_t duration, uint32_t messageId);
  void AddToQueue(uint8_t r, uint8_t g, uint8_t b, uint8_t duration, uint32_t messageId);
  ColorQueue();

};
