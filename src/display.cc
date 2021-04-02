#include "display.h"

Point getCoord(int idx) {
  int x = idx % 4;
  int y = idx / 4;
  return Point {
    x : GRID_OFFSET_X + (x * (BLOCK_SIZE + GRID_GAP)),
    y : GRID_OFFSET_Y + (y * (BLOCK_SIZE + GRID_GAP))
  };
}
