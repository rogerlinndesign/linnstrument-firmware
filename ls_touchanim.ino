/************************* ls_touchanim: Displays animations for touches **************************
Copyright 2023 Roger Linn Design (https://www.rogerlinndesign.com)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
***************************************************************************************************
This displays evolved animations for each touch based on the global settings.
**************************************************************************************************/

int32_t colsInRowsAnimated[MAXROWS];
unsigned long touchAnimationLastMoment[MAXCOLS][MAXROWS];
unsigned long touchAnimationSpeed[MAXCOLS][MAXROWS];
signed char touchAnimationLastState[MAXCOLS][MAXROWS];
byte lastTouchAnimCol[2];
byte lastTouchAnimRow[2];

void initializeTouchAnimation() {
  initializeLedsLayer(LED_LAYER_PLAYED);

  prevTouchAnimTimerCount = millis();

  for (byte i = 0; i < 2; ++i) {
    lastTouchAnimCol[i] = 0;
    lastTouchAnimRow[i] = 0;
  }

  for (byte r = 0; r < MAXROWS; ++r) {
      colsInRowsAnimated[r]= 0;
  }

  for (byte c = 0; c < MAXCOLS; ++c) {
    for (byte r = 0; r < MAXROWS; ++r) {
      touchAnimationLastMoment[c][r] = 0;
      touchAnimationLastState[c][r] = -1;
      touchAnimationSpeed[c][r] = 0;
    }
  }
}

unsigned long calcTouchAnimationSpeed(byte mode, byte value7Bit) {
  unsigned long speed = 196 - value7Bit;
  switch (mode) {
    case playedBlinds:
    case playedCurtains:
    case playedTargets:
    case playedUp:
    case playedDown:
    case playedLeft:
    case playedRight:
    case playedOrbits:
      speed = speed / 2;
      break;
  }
  return speed;
}

void startTouchAnimation(byte col, byte row, unsigned long speed) {
  if (touchAnimationLastState[col][row] != -1) {
      drawTouchedAnimation(col, row, cellOff, touchAnimationLastState[col][row]);
      touchAnimationLastState[col][row] = -1;

      if (lastTouchAnimCol[1] == col && lastTouchAnimRow[1] == row) {
        lastTouchAnimCol[1] = lastTouchAnimCol[0];
        lastTouchAnimRow[1] = lastTouchAnimRow[0];
        lastTouchAnimCol[0] = col;
        lastTouchAnimRow[0] = row;
      }
  }
  else {
    // for expanding touch animations, we only allow two to be active at any given time
    // in order to prevent the whole surface to be lit up with animated cells
    switch (Split[getSplitOf(col)].playedTouchMode) {
      case playedCrosses:
      case playedCircles:
      case playedSquares:
      case playedDiamonds:
      case playedStars:
      case playedSparkles:
      case playedCurtains:
      case playedBlinds:
      case playedTargets: {
        byte clear_col;
        byte clear_row;
        // determine which of the last two animations is outdated and should be replaced
        // with the new one
        if (cell(lastTouchAnimCol[0], lastTouchAnimRow[0]).touched != touchedCell &&
            cell(lastTouchAnimCol[1], lastTouchAnimRow[1]).touched == touchedCell) {
          clear_col = lastTouchAnimCol[0];
          clear_row = lastTouchAnimRow[0];
        }
        else {
          clear_col = lastTouchAnimCol[1];
          clear_row = lastTouchAnimRow[1];
          lastTouchAnimCol[1] = lastTouchAnimCol[0];
          lastTouchAnimRow[1] = lastTouchAnimRow[0];
        }

        // stop a previously active animation
        if (clear_col != 0 && clear_row != 0) {
          drawTouchedAnimation(clear_col, clear_row, cellOff, touchAnimationLastState[clear_col][clear_row]);
          touchAnimationLastState[clear_col][clear_row] = -1;
          colsInRowsAnimated[clear_row] &= ~(int32_t)(1 << clear_col);
        }

        lastTouchAnimCol[0] = col;
        lastTouchAnimRow[0] = row;
        break;
      }
    }
  }

  // store the touch animation info and draw the first frame
  colsInRowsAnimated[row] |= (int32_t)(1 << col);
  touchAnimationLastMoment[col][row] = millis();
  touchAnimationSpeed[col][row] = speed;
  drawTouchedAnimation(col, row, cellOn, 0);
}

