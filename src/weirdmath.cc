#pragma once
#include "weirdmath.h"

int wrapping_add(int cur, int max, int inc, int min) {
  auto nxt = cur + inc;
  return nxt <= max ? nxt : min;
}

int wrapping_sub(int cur, int max, int inc, int min) {
  auto nxt = cur - inc;
  return nxt >= min ? nxt : max;
}

float wrapping_add(float cur, float max, float inc, int min) {
  auto nxt = cur + inc;
  return nxt <= max ? nxt : min;
}

float wrapping_sub(float cur, float max, float inc, int min) {
  auto nxt = cur - inc;
  return nxt >= min ? nxt : max;
}

float saturating_add(float cur, float max, float inc, float min) {
  auto nxt = cur + inc;
  return nxt <= max ? nxt : max;
}

float saturating_sub(float cur, float max, float inc, float min) {
  auto nxt = cur - inc;
  return nxt >= min ? nxt : min;
}

float saturating_add_curve(float cur, float max, int step, float min) {
	auto inc = (max - min) / step;
  auto nxt = cur + inc;
  return nxt <= max ? nxt : max;
}

float saturating_sub_curve(float cur, float max, int step, float min) {
  auto inc = (max - min) / step;
	auto nxt = cur - inc;
  return nxt >= min ? nxt : min;
}
