/************************* ls_touchanim: Displays animations for touches **************************
This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/
or send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
***************************************************************************************************
This displays evolved animations for each touch based on the global settings.
**************************************************************************************************/

int32_t colsInRowsAnimated[MAXROWS];
unsigned long touchAnimationStart[MAXCOLS][MAXROWS];
unsigned long touchAnimationSpeed[MAXCOLS][MAXROWS];
signed char touchAnimationLastState[MAXCOLS][MAXROWS];

void initializeTouchAnimation() {
  initializeLedsLayer(LED_LAYER_PLAYED);

  prevTouchAnimTimerCount = millis();

  for (byte r = 0; r < MAXROWS; ++r) {
      colsInRowsAnimated[r]= 0;
  }

  for (byte c = 0; c < MAXCOLS; ++c) {
    for (byte r = 0; r < MAXROWS; ++r) {
      touchAnimationStart[c][r] = 0;
      touchAnimationLastState[c][r] = -1;
      touchAnimationSpeed[c][r] = 0;
    }
  }
}

void startTouchAnimation(byte col, byte row, unsigned long speed) {
  if (touchAnimationLastState[col][row] != -1) {
      drawTouchedAnimation(col, row, cellOff, touchAnimationLastState[col][row]);
      touchAnimationLastState[col][row] = -1;
  }

  colsInRowsAnimated[row] |= (int32_t)(1 << col);
  touchAnimationStart[col][row] = millis();
  touchAnimationSpeed[col][row] = speed;
  drawTouchedAnimation(col, row, cellOn, 0);
}