void touchAnimLed(byte col, byte row, byte color, CellDisplay disp) {
  if (col > 0) {
    setLed(col, row, color, disp, LED_LAYER_PLAYED);
  }
}

void drawTouchedAnimation(byte col, byte row, CellDisplay disp, signed char state) {
  byte state_max = 5;
  switch (Split[getSplitOf(col)].playedTouchMode) {
    case playedTargets:
      state_max = max(max(row, NUMROWS-row), max(col, NUMCOLS-col)) + 1;
      break;
    case playedBlinds:
    case playedUp:
    case playedDown:
      state_max = max(row, NUMROWS-row) + 1;
      break;
    case playedCurtains:
    case playedLeft:
    case playedRight:
      state_max = max(col, NUMCOLS-col) + 1;
      break;
    case playedOrbits:
      state_max = 8;
      break;
  }

  if (state >= state_max) {
    if (cell(col, row).touched == touchedCell) {
      state = state % state_max;
    }
    else {
      touchAnimationLastMoment[col][row] = 0;
      touchAnimationLastState[col][row] = -1;
      touchAnimationSpeed[col][row] = 0;
      colsInRowsAnimated[row] &= ~(int32_t)(1 << col);
    }
  }
  
  if (state >= 0 && state < state_max) {
    touchAnimationLastState[col][row] = state;

    byte color = Split[getSplitOf(col)].colorPlayed;
    switch (Split[getSplitOf(col)].playedTouchMode) {
      case playedSparkles:
        if (disp == cellOff) {
          for (int c = max(col - state, 0); c <= min(col + state, NUMCOLS); ++c) {
            touchAnimLed(c, row + state, color, disp);
            touchAnimLed(c, row - state, color, disp);
          }
          for (int r = max(row - state, 0); r <= min(row + state, NUMROWS); ++r) {
            touchAnimLed(col + state, r, color, disp);
            touchAnimLed(col - state, r, color, disp);
          }
        }
        else {
          int c1 = random(max(col - state, 0), min(col + state, NUMCOLS) + 1);
          touchAnimLed(c1, row + state, color, disp);
          int c2 = random(max(col - state, 0), min(col + state, NUMCOLS) + 1);
          touchAnimLed(c2, row - state, color, disp);
          int r1 = random(max(row - state, 0), min(row + state, NUMROWS) + 1);
          touchAnimLed(col + state, r1, color, disp);
          int r2 = random(max(row - state, 0), min(row + state, NUMROWS) + 1);
          touchAnimLed(col - state, r2, color, disp);
        }
        break;
      case playedSquares:
        for (int c = max(col - state, 0); c <= min(col + state, NUMCOLS); ++c) {
          touchAnimLed(c, row + state, color, disp);
          touchAnimLed(c, row - state, color, disp);
        }
        for (int r = max(row - state, 0); r <= min(row + state, NUMROWS); ++r) {
          touchAnimLed(col + state, r, color, disp);
          touchAnimLed(col - state, r, color, disp);
        }
        break;
      case playedDiamonds: {
        int c = col - state, r = row;
        for (; c <= col; ++c, ++r) {
          touchAnimLed(c, r, color, disp);
        }
        c = col; r = row + state;
        for (; r >= row; ++c, --r) {
          touchAnimLed(c, r, color, disp);
        }
        c = col + state; r = row;
        for (; c >= col; --c, --r) {
          touchAnimLed(c, r, color, disp);
        }
        c = col; r = row - state;
        for (; r <= row; --c, ++r) {
          touchAnimLed(c, r, color, disp);
        }
        break;
      }
      case playedBlinds:
        for (byte c = 0; c < NUMCOLS; ++c) {
          touchAnimLed(c, row + state, color, disp);
          touchAnimLed(c, row - state, color, disp);
        }
        break;
      case playedCurtains:
        for (byte r = 0; r < NUMROWS; ++r) {
          touchAnimLed(col + state, r, color, disp);
          touchAnimLed(col - state, r, color, disp);
        }
        break;
      case playedCircles:
        switch (state) {
          case 1:
            touchAnimLed(col, row + state, color, disp);
            touchAnimLed(col, row - state, color, disp);
            touchAnimLed(col - state, row, color, disp);
            touchAnimLed(col + state, row, color, disp);
            break;
          case 2:
            touchAnimLed(col - 1, row + state, color, disp);
            touchAnimLed(col + 0, row + state, color, disp);
            touchAnimLed(col + 1, row + state, color, disp);
            touchAnimLed(col - 1, row - state, color, disp);
            touchAnimLed(col + 0, row - state, color, disp);
            touchAnimLed(col + 1, row - state, color, disp);
            touchAnimLed(col - state, row - 1, color, disp);
            touchAnimLed(col - state, row + 0, color, disp);
            touchAnimLed(col - state, row + 1, color, disp);
            touchAnimLed(col + state, row - 1, color, disp);
            touchAnimLed(col + state, row + 0, color, disp);
            touchAnimLed(col + state, row + 1, color, disp);
            break;
          case 3:
            touchAnimLed(col - 2, row + state - 1, color, disp);
            touchAnimLed(col - 1, row + state, color, disp);
            touchAnimLed(col + 0, row + state, color, disp);
            touchAnimLed(col + 1, row + state, color, disp);
            touchAnimLed(col + 2, row + state - 1, color, disp);
            touchAnimLed(col - 2, row - state + 1, color, disp);
            touchAnimLed(col - 1, row - state, color, disp);
            touchAnimLed(col + 0, row - state, color, disp);
            touchAnimLed(col + 1, row - state, color, disp);
            touchAnimLed(col + 2, row - state + 1, color, disp);
            touchAnimLed(col - state, row - 1, color, disp);
            touchAnimLed(col - state, row + 0, color, disp);
            touchAnimLed(col - state, row + 1, color, disp);
            touchAnimLed(col + state, row - 1, color, disp);
            touchAnimLed(col + state, row + 0, color, disp);
            touchAnimLed(col + state, row + 1, color, disp);
            break;
          case 4:
            touchAnimLed(col - 3, row + state - 1, color, disp);
            touchAnimLed(col - 2, row + state - 1, color, disp);
            touchAnimLed(col - 1, row + state, color, disp);
            touchAnimLed(col + 0, row + state, color, disp);
            touchAnimLed(col + 1, row + state, color, disp);
            touchAnimLed(col + 2, row + state - 1, color, disp);
            touchAnimLed(col + 3, row + state - 1, color, disp);
            touchAnimLed(col - 3, row - state + 1, color, disp);
            touchAnimLed(col - 2, row - state + 1, color, disp);
            touchAnimLed(col - 1, row - state, color, disp);
            touchAnimLed(col + 0, row - state, color, disp);
            touchAnimLed(col + 1, row - state, color, disp);
            touchAnimLed(col + 2, row - state + 1, color, disp);
            touchAnimLed(col + 3, row - state + 1, color, disp);
            touchAnimLed(col - state + 1, row - 2, color, disp);
            touchAnimLed(col - state, row - 1, color, disp);
            touchAnimLed(col - state, row + 0, color, disp);
            touchAnimLed(col - state, row + 1, color, disp);
            touchAnimLed(col - state + 1, row + 2, color, disp);
            touchAnimLed(col + state - 1, row - 2, color, disp);
            touchAnimLed(col + state, row - 1, color, disp);
            touchAnimLed(col + state, row + 0, color, disp);
            touchAnimLed(col + state, row + 1, color, disp);
            touchAnimLed(col + state - 1, row + 2, color, disp);
            break;
        }
        break;
      case playedCrosses:
        touchAnimLed(col, row + state, color, disp);
        touchAnimLed(col, row - state, color, disp);
        touchAnimLed(col - state, row, color, disp);
        touchAnimLed(col + state, row, color, disp);
        break;
      case playedStars:
        if (state % 2 == 1) {
          touchAnimLed(col, row + state, color, disp);
          touchAnimLed(col, row - state, color, disp);
          touchAnimLed(col - state, row, color, disp);
          touchAnimLed(col + state, row, color, disp);
        }
        else {
          int half_state =  state / 2;
          touchAnimLed(col - half_state, row + half_state, color, disp);
          touchAnimLed(col + half_state, row - half_state, color, disp);
          touchAnimLed(col - half_state, row - half_state, color, disp);
          touchAnimLed(col + half_state, row + half_state, color, disp);
        }
        break;
      case playedTargets:
        for (int r = row + state; r < NUMROWS; ++r) {
          touchAnimLed(col, r, color, disp);
        }
        for (int r = row - state; r >= 0; --r) {
          touchAnimLed(col, r, color, disp);
        }
        for (int c = col - state; c >= 0; --c) {
          touchAnimLed(c, row, color, disp);
        }
        for (int c = col + state; c < NUMCOLS; ++c) {
          touchAnimLed(c, row, color, disp);
        }
        break;
      case playedUp:
        for (int r = row + state; r < NUMROWS; ++r) {
          touchAnimLed(col, r, color, disp);
        }
        break;
      case playedDown:
        for (int r = row - state; r >= 0; --r) {
          touchAnimLed(col, r, color, disp);
        }
        break;
      case playedLeft:
        for (int c = col - state; c >= 0; --c) {
          touchAnimLed(c, row, color, disp);
        }
        break;
      case playedRight:
        for (int c = col + state; c < NUMCOLS; ++c) {
          touchAnimLed(c, row, color, disp);
        }
        break;
      case playedOrbits:
        switch (state % 4) {
          case 0: touchAnimLed(col - 1, row + 1, color, disp); touchAnimLed(col + 1, row - 1, color, disp); break;
          case 1: touchAnimLed(col, row - 1, color, disp); touchAnimLed(col, row + 1, color, disp); break;
          case 2: touchAnimLed(col + 1, row + 1, color, disp); touchAnimLed(col - 1, row - 1, color, disp); break;
          case 3: touchAnimLed(col - 1, row, color, disp); touchAnimLed(col + 1, row, color, disp); break;
        }
        break;
    }
  }
}

