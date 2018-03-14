#include "color.h"

ColorQueue::ColorQueue(uint8_t r, uint8_t g, uint8_t b, uint8_t duration, uint32_t messageId)
{
  _r = r;
  _g = g;
  _b = b;
  _duration = duration;
  _messageId = messageId;
};

void ColorQueue::AddToQueue(uint8_t r, uint8_t g, uint8_t b, uint8_t duration, uint32_t messageId){
  _r = r;
  _g = g;
  _b = b;
  _duration = duration;
  _messageId = _messageId;
};

ColorQueue::ColorQueue(){};
