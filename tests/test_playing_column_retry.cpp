// Whole sketch, real Z predicates and MIDI queue; only sensor inputs/clock are mocked.
namespace columnRetry {
void scan(byte col, byte row) {
  linn::select(col,row);sensorCell->shouldRefreshData();
  modeLoopPerformance();host::advanceMicros(1000);
}


void hold(byte col,byte row,int x,int y,int z,int count=3) {
  host::setSensor(col,row,x,y,z);
  for(int n=0;n<count;++n)scan(col,row);
}
void press(byte col,byte row,int x,int y,int z) {
  for(int n=1;n<=VELOCITY_TOTAL_SAMPLES;++n)
    hold(col,row,x,y,z*n/VELOCITY_TOTAL_SAMPLES,1);
  hold(col,row,x,y,z,4);
}
void release(byte col,byte row) {
  for(int n=0;n<4;++n) {hold(col,row,0,0,0,1);host::advanceMicros(30000);}
}
}
TEST(column_retry_transfers_without_repress_or_extra_notes) {
  for(byte model : {byte(128),byte(200)}) for(int direction : {-1,1}) {
    linn::reset(model);
    Global.velocitySensitivity=velocityFixed; Global.valueForFixedVelocity=77;
    Split[LEFT].midiMode=channelPerNote; Split[LEFT].sendX=true;
    Split[LEFT].mpe=true;Split[LEFT].midiChanMain=1;
    for(byte ch=0;ch<16;++ch)Split[LEFT].midiChanSet[ch]=ch!=0;
    Split[LEFT].lowRowMode=lowRowNormal;
    updateSplitMidiChannels(LEFT); sent();
    byte from=direction==1?6:7,to=from+direction;
    int centre=FXD_TO_INT(calculateDefaultMeasuredX(from));
    int edge=(centre+FXD_TO_INT(calculateDefaultMeasuredX(to)))/2;
    int bad=edge+direction*200;
    int notes[8],channels[8];
    for(byte r=0;r<8;++r) {
      columnRetry::press(from,r,centre,2000,900);
      notes[r]=cell(from,r).note;channels[r]=cell(from,r).channel;
      CHECK(cell(from,r).hasNote());
      columnRetry::hold(from,r,edge,2000,350);
    }
    CHECK_EQ(notesIn(sent()).size(),8);
    for(byte r=0;r<3;++r)columnRetry::press(to,r,edge,2000,900);
    columnRetry::press(to,3,bad,2000,900);
    CHECK_EQ(cell(to,3).touched,ignoredCell);CHECK(cell(to,3).columnLimited);
    CHECK(!cell(to,3).hasNote());
    CHECK_EQ(rowsInColsTouched[to]&15,15);
    for(byte r=4;r<8;++r)columnRetry::press(to,r,edge,2000,900);
    CHECK_EQ(notesIn(sent()).size(),0);
    int32_t rows=rowsInColsTouched[to],cols=colsInRowsTouched[3];
    columnRetry::hold(to,3,bad,2000,900,100);
    CHECK_EQ(cell(to,3).touched,ignoredCell);
    CHECK_EQ(rowsInColsTouched[to],rows);CHECK_EQ(colsInRowsTouched[3],cols);
    CHECK_EQ(notesIn(sent()).size(),0);
    // Existing meaningful-vs-active distinction is deliberate: feather alone
    // cannot create a newly accepted note, even when the X match improves.
    int feather=0;
    for(int z=1;z<300;++z) {
      host::setSensor(to,3,edge,2000,z);linn::select(to,3);
      sensorCell->shouldRefreshData();
      if(sensorCell->isActiveTouch() && !sensorCell->isMeaningfulTouch()) {feather=z;break;}
    }
    CHECK(feather>0);
    columnRetry::hold(to,3,edge,2000,feather,10);
    CHECK_EQ(cell(to,3).touched,ignoredCell);CHECK(cell(to,3).columnLimited);
    CHECK_EQ(notesIn(sent()).size(),0);
    // Restore the original pressure without releasing the destination pad.
    columnRetry::hold(to,3,edge,2000,900,6);
    CHECK(cell(to,3).hasNote());CHECK(!cell(from,3).hasNote());
    CHECK_EQ(cell(to,3).note,notes[3]);CHECK_EQ(cell(to,3).channel,channels[3]);
    CHECK(!cell(to,3).columnLimited);CHECK_EQ(notesIn(sent()).size(),0);
    for(byte r=0;r<8;++r) {columnRetry::release(from,r);columnRetry::release(to,r);}
    auto off=notesIn(sent()); CHECK_EQ(off.size(),8);
    for(byte r=0;r<8;++r) {
      CHECK(!cell(from,r).hasNote());CHECK(!cell(to,r).hasNote());
      int matches=0;
      for(auto& message:off) if((message[0]&0xf0)==0x80 && message[1]==notes[r] &&
                                (message[0]&15)+1==channels[r]) ++matches;
      CHECK_EQ(matches,1);
    }
    CHECK_EQ(cellsTouched,0);
    // Other ignore reasons and a reset must not inherit the retry reason.
    linn::select(to,3);cellTouched(ignoredCell);
    columnRetry::hold(to,3,edge,2000,900,10);
    CHECK_EQ(cell(to,3).touched,ignoredCell);CHECK(!cell(to,3).columnLimited);
    CHECK_EQ(notesIn(sent()).size(),0);
    cell(to,3).columnLimited=true;cell(to,3).clearSensorData();
    CHECK(!cell(to,3).columnLimited);
  }
}

TEST(column_retry_admits_after_the_old_pad_releases) {
  linn::reset(128);
  Global.velocitySensitivity=velocityFixed;Global.valueForFixedVelocity=77;
  Split[LEFT].midiMode=channelPerNote;updateSplitMidiChannels(LEFT);sent();
  int fromX=FXD_TO_INT(calculateDefaultMeasuredX(6));
  int toX=FXD_TO_INT(calculateDefaultMeasuredX(7));
  for(byte row=1;row<4;++row)columnRetry::press(7,row,toX,2000,900);
  columnRetry::press(6,4,fromX,2000,900);
  CHECK_EQ(notesIn(sent()).size(),4);
  int oldNote=cell(6,4).note;
  columnRetry::press(7,4,toX,2000,900);
  CHECK_EQ(cell(7,4).touched,ignoredCell);CHECK(cell(7,4).columnLimited);
  CHECK_EQ(notesIn(sent()).size(),0);
  columnRetry::release(6,4);
  auto oldOff=notesIn(sent());CHECK_EQ(oldOff.size(),1);
  if(oldOff.size()==1)CHECK_EQ(oldOff[0][1],oldNote);
  CHECK_EQ(colsInRowsTouched[4],1<<7);
  columnRetry::hold(7,4,toX,2000,900,VELOCITY_TOTAL_SAMPLES+4);
  CHECK(cell(7,4).hasNote());CHECK(!cell(7,4).columnLimited);
  auto newOn=notesIn(sent());CHECK_EQ(newOn.size(),1);
  if(newOn.size()==1)CHECK_EQ(newOn[0][0]&0xf0,0x90);
  for(byte row=1;row<=4;++row)columnRetry::release(7,row);
  CHECK_EQ(notesIn(sent()).size(),4);CHECK_EQ(cellsTouched,0);
}
