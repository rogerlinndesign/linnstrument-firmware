/************************** ls_promo: LinnStrument Promotional Animation **************************
This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/
or send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
***************************************************************************************************
This shows a promotional animation that explains what's unique about the LinnStrument
**************************************************************************************************/

void playPromoAnimation() {
  Device.promoAnimationActive = true;
  storeSettings();

  setDisplayMode(displayPromo);

  while (!stopAnimation) {
    clearFullDisplay();
    big_scroll_text("     LINNSTRUMENT", COLOR_GREEN);

    if (stopAnimation) break;

    animationActive = true;
    clearFullDisplay();
    for (byte l = 0; l < 30 && !stopAnimation; ++l) {
      for (byte col = 1; col < NUMCOLS && !stopAnimation; ++col) {
        for (byte row = 0; row < NUMROWS && !stopAnimation; ++row) {
          setLed(col, row, random(0, 7), cellOn);
        }
      }  
      delayUsecWithScanning(100000);
    }
    animationActive = false;

    if (stopAnimation) break;

    clearFullDisplay();
    big_scroll_text("     PRESSURE SENSING", COLOR_MAGENTA);

    if (stopAnimation) break;

    animationActive = true;
    clearFullDisplay();
    paintNormalDisplay();
    for (byte l = 0; l < 5 && !stopAnimation; ++l) {
      byte col = random(3, 24);
      byte row = random(2, 6);
      setLed(col, row, COLOR_RED, cellOn);
      delayUsecWithScanning(100000);
      setLed(col-1, row, COLOR_RED, cellOn);
      setLed(col+1, row, COLOR_RED, cellOn);
      setLed(col,   row-1, COLOR_RED, cellOn);
      setLed(col,   row+1, COLOR_RED, cellOn);
      delayUsecWithScanning(100000);
      setLed(col-2, row, COLOR_RED, cellOn);
      setLed(col+2, row, COLOR_RED, cellOn);
      setLed(col,   row-2, COLOR_RED, cellOn);
      setLed(col,   row+2, COLOR_RED, cellOn);
      setLed(col-1, row-1, COLOR_RED, cellOn);
      setLed(col-1, row+1, COLOR_RED, cellOn);
      setLed(col+1, row-1, COLOR_RED, cellOn);
      setLed(col+1, row+1, COLOR_RED, cellOn);
      delayUsecWithScanning(100000);
      paintNormalDisplayCell(Global.currentPerSplit, col-2, row);
      paintNormalDisplayCell(Global.currentPerSplit, col+2, row);
      paintNormalDisplayCell(Global.currentPerSplit, col,   row-2);
      paintNormalDisplayCell(Global.currentPerSplit, col,   row+2);
      paintNormalDisplayCell(Global.currentPerSplit, col-1, row-1);
      paintNormalDisplayCell(Global.currentPerSplit, col-1, row+1);
      paintNormalDisplayCell(Global.currentPerSplit, col+1, row-1);
      paintNormalDisplayCell(Global.currentPerSplit, col+1, row+1);
      delayUsecWithScanning(100000);
      paintNormalDisplayCell(Global.currentPerSplit, col-1, row);
      paintNormalDisplayCell(Global.currentPerSplit, col+1, row);
      paintNormalDisplayCell(Global.currentPerSplit, col,   row-1);
      paintNormalDisplayCell(Global.currentPerSplit, col,   row+1);
      delayUsecWithScanning(100000);
      paintNormalDisplayCell(Global.currentPerSplit, col, row);
      delayUsecWithScanning(100000);
    }
    animationActive = false;

    if (stopAnimation) break;

    clearFullDisplay();
    big_scroll_text("     PITCH SLIDES", COLOR_YELLOW);

    if (stopAnimation) break;
    
    animationActive = true;
    clearFullDisplay();
    paintNormalDisplay();
    for (byte l = 0; l < 5 && !stopAnimation; ++l) {
      byte row = random(0, NUMROWS);
      for (byte col = 1; col < NUMCOLS && !stopAnimation; ++col) {
        setLed(col, row, COLOR_RED, cellOn);
        delayUsecWithScanning(30000);
        paintNormalDisplayCell(Global.currentPerSplit, col, row);
      }  
      for (byte col = NUMCOLS-2; col >= 1 && !stopAnimation; --col) {
        setLed(col, row, COLOR_RED, cellOn);
        delayUsecWithScanning(30000);
        paintNormalDisplayCell(Global.currentPerSplit, col, row);
      }  
      delayUsecWithScanning(100000);
    }
    animationActive = false;

    if (stopAnimation) break;

    clearFullDisplay();
    big_scroll_text("     Y-AXIS CONTROL", COLOR_BLUE);

    if (stopAnimation) break;
    
    animationActive = true;
    clearFullDisplay();
    paintNormalDisplay();
    for (byte l = 0; l < 5 && !stopAnimation; ++l) {
      byte col = random(1, NUMCOLS);
      for (byte row = 0; row < NUMROWS && !stopAnimation; ++row) {
        setLed(col, row, COLOR_RED, cellOn);
        delayUsecWithScanning(60000);
        paintNormalDisplayCell(Global.currentPerSplit, col, row);
      }  
      for (short row = NUMROWS-2; row >= 0 && !stopAnimation; --row) {
        setLed(col, row, COLOR_RED, cellOn);
        delayUsecWithScanning(60000);
        paintNormalDisplayCell(Global.currentPerSplit, col, row);
      }  
      delayUsecWithScanning(100000);
    }
    animationActive = false;

    if (stopAnimation) break;

    clearFullDisplay();
    big_scroll_text("     POLYPHONIC", COLOR_RED);

    if (stopAnimation) break;
    
    animationActive = true;
    clearFullDisplay();
    paintNormalDisplay();
    for (byte l = 0; l < 6 && !stopAnimation; ++l) {
      byte dir = random(0, 2);
      byte pattern = random(0, 2);
      byte col;
      if (dir) {
        col = 1 + random(0, 3);
      }
      else {
        col = NUMCOLS - 4 - random(0, 3);
      }
      byte row = random(0, NUMROWS-2);
      while (!stopAnimation) {
        if (pattern) {
          setLed(col+1, row+0, COLOR_RED, cellOn);
          setLed(col+1, row+1, COLOR_RED, cellOn);
          setLed(col, row+2, COLOR_RED, cellOn);
          delayUsecWithScanning(80000);
          paintNormalDisplayCell(Global.currentPerSplit, col+1, row+0);
          paintNormalDisplayCell(Global.currentPerSplit, col+1, row+1);
          paintNormalDisplayCell(Global.currentPerSplit, col, row+2);
        }
        else {
          setLed(col, row+0, COLOR_RED, cellOn);
          setLed(col+3, row+0, COLOR_RED, cellOn);
          setLed(col+1, row+1, COLOR_RED, cellOn);
          delayUsecWithScanning(80000);
          paintNormalDisplayCell(Global.currentPerSplit, col, row+0);
          paintNormalDisplayCell(Global.currentPerSplit, col+3, row+0);
          paintNormalDisplayCell(Global.currentPerSplit, col+1, row+1);
        }

        if (dir && (++col >= (2 * NUMCOLS) / 3)) {
          break;
        }
        else if (!dir && (--col < NUMCOLS / 3)) {
          break;
        }
      }
      delayUsecWithScanning(150000);
    }
    animationActive = false;

    if (stopAnimation) break;

    clearFullDisplay();
    big_scroll_text("     OPEN & CUSTOMIZABLE", COLOR_BLUE);

    if (stopAnimation) break;

    delayUsecWithScanning(1000000);
  }
  stopAnimation = false;
  animationActive = false;
  clearFullDisplay();
  Device.promoAnimationActive = false;
  storeSettings();
  
  lastTouchMoment = millis();
  controlButton = -1;
  setDisplayMode(displayNormal);
  updateDisplay();
}