void drawTouchedAnimation(byte col, byte row, CellDisplay cellDisplay, signed char state) {
  if (state >= 5) {
    touchAnimationStart[col][row] = 0;
    touchAnimationLastState[col][row] = -1;
    colsInRowsAnimated[row] &= ~(int32_t)(1 << col);
  }
  else if (state >= 0 && state < 5) {
    touchAnimationLastState[col][row] = state;

    byte color = Split[getSplitOf(col)].colorPlayed;
    switch (Split[getSplitOf(col)].playedTouchMode) {
      case playedSparkles:
        if (cellDisplay == cellOff) {
          for (int c = max(col - state, 0); c <= min(col + state, NUMCOLS); ++c) {
            setLed(c, row + state, color, cellDisplay, LED_LAYER_PLAYED);
            setLed(c, row - state, color, cellDisplay, LED_LAYER_PLAYED);
          }
          for (int r = max(row - state, 0); r <= min(row + state, NUMROWS); ++r) {
            setLed(col + state, r, color, cellDisplay, LED_LAYER_PLAYED);
            setLed(col - state, r, color, cellDisplay, LED_LAYER_PLAYED);
          }
        }
        else {
          int c1 = random(max(col - state, 0), min(col + state, NUMCOLS) + 1);
          setLed(c1, row + state, color, cellDisplay, LED_LAYER_PLAYED);
          int c2 = random(max(col - state, 0), min(col + state, NUMCOLS) + 1);
          setLed(c2, row - state, color, cellDisplay, LED_LAYER_PLAYED);
          int r1 = random(max(row - state, 0), min(row + state, NUMROWS) + 1);
          setLed(col + state, r1, color, cellDisplay, LED_LAYER_PLAYED);
          int r2 = random(max(row - state, 0), min(row + state, NUMROWS) + 1);
          setLed(col - state, r2, color, cellDisplay, LED_LAYER_PLAYED);
        }
        break;
      case playedSquares:
        for (int c = max(col - state, 0); c <= min(col + state, NUMCOLS); ++c) {
          setLed(c, row + state, color, cellDisplay, LED_LAYER_PLAYED);
          setLed(c, row - state, color, cellDisplay, LED_LAYER_PLAYED);
        }
        for (int r = max(row - state, 0); r <= min(row + state, NUMROWS); ++r) {
          setLed(col + state, r, color, cellDisplay, LED_LAYER_PLAYED);
          setLed(col - state, r, color, cellDisplay, LED_LAYER_PLAYED);
        }
        break;
      case playedBlinds:
        for (byte c = 0; c < NUMCOLS; ++c) {
          setLed(c, row + state, color, cellDisplay, LED_LAYER_PLAYED);
          setLed(c, row - state, color, cellDisplay, LED_LAYER_PLAYED);
        }
        break;
      case playedCurtains:
        for (byte r = 0; r < NUMROWS; ++r) {
          setLed(col + state, r, color, cellDisplay, LED_LAYER_PLAYED);
          setLed(col - state, r, color, cellDisplay, LED_LAYER_PLAYED);
        }
        break;
      case playedCircles:
        switch (state) {
          case 1:
            setLed(col, row + state, color, cellDisplay, LED_LAYER_PLAYED);
            setLed(col, row - state, color, cellDisplay, LED_LAYER_PLAYED);
            setLed(col - state, row, color, cellDisplay, LED_LAYER_PLAYED);
            setLed(col + state, row, color, cellDisplay, LED_LAYER_PLAYED);
            break;
          case 2:
            setLed(col - 1, row + state, color, cellDisplay, LED_LAYER_PLAYED);
            setLed(col + 0, row + state, color, cellDisplay, LED_LAYER_PLAYED);
            setLed(col + 1, row + state, color, cellDisplay, LED_LAYER_PLAYED);
            setLed(col - 1, row - state, color, cellDisplay, LED_LAYER_PLAYED);
            setLed(col + 0, row - state, color, cellDisplay, LED_LAYER_PLAYED);
            setLed(col + 1, row - state, color, cellDisplay, LED_LAYER_PLAYED);
            setLed(col - state, row - 1, color, cellDisplay, LED_LAYER_PLAYED);
            setLed(col - state, row + 0, color, cellDisplay, LED_LAYER_PLAYED);
            setLed(col - state, row + 1, color, cellDisplay, LED_LAYER_PLAYED);
            setLed(col + state, row - 1, color, cellDisplay, LED_LAYER_PLAYED);
            setLed(col + state, row + 0, color, cellDisplay, LED_LAYER_PLAYED);
            setLed(col + state, row + 1, color, cellDisplay, LED_LAYER_PLAYED);
            break;
          case 3:
            setLed(col - 2, row + state - 1, color, cellDisplay, LED_LAYER_PLAYED);
            setLed(col - 1, row + state, color, cellDisplay, LED_LAYER_PLAYED);
            setLed(col + 0, row + state, color, cellDisplay, LED_LAYER_PLAYED);
            setLed(col + 1, row + state, color, cellDisplay, LED_LAYER_PLAYED);
            setLed(col + 2, row + state - 1, color, cellDisplay, LED_LAYER_PLAYED);
            setLed(col - 2, row - state + 1, color, cellDisplay, LED_LAYER_PLAYED);
            setLed(col - 1, row - state, color, cellDisplay, LED_LAYER_PLAYED);
            setLed(col + 0, row - state, color, cellDisplay, LED_LAYER_PLAYED);
            setLed(col + 1, row - state, color, cellDisplay, LED_LAYER_PLAYED);
            setLed(col + 2, row - state + 1, color, cellDisplay, LED_LAYER_PLAYED);
            setLed(col - state, row - 1, color, cellDisplay, LED_LAYER_PLAYED);
            setLed(col - state, row + 0, color, cellDisplay, LED_LAYER_PLAYED);
            setLed(col - state, row + 1, color, cellDisplay, LED_LAYER_PLAYED);
            setLed(col + state, row - 1, color, cellDisplay, LED_LAYER_PLAYED);
            setLed(col + state, row + 0, color, cellDisplay, LED_LAYER_PLAYED);
            setLed(col + state, row + 1, color, cellDisplay, LED_LAYER_PLAYED);
            break;
          case 4:
            setLed(col - 3, row + state - 1, color, cellDisplay, LED_LAYER_PLAYED);
            setLed(col - 2, row + state - 1, color, cellDisplay, LED_LAYER_PLAYED);
            setLed(col - 1, row + state, color, cellDisplay, LED_LAYER_PLAYED);
            setLed(col + 0, row + state, color, cellDisplay, LED_LAYER_PLAYED);
            setLed(col + 1, row + state, color, cellDisplay, LED_LAYER_PLAYED);
            setLed(col + 2, row + state - 1, color, cellDisplay, LED_LAYER_PLAYED);
            setLed(col + 3, row + state - 1, color, cellDisplay, LED_LAYER_PLAYED);
            setLed(col - 3, row - state + 1, color, cellDisplay, LED_LAYER_PLAYED);
            setLed(col - 2, row - state + 1, color, cellDisplay, LED_LAYER_PLAYED);
            setLed(col - 1, row - state, color, cellDisplay, LED_LAYER_PLAYED);
            setLed(col + 0, row - state, color, cellDisplay, LED_LAYER_PLAYED);
            setLed(col + 1, row - state, color, cellDisplay, LED_LAYER_PLAYED);
            setLed(col + 2, row - state + 1, color, cellDisplay, LED_LAYER_PLAYED);
            setLed(col + 3, row - state + 1, color, cellDisplay, LED_LAYER_PLAYED);
            setLed(col - state + 1, row - 2, color, cellDisplay, LED_LAYER_PLAYED);
            setLed(col - state, row - 1, color, cellDisplay, LED_LAYER_PLAYED);
            setLed(col - state, row + 0, color, cellDisplay, LED_LAYER_PLAYED);
            setLed(col - state, row + 1, color, cellDisplay, LED_LAYER_PLAYED);
            setLed(col - state + 1, row + 2, color, cellDisplay, LED_LAYER_PLAYED);
            setLed(col + state - 1, row - 2, color, cellDisplay, LED_LAYER_PLAYED);
            setLed(col + state, row - 1, color, cellDisplay, LED_LAYER_PLAYED);
            setLed(col + state, row + 0, color, cellDisplay, LED_LAYER_PLAYED);
            setLed(col + state, row + 1, color, cellDisplay, LED_LAYER_PLAYED);
            setLed(col + state - 1, row + 2, color, cellDisplay, LED_LAYER_PLAYED);
            break;
        }
        break;
      case playedCross:
        setLed(col, row + state, color, cellDisplay, LED_LAYER_PLAYED);
        setLed(col, row - state, color, cellDisplay, LED_LAYER_PLAYED);
        setLed(col - state, row, color, cellDisplay, LED_LAYER_PLAYED);
        setLed(col + state, row, color, cellDisplay, LED_LAYER_PLAYED);
        break;
      case playedStars:
        if (state % 2 == 1) {
          setLed(col, row + state, color, cellDisplay, LED_LAYER_PLAYED);
          setLed(col, row - state, color, cellDisplay, LED_LAYER_PLAYED);
          setLed(col - state, row, color, cellDisplay, LED_LAYER_PLAYED);
          setLed(col + state, row, color, cellDisplay, LED_LAYER_PLAYED);
        }
        else {
          int half_state =  state / 2;
          setLed(col - half_state, row + half_state, color, cellDisplay, LED_LAYER_PLAYED);
          setLed(col + half_state, row - half_state, color, cellDisplay, LED_LAYER_PLAYED);
          setLed(col - half_state, row - half_state, color, cellDisplay, LED_LAYER_PLAYED);
          setLed(col + half_state, row + half_state, color, cellDisplay, LED_LAYER_PLAYED);
        }
        break;
    }
  }
}

