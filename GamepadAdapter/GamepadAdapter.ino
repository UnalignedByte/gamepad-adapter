#include "GamepadAdapter.h"

// General
void tickHigh(int pin) {
  digitalWrite(pin, HIGH);
  delayMicroseconds(10);
  digitalWrite(pin, LOW);
}

void tickLow(int pin) {
  digitalWrite(pin, LOW);
  delayMicroseconds(10);
  digitalWrite(pin, HIGH);
}

void debounceAndUpdateInput(DebouncedInput *input, bool newState) {
  unsigned long currentTime = millis();
  if (currentTime - input->newReadingTime > debounceDuration) {
    if (newState == input->newState) {
      input->state = newState;
    } else {
      input->newState = newState;
      input->newReadingTime = currentTime;
    }
  }
}

void sendByte(uint8_t data, int dataPin, int clockPin) { 
  for (int i=0; i<8; i++) {
    uint8_t bit = (data >> i) & 0x01;
    digitalWrite(dataPin, bit ? HIGH : LOW);
  }
}

uint8_t receiveByte(int dataPin, int clockPin) {
  uint8_t data = 0;
  for (int i=0; i<8; i++) {
    tickLow(clockPin);
    uint8_t bit = digitalRead(dataPin) == HIGH;
    data |= bit << i;
  }
  return data;
}

uint8_t exchangeBytes(uint8_t txData, int txPin, int rxPin, int clockPin) {
  uint8_t rxData = 0;
  for (int i=0; i<8; i++) {
    uint8_t txBit = (txData >> i) & 0x01;
    digitalWrite(txPin, txBit ? HIGH : LOW);
    tickLow(clockPin);
    uint8_t rxBit = digitalRead(rxPin) == HIGH;
    rxData |= rxBit << i;
  }
  return rxData;
}

// SNES
void startSnesGamepad() {
  pinMode(SNES_LATCH, OUTPUT);
  pinMode(SNES_CLK, OUTPUT);
  pinMode(SNES_DATA, INPUT);

  digitalWrite(SNES_LATCH, LOW);
  digitalWrite(SNES_CLK, HIGH);
}

void updateState(SnesGamepadState *state) {
  tickHigh(SNES_LATCH);

  debounceAndUpdateInput(&state->b, digitalRead(SNES_DATA) == LOW);
  tickLow(SNES_CLK);
  debounceAndUpdateInput(&state->y, digitalRead(SNES_DATA) == LOW);
  tickLow(SNES_CLK);
  debounceAndUpdateInput(&state->select, digitalRead(SNES_DATA) == LOW);
  tickLow(SNES_CLK);
  debounceAndUpdateInput(&state->start, digitalRead(SNES_DATA) == LOW);
  tickLow(SNES_CLK);
  debounceAndUpdateInput(&state->up, digitalRead(SNES_DATA) == LOW);
  tickLow(SNES_CLK);
  debounceAndUpdateInput(&state->down, digitalRead(SNES_DATA) == LOW);
  tickLow(SNES_CLK);
  debounceAndUpdateInput(&state->left, digitalRead(SNES_DATA) == LOW);
  tickLow(SNES_CLK);
  debounceAndUpdateInput(&state->right, digitalRead(SNES_DATA) == LOW);
  tickLow(SNES_CLK);
  debounceAndUpdateInput(&state->a, digitalRead(SNES_DATA) == LOW);
  tickLow(SNES_CLK);
  debounceAndUpdateInput(&state->x, digitalRead(SNES_DATA) == LOW);
  tickLow(SNES_CLK);
  debounceAndUpdateInput(&state->l, digitalRead(SNES_DATA) == LOW);
  tickLow(SNES_CLK);
  debounceAndUpdateInput(&state->r, digitalRead(SNES_DATA) == LOW);
  
  return state;
}

bool isStateIdentical(SnesGamepadState *first, SnesGamepadState *second) {
  if (first->up.state != second->up.state ||
    first->down.state != second->down.state ||
    first->left.state != second->left.state ||
    first->right.state != second->right.state ||
    first->a.state != second->a.state ||
    first->b.state != second->b.state ||
    first->x.state != second->x.state ||
    first->y.state != second->y.state ||
    first->l.state != second->l.state ||
    first->r.state != second->r.state ||
    first->start.state != second->start.state ||
    first->select.state != second->select.state) {
    return false;
  } else {
    return true;
  }
}