void performAdvanceTouchAnimations(unsigned long nowMillis) {
  if ((Split[LEFT].playedTouchMode == playedCell || Split[LEFT].playedTouchMode == playedSame) &&
      (Split[RIGHT].playedTouchMode == playedCell || Split[RIGHT].playedTouchMode == playedSame)) return;

  bool buffer_started = false;

  for (byte row = 0; row < NUMROWS; ++row) {
    int32_t colsInRowAnimated = colsInRowsAnimated[row];
    while (colsInRowAnimated) {
      byte col = 31 - __builtin_clz(colsInRowAnimated);

      if (!buffer_started) {
        startBufferedLeds();
        buffer_started = true;
      }

      // clear the animation of the touch in its last state
      drawTouchedAnimation(col, row, cellOff, touchAnimationLastState[col][row]);

      colsInRowAnimated &= ~(1 << col);
    }
  }

  if (buffer_started) {
    for (byte row = 0; row < NUMROWS; ++row) {
      int32_t colsInRowAnimated = colsInRowsAnimated[row];
      while (colsInRowAnimated) {
        byte col = 31 - __builtin_clz(colsInRowAnimated);

        signed char state = touchAnimationLastState[col][row];
        unsigned long speed = touchAnimationSpeed[col][row];

        // if the cell is still touched without a pending release and at least 100ms have passed since
        // the initial touch, update the speed of the animation based on the current pressure, but apply
        // it with a slew rate
        TouchInfo* c = &cell(col, row);
        if (c->touched == touchedCell && c->pendingReleaseCount == 0 && calcTimeDelta(nowMillis, c->lastTouch) > 100) {
          speed -= speed/3;
          speed += calcTouchAnimationSpeed(Split[getSplitOf(col)].playedTouchMode, scale1016to127(c->pressureZ, true))/3;
          touchAnimationSpeed[col][row] = speed;
        }

        // if we exceed the time delay between each animation step, increase the state counter
        // and reset the delay
        if (calcTimeDelta(nowMillis, touchAnimationLastMoment[col][row]) >= speed) {
          state += 1;
          touchAnimationLastMoment[col][row] = nowMillis;
        }

        // draw the animation of the touch in its last state
        drawTouchedAnimation(col, row, cellOn, state);
      
        colsInRowAnimated &= ~(1 << col);
      }
    }

    finishBufferedLeds();
  }
}