void performAdvanceTouchAnimations(unsigned long nowMillis) {
  bool buffer_started = false;

  for (byte row = 0; row < NUMROWS; ++row) {
    int32_t colsInRowAnimated = colsInRowsAnimated[row];
    while (colsInRowAnimated) {
      byte col = 31 - __builtin_clz(colsInRowAnimated);
      if (!buffer_started) {
        startBufferedLeds();
        buffer_started = true;
      }
      signed char state = calcTimeDelta(nowMillis, touchAnimationStart[col][row]) / touchAnimationSpeed[col][row];
      if (state != touchAnimationLastState[col][row]) {
        drawTouchedAnimation(col, row, cellOff, touchAnimationLastState[col][row]);
      }
      colsInRowAnimated &= ~(1 << col);
    }
  }

  if (buffer_started) {
    for (byte row = 0; row < NUMROWS; ++row) {
      int32_t colsInRowAnimated = colsInRowsAnimated[row];
      while (colsInRowAnimated) {
        byte col = 31 - __builtin_clz(colsInRowAnimated);

        signed char state = calcTimeDelta(nowMillis, touchAnimationStart[col][row]) / touchAnimationSpeed[col][row];
        drawTouchedAnimation(col, row, cellOn, state);
      
        colsInRowAnimated &= ~(1 << col);
      }
    }

    finishBufferedLeds();
  }
}
