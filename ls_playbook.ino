/*************************** ls_playbook: Displays a playbook of images ***************************
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
This cycles through a series of images to creation an animated playbook.
**************************************************************************************************/

const int CHRISTMAS_FRAME_COUNT = 7;
const char* CHRISTMAS_FRAMES_200[CHRISTMAS_FRAME_COUNT] = {
    " W    W       W    W  W   "
    "  W W  G    W  W    W    W"
    "      GGRW    W      G    "
    "   W  RGG       WG  RGG W "
    "  W WGGGGGW W   RGG GGR   "
    "     RGGRG    W GGRGGRGG W"
    " W  GGRGGGGW   GRGGGGGGRG "
    " WWWWWWYWWWWWWWWWYWWWYWWWW",

    " W     W   W     W        "
    " W    WG      W    W  W   "
    "  W W GGR   W  W    WG   W"
    "      RGGW    W  G  RGG   "
    "   W GGGGG      RGG GGR W "
    "  W WRGGRGW W   GGRGGRGG  "
    "    GGRGGGG   WGRGGGGGGRGW"
    " WWWWWWYWWWWWWWWWYWWWYWWWW",

    "              W     W    W"
    " W     G   W     W        "
    " W    GGR     W    W GW   "
    "  W W RGG   W  W G  RGG  W"
    "     GGGGG    W RGG GGR   "
    "   W RGGRG      GGRGGRGGW "
    "  W GGRGGGG W  GRGGGGGGRG "
    " WWWWWWYWWWWWWWWWYWWWYWWWW",

    "  W W W   W W     W   W   "
    "       G      W     W    W"
    " W    GGR  W     W   G    "
    " W    RGG     W  G WRGG   "
    "  W WGGGGG  W  WRGG GGR  W"
    "     RGGRG    W GGRGGRGG  "
    "   WGGRGGGG    GRGGGGGGRG "
    " WWWWWWYWWWWWWWWWYWWWYWWWW",

    "   W            W       W "
    "  W W WG  W W     W   W   "
    "      GGR     W     WG   W"
    " W    RGG  W     G  RGG   "
    " W   GGGGG    W RGGWGGR   "
    "  W WRGGRG  W  WGGRGGRGG W"
    "    GGRGGGG   WGRGGGGGGRG "
    " WWWWWWYWWWWWWWWWYWWWYWWWW",

    "         W    W           "
    "   W   G        W       W "
    "  W W GGR W W     W  GW   "
    "      RGG     W  G  RGG  W"
    " W   GGGGG W    RGG GGR   "
    " W   RGGRG    W GGRGGRGG  "
    "  W GGRGGGG W  GRGGGGGGRGW"
    " WWWWWWYWWWWWWWWWYWWWYWWWW",

    "  W W       W  W    W    W"
    "       G W    W           "
    "   W  GGR       W    G  W "
    "  W W RGG W W    GW RGG   "
    "     GGGGG    W RGG GGR  W"
    " W   RGGRG W    GGRGGRGG  "
    " W  GGRGGGG   WGRGGGGGGRG "
    " WWWWWWYWWWWWWWWWYWWWYWWWW"
  };
const char* CHRISTMAS_FRAMES_128[CHRISTMAS_FRAME_COUNT] = {
    "  W    W       W          "
    " W W W  G    W  W         "
    "       GGRW    W          "
    "    W  RGG                "
    "   W WGGGGGW W            "
    " W    RGGRG    W          "
    "  W  GGRGGGGW             "
    " WWWWWWWYWWWWWWWW         ",

    "  W     W   W             "
    "  W    WG      W          "
    " W W W GGR   W  W         "
    "       RGGW    W          "
    "    W GGGGG               "
    "   W WRGGRGW W            "
    " W   GGRGGGG   W          "
    " WWWWWWWYWWWWWWWW         ",

    " W             W          "
    "  W     G   W             "
    "  W    GGR     W          "
    " W W W RGG   W  W         "
    "      GGGGG    W          "
    "    W RGGRG               "
    "   W GGRGGGG W            "
    " WWWWWWWYWWWWWWWW         ",

    "   W W W   W W            "
    " W      G      W          "
    "  W    GGR  W             "
    "  W    RGG     W          "
    " W W WGGGGG  W  W         "
    "      RGGRG    W          "
    "    WGGRGGGG              "
    " WWWWWWWYWWWWWWWW         ",

    "    W                     "
    "   W W WG  W W            "
    " W     GGR     W          "
    "  W    RGG  W             "
    "  W   GGGGG    W          "
    " W W WRGGRG  W  W         "
    "     GGRGGGG   W          "
    " WWWWWWWYWWWWWWWW         ",

    "          W    W          "
    "    W   G                 "
    "   W W GGR W W            "
    " W     RGG     W          "
    "  W   GGGGG W             "
    "  W   RGGRG    W          "
    " W W GGRGGGG W  W         "
    " WWWWWWWYWWWWWWWW         ",

    " W W W       W  W         "
    "        G W    W          "
    "    W  GGR                "
    "   W W RGG W W            "
    " W    GGGGG    W          "
    "  W   RGGRG W             "
    "  W  GGRGGGG   W          "
    " WWWWWWWYWWWWWWWW         "
  };

void playChristmasAnimation() {
  if (LINNMODEL == 200) {
    playPlayBook(CHRISTMAS_FRAME_COUNT, CHRISTMAS_FRAMES_200);
  }
  else if (LINNMODEL == 128) {
    playPlayBook(CHRISTMAS_FRAME_COUNT, CHRISTMAS_FRAMES_128);
  }
}

void playPlayBook(int totalFrames, const char** playbook) {
  Device.sleepAnimationActive = true;
  storeSettings();

  setDisplayMode(displayAnimation);
  clearFullDisplay();

  int frameIndex = 0;
  while (!stopAnimation) {
    animationActive = true;
    const char* frameData = playbook[frameIndex];
    startBufferedLeds();
    for (int r = MAXROWS-1; r >= 0; --r) {
      for (int c = 0; c < MAXCOLS; ++c) {
        setLed(c, r, colorCharToNumber(*frameData), cellOn);
        frameData++;
      }
      performContinuousTasks();
    }
    finishBufferedLeds();
    delayUsecWithScanning(400000);
    animationActive = false;

    if (++frameIndex >= totalFrames) {
      frameIndex = 0;
    }
  }
  stopAnimation = false;
  animationActive = false;
  clearFullDisplay();
  Device.sleepAnimationActive = false;
  storeSettings();

  lastTouchMoment = millis();
  setDisplayMode(displayNormal);
  updateDisplay();
}

byte colorCharToNumber(char color) {
  switch (color) {
    case 'B': return 5;
    case 'C': return 4;
    case 'G': return 3;
    case 'L': return 10;
    case 'M': return 6;
    case 'O': return 9;
    case 'P': return 11;
    case 'R': return 1;
    case 'W': return 8;
    case 'Y': return 2;
  }
  return 0;
}