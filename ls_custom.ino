/************************** ls_promo: LinnStrument Promotional Animation **************************
This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/
or send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
***************************************************************************************************
This shows custom animations 1-8 - jas 2015/01/04
**************************************************************************************************/

void playCustomAnimation() {
  setDisplayMode(displayCustom);
  byte animations = 0;
  byte animationNumber = 0;
  byte activeAnimations[8];
  for(byte i=0; i<8; i++){
      if(Global.customAnimations[i]){
          activeAnimations[animations++] = i;
          animationNumber = i;
      }
  }
  boolean medley = animations != 1;
  paintNormalDisplay();
  paintCustom();
  while (!stopAnimation) {
    animationActive = true;
    switch (animationNumber) {
      case 0:
        playSparkle();
        break;
      case 1:
        playPressure();
        break;
      case 2:
        playXAxis();
        break;
      case 3:
        playYAxis();
        break;
      case 4:
        playPoly();
        break;
      case 5:
        playLife();
        break;
      case 6:
        playFollow();
        break;
      case 7:
        playSnake();
        break;
      default:
        medley = true;
        paintNormalDisplay();
        paintCustom();
    }
    if (medley) {
      animationNumber = (animations > 1) ? activeAnimations[random(0, animations)] : random(0, 8);
    }
  }
  stopAnimation = false;
  animationActive = false;
  clearDisplay();
  clearLed(0,0);

  controlButton = -1;
  setDisplayMode(displayNormal);
  updateDisplay();
}
void paintCustom() { //-- in-progress
// for (byte col=17; col<25; col++) {
//     setLed(col, 7, COLOR_BLUE, cellPulse);
// }
  setLed(0, 0, COLOR_RED, cellPulse);
}

void paintCustomCell(byte col, byte row) {
  if (col > 0 && col < NUMCOLS && row > -1 && row < NUMROWS) paintNormalDisplayCell(Global.currentPerSplit, col, row);
  //  byte color = leds[col][row] >> 4; //-- if needed, this to ls_leds to bring leds array in scope, jas 2015/01/07 --
}
void setCustomLed(byte col, byte row, byte color) {
  if (col > 0 && col < NUMCOLS && row > -1 && row < NUMROWS) setLed(col, row, color, cellOn);
  //  byte color = leds[col][row] >> 4; //-- if needed, this to ls_leds to bring leds array in scope, jas 2015/01/07 --
}

void playSparkle() { // sparkle lights
  byte steps = 10;
  byte rows[steps];
  byte cols[steps];
  byte colors[steps];
  byte row, col, color, dx, dy, dx2, dy2;
  byte baseRow = random(0, 5);
  byte baseCol = random(1, 18);

  for (byte seg = 0; seg < steps && !stopAnimation; ++seg) { // make a random figure from a small group of cells
    rows[seg] = baseRow + random(0, 3);
    cols[seg]   = baseCol + random(0, 8);
    colors[seg] = random(1, 7);
    }

  for (byte repeat = 0; repeat < 40 && !stopAnimation; ++repeat) { // move the figure around as a unit
    for (byte seg = 0; seg < steps && !stopAnimation; ++seg) {
      setCustomLed(cols[seg], rows[seg], colors[seg]); //show it in current position
    }
    delayUsecWithScanning(100000);

    dx = random(0,3) - 1; //random walk for the unit
    dy = random(0,3) - 1;

    for (byte seg = 0; seg < steps && !stopAnimation; ++seg) {
      paintCustomCell(cols[seg], rows[seg]); // erase the old position before calculating new one
      dx2 = random(0,3) - 1; //random walk individual cells in the unit
      dy2 = random(0,3) - 1;
      cols[seg] += dx2;
      rows[seg] += dy2;
    }
  }
}

void playFollow() { // follow the leader - independent cells follow kinda
  byte steps = 10;
  byte rows[steps];
  byte cols[steps];
  byte colors[steps];
  byte color, dx2, dy2, x1, y1;
  byte row = random(2, 5);
  byte col = random(6, 15);

  for (byte seg = 0; seg < steps && !stopAnimation; ++seg) { // initialize random pattern
    row += random(0, 3) - 1;
    col++;
    rows[seg] = row;
    cols[seg] = col;
    colors[seg] = random(1, 7);
    }

  for (byte repeat = 0; repeat < 40 && !stopAnimation; ++repeat) { // move the figure around as a unit
    for (byte seg = 0; seg < steps && !stopAnimation; ++seg) {
      setCustomLed(cols[seg], rows[seg], colors[seg]); //show it in current position
    }
    delayUsecWithScanning(100000);

    x1 = cols[0]; // old position of leader
    y1 = rows[0];

    for (byte seg = 0; seg < steps && !stopAnimation; ++seg) {
      paintCustomCell(cols[seg], rows[seg]); // erase the old position before calculating new one
      cols[seg] += random(0,3) - 1 + (cols[seg] < x1 - 1 ? 1 : (cols[seg] > x1 + 1 ? -1 : 0)); // random walk towards leader
      rows[seg] += random(0,3) - 1 + (rows[seg] < y1 - 1 ? 1 : (rows[seg] > y1 + 1 ? -1 : 0));
      x1 = cols[seg];
      y1 = rows[seg];
    }
  }
}