void encodeState(SnesGamepadState *state, uint8_t *data) {
  // Direction
  bool up = state->up.state;
  bool right = state->right.state;
  bool down = state->down.state;
  bool left = state->left.state;
 
  // Up
  if (up && !right && !down && !left) {
    data[0] = 1;
  // Up Right
  } else if (up && right && !down && !left) {
    data[0] = 2;
  // Right
  } else if (!up && right && !down && !left) {
    data[0] = 3;
  // Right Down
  } else if (!up && right && down && !left) {
    data[0] = 4;
  // Down
  } else if (!up && !right && down && !left) {
    data[0] = 5;
  // Down Left
  } else if (!up && !right && down && left) {
    data[0] = 6;
  // Left
  } else if (!up && !right && !down && left) {
    data[0] = 7;
  // Left Up
  } else if (up && !right && !down && left) {
    data[0] = 8;
  } else {
    data[0] = 0;
  }

  // Buttons
  data[1] = 0;
  data[1] |= state->a.state << 0;
  data[1] |= state->b.state << 1;
  data[1] |= state->x.state << 2;
  data[1] |= state->y.state << 3;
  data[1] |= state->l.state << 4;
  data[1] |= state->r.state << 5;
  data[1] |= state->start.state << 6;
  data[1] |= state->select.state << 7;
}

void printDescriptionForState(SnesGamepadState *state) {
  Serial.print(state->up.state ? "UP, " : "up, ");
  Serial.print(state->down.state ? "DOWN, " : "down, ");
  Serial.print(state->left.state ? "LEFT, " : "left, ");
  Serial.print(state->right.state ? "RIGHT, " : "right, ");
  Serial.print(state->a.state ? "A, " : "a, ");
  Serial.print(state->b.state ? "B, " : "b, ");
  Serial.print(state->x.state ? "X, " : "x, ");
  Serial.print(state->y.state ? "Y, " : "y, ");
  Serial.print(state->l.state ? "L, " : "l, ");
  Serial.print(state->r.state ? "R, " : "r, ");
  Serial.print(state->start.state ? "START, " : "start, ");
  Serial.print(state->select.state ? "SELECT" : "select");
  Serial.println();
}

// DualShock
void startDualShock() {
  pinMode(DS_DATA, INPUT);
  pinMode(DS_CMD, OUTPUT);
  pinMode(DS_ATT, OUTPUT);
  pinMode(DS_CLK, OUTPUT);

  digitalWrite(DS_CLK, HIGH);
  digitalWrite(DS_ATT, HIGH);
  // Has to be pulled-up because the gamepad can't do it itself
  digitalWrite(DS_DATA, HIGH);
}

void updateState(DualShockState *state) {
  digitalWrite(DS_ATT, LOW);
  exchangeBytes(0x01, DS_CMD, DS_DATA, DS_CLK);
  uint8_t mode = exchangeBytes(0x42, DS_CMD, DS_DATA, DS_CLK);
  receiveByte(DS_DATA, DS_CLK);
  uint8_t data1 = ~receiveByte(DS_DATA, DS_CLK);
  uint8_t data2 = ~receiveByte(DS_DATA, DS_CLK);
  uint8_t data3;
  uint8_t data4;
  uint8_t data5;
  uint8_t data6;
  // 0x73 is Analog mode and the data has to be read straighth away
  if(mode == 0x73) {
    data3 = receiveByte(DS_DATA, DS_CLK);
    data4 = ~receiveByte(DS_DATA, DS_CLK);
    data5 = receiveByte(DS_DATA, DS_CLK);
    data6 = ~receiveByte(DS_DATA, DS_CLK);
  }
  digitalWrite(DS_ATT, HIGH);

  state->select = (data1 >> 0) & 0x01;
  state->start  = (data1 >> 3) & 0x01;
  state->up     = (data1 >> 4) & 0x01;
  state->right  = (data1 >> 5) & 0x01;
  state->down   = (data1 >> 6) & 0x01;
  state->left   = (data1 >> 7) & 0x01;

  state->l2       = (data2 >> 0) & 0x01;
  state->r2       = (data2 >> 1) & 0x01;
  state->l1       = (data2 >> 2) & 0x01;
  state->r1       = (data2 >> 3) & 0x01;
  state->triangle = (data2 >> 4) & 0x01;
  state->circle   = (data2 >> 5) & 0x01;
  state->cross    = (data2 >> 6) & 0x01;
  state->square   = (data2 >> 7) & 0x01;

  // 0x73 is analog mode
  if (mode == 0x73) {
    state->l3 = (data1 >> 1) & 0x01;
    state->r3 = (data1 >> 2) & 0x01;
    state->rx = data3;
    state->ry = data4;
    state->lx = data5;
    state->ly = data6;

    // Add a bit of dead zone
    const uint8_t deadZoneMin = 0x80 - 0x1C;
    const uint8_t deadZoneMax = 0x80 + 0x1C;
    if (state->lx > deadZoneMin && state->lx < deadZoneMax)
      state->lx = 0x80;
    if (state->ly > deadZoneMin && state->ly < deadZoneMax)
      state->ly = 0x80;
    if (state->rx > deadZoneMin && state->rx < deadZoneMax)
      state->rx = 0x80;
    if (state->ry > deadZoneMin && state->ry < deadZoneMax)
      state->ry = 0x80;
  // We're in digital mode (0x41), so reset everything to  default
  } else {
    state->l3 = false;
    state->r3 = false;
    // 0x80 is middle between 0x00 and 0xff
    state->lx = 0x80;
    state->ly = 0x80;
    state->rx = 0x80;
    state->ry = 0x80;
  }
}

