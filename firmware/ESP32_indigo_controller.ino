/*
  《青出于蓝》山东蓝印花布草木染交互装置 - 摇杆多视角观察升级固件
  开发板：ESP32-WROOM-32E (38-Pin Type-C版)
  串口输出格式：光敏,电位器,蓝帽键1,黄帽键2,摇杆X,摇杆Y,摇杆按下SW
*/

const int PIN_LIGHT = 34; // 光敏电阻分压 (IO34)
const int PIN_POT   = 35; // 刮浆电位器   (IO35)
const int PIN_BTN1  = 14; // 蓝帽按键1    (IO14)
const int PIN_BTN2  = 27; // 黄帽按键2    (IO27)

// 新增：双轴摇杆引脚
const int PIN_JOY_X = 32; // 摇杆 VRx     (IO32)
const int PIN_JOY_Y = 33; // 摇杆 VRy     (IO33)
const int PIN_JOY_SW= 25; // 摇杆按下 SW  (IO25)

float smoothPot = 0.0;
int outputPot   = 0;

int lastRawBtn1 = HIGH;
int lastRawBtn2 = HIGH;
int lastRawSw   = HIGH;
unsigned long btn1HoldUntil = 0;
unsigned long btn2HoldUntil = 0;
unsigned long swHoldUntil   = 0;

void setup() {
  Serial.begin(115200);

  pinMode(PIN_BTN1, INPUT_PULLUP);
  pinMode(PIN_BTN2, INPUT_PULLUP);
  pinMode(PIN_JOY_SW, INPUT_PULLUP); // 摇杆按键启用内部上拉

  // 模拟口 IO34, IO35, IO32, IO33 不调用 pinMode，直接 analogRead

  delay(200);
  smoothPot = (float)analogRead(PIN_POT);
  outputPot = (int)smoothPot;
}

int readAvgADC(int pin) {
  long sum = 0;
  for (int i = 0; i < 8; i++) {
    sum += analogRead(pin);
    delayMicroseconds(80);
  }
  return (int)(sum / 8);
}

void loop() {
  unsigned long now = millis();

  // 1. 光敏电阻 (IO34)
  int outputLight = readAvgADC(PIN_LIGHT);

  // 2. 电位器 (IO35) 平滑滤波 + 死区防抖
  float rawPot = (float)readAvgADC(PIN_POT);
  smoothPot = smoothPot * 0.75f + rawPot * 0.25f;
  if (abs((int)smoothPot - outputPot) > 35) {
    outputPot = (int)smoothPot;
  }

  // 3. 摇杆双轴读取 (IO32, IO33)，范围 0 ~ 4095，静止居中约 1850~2050
  int joyX = readAvgADC(PIN_JOY_X);
  int joyY = readAvgADC(PIN_JOY_Y);

  // 4. 按键与摇杆下压检测（200ms 信号锁存防漏帧）
  int curBtn1 = digitalRead(PIN_BTN1);
  if (lastRawBtn1 == HIGH && curBtn1 == LOW) btn1HoldUntil = now + 200;
  lastRawBtn1 = curBtn1;

  int curBtn2 = digitalRead(PIN_BTN2);
  if (lastRawBtn2 == HIGH && curBtn2 == LOW) btn2HoldUntil = now + 200;
  lastRawBtn2 = curBtn2;

  int curSw = digitalRead(PIN_JOY_SW);
  if (lastRawSw == HIGH && curSw == LOW) swHoldUntil = now + 200;
  lastRawSw = curSw;

  int sendBtn1 = (now < btn1HoldUntil) ? 1 : 0;
  int sendBtn2 = (now < btn2HoldUntil) ? 1 : 0;
  int sendSw   = (now < swHoldUntil) ? 1 : 0;

  // 5. 发送 7 列数据包
  Serial.print(outputLight); Serial.print(",");
  Serial.print(outputPot);   Serial.print(",");
  Serial.print(sendBtn1);    Serial.print(",");
  Serial.print(sendBtn2);    Serial.print(",");
  Serial.print(joyX);        Serial.print(",");
  Serial.print(joyY);        Serial.print(",");
  Serial.println(sendSw);

  delay(35);
}