void playSnake() { // body cells follow the head
  byte steps = 10;
  byte rows[steps];
  byte cols[steps];
  byte colors[steps];
  byte color, dx, dy, dx2, dy2;
  byte row = random(2, 6);
  byte col = random(6, 12);

  for (byte seg = 0; seg < steps && !stopAnimation; ++seg) { // initialize position and color of cells
    row += random(0, 3) - 1;
    if (row < 0) row = 0;
    if (row >= NUMROWS) row = NUMROWS - 1;
    col++;
    rows[seg] = row;
    cols[seg] = col;
    colors[seg] = random(1, 7);
    }

  for (byte repeat = 0; repeat < 40 && !stopAnimation; ++repeat) { // move all cells
    for (byte seg = 0; seg < steps && !stopAnimation; ++seg) {
      setCustomLed(cols[seg], rows[seg], colors[seg]); //paint from back to front
    }

    delayUsecWithScanning(100000);

    paintCustomCell(cols[0], rows[0]); // erase the old tail position before calculating new

    for (byte seg = 0; seg < steps - 1 && !stopAnimation; ++seg) { // each cell follows previous
      cols[seg] = cols[seg + 1];
      rows[seg] = rows[seg + 1];
    }

    col = cols[steps-1] + random(0,3) - 1; //randowm walk for the head only
    row = rows[steps-1] + random(0,3) - 1;
    if (row < 0) row = 0;
    if (row >= NUMROWS) row = NUMROWS - 1;
    if (col < 1) col = 1;
    if (col >= NUMCOLS) col = NUMCOLS - 1;
    cols[steps-1] = col;
    rows[steps-1] = row;
  }
}

void playPressure() { // pressure points}
    //--paintNormalDisplay();
    byte col = random(3, 24);
    byte row = random(2, 6);
    setCustomLed(col, row, COLOR_RED);
    delayUsecWithScanning(100000);
    setCustomLed(col-1, row, COLOR_RED);
    setCustomLed(col+1, row, COLOR_RED);
    setCustomLed(col,   row-1, COLOR_RED);
    setCustomLed(col,   row+1, COLOR_RED);
    delayUsecWithScanning(100000);
    setCustomLed(col-2, row, COLOR_RED);
    setCustomLed(col+2, row, COLOR_RED);
    setCustomLed(col,   row-2, COLOR_RED);
    setCustomLed(col,   row+2, COLOR_RED);
    setCustomLed(col-1, row-1, COLOR_RED);
    setCustomLed(col-1, row+1, COLOR_RED);
    setCustomLed(col+1, row-1, COLOR_RED);
    setCustomLed(col+1, row+1, COLOR_RED);
    delayUsecWithScanning(100000);
    paintCustomCell(col-2, row);
    paintCustomCell(col+2, row);
    paintCustomCell(col,   row-2);
    paintCustomCell(col,   row+2);
    paintCustomCell(col-1, row-1);
    paintCustomCell(col-1, row+1);
    paintCustomCell(col+1, row-1);
    paintCustomCell(col+1, row+1);
    delayUsecWithScanning(100000);
    paintCustomCell(col-1, row);
    paintCustomCell(col+1, row);
    paintCustomCell(col,   row-1);
    paintCustomCell(col,   row+1);
    delayUsecWithScanning(100000);
    paintCustomCell(col, row);
    delayUsecWithScanning(100000);
}

void playXAxis() { // move lights in X
    //--paintNormalDisplay();
      byte row = random(0, NUMROWS);
      byte col;
      byte dir = random(0,2);
      for (byte seg = 1; seg < NUMCOLS && !stopAnimation; ++seg) {
        col = dir > 0 ? seg : (NUMCOLS - seg);
        setCustomLed(col, row, COLOR_RED);
        delayUsecWithScanning(30000);
        paintCustomCell(col, row);
      }  
      delayUsecWithScanning(100000);
}

void playYAxis() { // move lights in Y
    //--paintNormalDisplay();
      byte row;
      byte col = random(1, NUMCOLS);
      byte dir = random(0,2);
      for (byte seg = 0; seg < NUMROWS && !stopAnimation; ++seg) {
        row = dir > 0 ? seg : NUMROWS - seg;
        setCustomLed(col, row, COLOR_RED);
        delayUsecWithScanning(60000);
        paintCustomCell(col, row);
      }  
      delayUsecWithScanning(100000);
}