bool isStateIdentical(DualShockState *first, DualShockState *second) {
  if (first->rx != second->rx || first->ry != second->ry || first->lx != second->lx || first->ly != second->ly ||
    first->up != second->up || first->down != second->down || first->left != second->left || first->right != second->right ||
    first->circle != second->circle || first->cross != second->cross || first->triangle != second->triangle || first->square != second->square ||
    first->l1 != second->l1 || first->l2 != second->l2 || first->l3 != second->l3 ||
    first->r1 != second->r1 || first->r2 != second->r2 || first->r3 != second->r3 ||
    first->start != second->start || first->select != second->select) {
    return false;
  } else {
    return true;
  }
}

void encodeState(DualShockState *state, uint8_t *data) {
  // Direction
  bool up = state->up;
  bool right = state->right;
  bool down = state->down;
  bool left = state->left;
  // Up
  if (up && !right && !down && !left) {
    data[0] = 1;
  // Up Right
  } else if (up && right && !down && !left) {
    data[0] = 2;
  // Right
  } else if (!up && right && !down && !left) {
    data[0] = 3;
  // Right Down
  } else if (!up && right && down && !left) {
    data[0] = 4;
  // Down
  } else if (!up && !right && down && !left) {
    data[0] = 5;
  // Down Left
  } else if (!up && !right && down && left) {
    data[0] = 6;
  // Left
  } else if (!up && !right && !down && left) {
    data[0] = 7;
  // Left Up
  } else if (up && !right && !down && left) {
    data[0] = 8;
  } else {
    data[0] = 0;
  }

  // Buttons 1
  data[1] = 0;
  data[1] |= state->circle   << 0;
  data[1] |= state->cross    << 1;
  data[1] |= state->triangle << 2;
  data[1] |= state->square   << 3;
  data[1] |= state->l1       << 4;
  data[1] |= state->l2       << 5;
  data[1] |= state->r1       << 6;
  data[1] |= state->r2       << 7;

  // Buttons 2
  data[2] = 0;
  data[2] |= state->start  << 0;
  data[2] |= state->select << 1;
  data[2] |= state->l3     << 2;
  data[2] |= state->r3     << 3;

  // Left Stick
  data[3] = state->lx;
  data[4] = state->ly;
 
  // Right Stick
  data[5] = state->rx;
  data[6] = state->ry;
}

void printDescriptionForState(DualShockState *state) {
  Serial.print(state->lx);
  Serial.print(" | ");
  Serial.print(state->ly);
  Serial.print(", ");
  Serial.print(state->rx);
  Serial.print(" | ");
  Serial.print(state->ry);
  Serial.print(", ");
  Serial.print(state->up ? "UP, " : "up, ");
  Serial.print(state->down ? "DOWN, " : "down, ");
  Serial.print(state->left ? "LEFT, " : "left, ");
  Serial.print(state->right ? "RIGHT, " : "right, ");
  Serial.print(state->circle ? "O, " : "o, ");
  Serial.print(state->cross ? "X, " : "x, ");
  Serial.print(state->triangle ? "T, " : "t, ");
  Serial.print(state->square ? "S, " : "s, ");
  Serial.print(state->l1 ? "L1, " : "l1, ");
  Serial.print(state->l2 ? "L2, " : "l2, ");
  Serial.print(state->l3 ? "L3, " : "l3, ");
  Serial.print(state->r1 ? "R1, " : "r1, ");
  Serial.print(state->r2 ? "R2, " : "r2, ");
  Serial.print(state->r3 ? "R3, " : "r3, ");
  Serial.print(state->start ? "START, " : "start, ");
  Serial.print(state->select ? "SELECT" : "select");
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  startSnesGamepad();
  startDualShock();
}

SnesGamepadState snesGamepadState;
SnesGamepadState snesGamepadOldState;

DualShockState dualShockState;
DualShockState dualShockOldState;

void loop() {
  bool shouldSendData = false;
  // Update SNES Gamepad
  updateState(&snesGamepadState);
  if (!isStateIdentical(&snesGamepadState, &snesGamepadOldState)) {
    snesGamepadOldState = snesGamepadState;
    shouldSendData = true;
  }
  // Update DualShock
  updateState(&dualShockState);
  if (!isStateIdentical(&dualShockState, &dualShockOldState)) {
    dualShockOldState = dualShockState;
    shouldSendData = true;
  }

  if (shouldSendData) {
    uint8_t data[10];
    encodeState(&snesGamepadState, data);
    encodeState(&dualShockState, &data[2]);
    for(int i=0; i < 10; i++) {
      Serial.write(data[i]);
    }
    //printDescriptionForState(&dualShockState);
    //printDescriptionForState(&snesGamepadState);
  }
}
