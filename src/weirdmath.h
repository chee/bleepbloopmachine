#pragma once
int wrapping_add(int cur, int max, int inc = 1, int min = 0);
int wrapping_sub(int cur, int max, int inc = 1, int min = 0);
float wrapping_add(float cur, float max = 1.0, float inc = 0.1, int min = -1.0);
float wrapping_sub(float cur, float max = 1.0, float inc = 0.1, int min = -1.0);
float saturating_add(float cur, float max = 1.0, float inc = 0.1,
                     float min = -1.0);
float saturating_sub(float cur, float max = 1.0, float inc = 0.1,
                     float min = -1.0);

float saturating_add_curve(float cur, float max = 1.0, int step = 10, float min = -1.0);

float saturating_sub_curve(float cur, float max = 1.0, int step = 10, float min = -1.0);