void playPoly() { // polyphonic lights
    //--paintNormalDisplay();
    byte dir = random(0, 2);
    byte pattern = random(0, 2);
    byte col;
    if (dir) {
      col = 1 + random(0, 3);
    }
    else {
      col = NUMCOLS - 2 - random(0, 3);
    }
    byte row = random(0, NUMROWS-2);
    while (!stopAnimation) {
      if (pattern) {
        setCustomLed(col, row+0, COLOR_RED);
        setCustomLed(col, row+1, COLOR_RED);
        setCustomLed(col-1, row+2, COLOR_RED);
        delayUsecWithScanning(60000);
        paintCustomCell(col, row+0);
        paintCustomCell(col, row+1);
        paintCustomCell(col-1, row+2);
      }
      else {
        setCustomLed(col, row+0, COLOR_RED);
        setCustomLed(col+3, row+0, COLOR_RED);
        setCustomLed(col+1, row+1, COLOR_RED);
        delayUsecWithScanning(60000);
        paintCustomCell(col, row+0);
        paintCustomCell(col+3, row+0);
        paintCustomCell(col+1, row+1);
      }

      if (dir && (++col >= NUMCOLS / 2)) {
        break;
      }
      else if (!dir && (--col < NUMCOLS / 3)) {
        break;
      }
    }
    delayUsecWithScanning(100000);
}


byte lifeCells[NUMCOLS][NUMROWS];
byte lifeNeighbors[NUMCOLS][NUMROWS];

void playLife() {
// Conway's game of life - with color
// 
//-- Any live cell with fewer than two live neighbours dies, as if caused by under-population.
//-- Any live cell with two or three live neighbours lives on to the next generation.
//-- Any live cell with more than three live neighbours dies, as if by overcrowding.
//-- Any dead cell with exactly three live neighbours becomes a live cell, as if by reproduction.

  // initialize cells with random color
  byte color, stepColor = random(1, 7); // one color per generation

  for (byte row=0; row<NUMROWS; row++){
    for (byte col=1; col<NUMCOLS; col++){ // remember - surface cols start at 1
      color = random(0, 6) > 0 ? 0 : stepColor ; // light only 1 of 6 cells (not too dense)
      lifeCells[col][row] = color;
      setCustomLed(col,row,color);
    }
  }

  boolean changed = true;

  for (int seg=0; seg<100 && changed && !stopAnimation; seg++){
    changed = false;
    stepColor = random(1, 7);

    // repeating phase for calculation of neighbors before changing cells

    // initialize neighbors
    for (byte row=0; row<NUMROWS; row++){
      for (byte col=1; col<NUMCOLS; col++){
        lifeNeighbors[col][row] = 0;
      }
    }

    // calculate neighbors
    // neighbors wraparound last row and col to first
    for (byte row=0; row<NUMROWS; row++){
      for (byte col=1; col<NUMCOLS; col++){
        byte prevCol = col>1 ? col-1 : NUMCOLS-1;
        byte prevRow = row>0 ? row-1 : NUMROWS-1;
        byte nextCol = col+1 < NUMCOLS ? col+1 : 1;
        color = lifeCells[col][row];
        if (color>0) lifeNeighbors[prevCol][row]++;
        if (lifeCells[prevCol][row]>0) lifeNeighbors[col][row]++;
        if (color>0) {
          lifeNeighbors[col][prevRow]++;
          lifeNeighbors[prevCol][prevRow]++;
          lifeNeighbors[nextCol][prevRow]++;
        }
        if (lifeCells[col][prevRow]>0) lifeNeighbors[col][row]++;
        if (lifeCells[prevCol][prevRow]>0) lifeNeighbors[col][row]++;
        if (lifeCells[nextCol][prevRow]>0) lifeNeighbors[col][row]++;
      }
    }
    
    // change state of cells based on neighbors

    for (byte row=0; row<NUMROWS && !stopAnimation; row++){
      for (byte col=1; col<NUMCOLS && !stopAnimation; col++){
          byte live = lifeCells[col][row] > 0;
          byte neighbors = lifeNeighbors[col][row];
          if (live){
              if (neighbors < 2 || neighbors > 3) {
                lifeCells[col][row] = 0;
                setCustomLed(col,row,0);
                changed = true;
              }
          }
          else if (neighbors == 3) {
            color = random(1, 7);
            lifeCells[col][row] = stepColor;
            setCustomLed(col,row,stepColor);
            changed = true;
          } 
      }
    }
  delayUsecWithScanning(120000);
  }
  if (!changed) delayUsecWithScanning(700000);
}

