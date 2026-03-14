#include "ESP32Servo.h"

// 모터 핀 설정
const int L_Motor1 = 4;
const int L_Motor2 = 2;
const int R_Motor1 = 27;
const int R_Motor2 = 13;

// IR 센서 핀
const int IR1 = 36;
const int IR2 = 39;
const int IR3 = 34;
const int IR4 = 35;

// 조향 서보 핀
Servo servo;
const int servoPin = 18;
const int servoChannel = 4;
const int servoPwmFreq = 50;
const int servoResolution = 16;

// 초음파 회전용 서보 핀
const int servo2Pin = 16;
const int servo2Channel = 5;

// 초음파 센서 핀
const int trigPin = 32;
const int echoPin = 23;
float distance;

// PWM 설정
const int pwmFreq = 1000;
const int pwmRes = 8;
const int ch_L1 = 0;
const int ch_L2 = 1;
const int ch_R1 = 2;
const int ch_R2 = 3;

// 속도 및 조향 값
const int normalSpeed = 52;
const int parkingSpeed = 50;
const int curveSpeed = 80;
const int centerAngle = 90;
const int steerAmount = 65;
const int fastSpeed = 100;

// 상태 변수
int obstacleCount = 0;
bool obstacleDetected = false;
bool obstacleEnd = false;
bool obstacleInter = false;
bool obstacleInterEnd = false;
bool interEnd = false;
bool backParking = false;
bool backParkingEnd = false;
bool verticalLeft = false;
bool verticalParking = false;
bool tparkingDone = false;
bool curveDone = false;
bool finalStopDone = false;
bool recoveryDriving = false;
bool missionStarted = false;
bool stopDetected = false;
bool parLane = false;
int stopCnt = 0;
unsigned long recoveryStartTime = 0;
const unsigned long ignoreTimeAfterParking = 500;


// 기능 함수들
int angleToDuty(int angle) {
  return map(angle, 0, 180, 3277, 6553);
}

void stopMotors() {
  ledcWrite(ch_L1, 0); ledcWrite(ch_L2, 0);
  ledcWrite(ch_R1, 0); ledcWrite(ch_R2, 0);
}

void moveForward(int speed) {
  ledcWrite(ch_L1, speed); ledcWrite(ch_L2, 0);
  ledcWrite(ch_R1, speed); ledcWrite(ch_R2, 0);
}

void moveBackward(int speed) {
  ledcWrite(ch_L1, 0); ledcWrite(ch_L2, speed);
  ledcWrite(ch_R1, 0); ledcWrite(ch_R2, speed);
}

float measureDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(4);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(20);
  digitalWrite(trigPin, LOW);
  unsigned long duration = pulseIn(echoPin, HIGH);
  return duration * 0.034 / 2.0;
}

void avoidRight() {
  stopMotors(); delay(300);
  ledcWrite(servoChannel, angleToDuty(centerAngle + 40));
  moveForward(curveSpeed); delay(600); // 500 -> 600
  // ledcWrite(servoChannel, angleToDuty(centerAngle));
  // moveForward(normalSpeed); delay(400);
  ledcWrite(servoChannel, angleToDuty(centerAngle - 40));
  moveForward(curveSpeed); delay(100); // 150 -> 100
  obstacleDetected = false;
}

void avoidLeft() {
  stopMotors(); delay(300);
  ledcWrite(servoChannel, angleToDuty(centerAngle - 50));
  moveForward(curveSpeed); delay(440); // 580 -> 400
  ledcWrite(servoChannel, angleToDuty(centerAngle));
  moveForward(normalSpeed); delay(300);
  ledcWrite(servoChannel, angleToDuty(centerAngle + 40));
  moveForward(curveSpeed); delay(150); // 300->150
  stopMotors(); delay(300);
  obstacleDetected = false;
  obstacleEnd = true;
}

void obstacleFinal() {
  stopMotors(); delay(300);
  ledcWrite(servoChannel, angleToDuty(centerAngle));
  moveBackward(normalSpeed); delay(700);
  ledcWrite(servoChannel, angleToDuty(centerAngle - 65));
  moveForward(normalSpeed); delay(850); // 
  obstacleInterEnd = true;
}

void backPark() {
  stopMotors();
  delay(300);

  // 1. 전진하며 여유 공간 확보
  ledcWrite(servoChannel, angleToDuty(centerAngle-10));
  moveForward(parkingSpeed);
  delay(700);

  // 2. 우측 조향 후 후진
  ledcWrite(servoChannel, angleToDuty(140));
  moveBackward(parkingSpeed);
  delay(1550); // 1500

  // 3. 전진하며 여유 공간 확보
  ledcWrite(servoChannel, angleToDuty(centerAngle+25));
  moveForward(parkingSpeed);
  delay(450); // 500

  // 4. 좌측 조향 후 후진
  ledcWrite(servoChannel, angleToDuty(40));
  moveBackward(parkingSpeed);
  delay(1350); //1300->1450

  ledcWrite(servoChannel, angleToDuty(centerAngle));
  moveBackward(parkingSpeed);
  delay(100); // 50

  // 공간 확보를 위한 후진
  // ledcWrite(servoChannel, angleToDuty(centerAngle));
  // moveBackward(curveSpeed);
  // delay(150);

  // 5. 정차 후 대기 (주차 완료)
  stopMotors();
  delay(2000);

  // 6. 좌측 조향하며 전진
  ledcWrite(servoChannel, angleToDuty(40));
  moveForward(parkingSpeed);
  delay(830); // 950 -> 750 -> 830

  // 그냥 전진
  ledcWrite(servoChannel, angleToDuty(90));
  moveForward(parkingSpeed);
  delay(300); // 300

  // 7. 우측 조향하며 전진하여 도로 복귀
  ledcWrite(servoChannel, angleToDuty(140));
  moveForward(parkingSpeed);
  delay(450);

  // // 8. 중앙 정렬 후 직진
  // ledcWrite(servoChannel, angleToDuty(centerAngle));
  // moveForward(curveSpeed);
  // delay(700);

  stopMotors();
}

/*void verticalLeftFunc() {
  stopMotors();
  delay(300);

  // (1) 살짝 후진
  ledcWrite(servoChannel, angleToDuty(centerAngle));
  moveBackward(50);
  delay(500);

  stopMotors();
  delay(100);

  // (2) 빠르게 좌회전
  ledcWrite(servoChannel, angleToDuty(centerAngle - 55));
  moveForward(50);  
  delay(500);

  // (3) 중앙 복귀 후 전진
  // ledcWrite(servoChannel, angleToDuty(centerAngle));
  // moveForward(curveSpeed);
  // delay(600);

  // stopMotors();
  // delay(500); 
}*/

void T_parking() {
  stopMotors();
  delay(300);

  ledcWrite(servoChannel, angleToDuty(centerAngle));
  moveBackward(50);
  delay(50);

  // 좌회전 후 후진
  ledcWrite(servoChannel, angleToDuty(centerAngle + 50));
  moveBackward(50);
  delay(1400); // 
 
  // 후진 정렬
  ledcWrite(servoChannel, angleToDuty(centerAngle));
  moveBackward(50);
  delay(350); // 500 -> 350

  stopMotors();
  delay(1500);
}

void setup() {
  Serial.begin(115200);

  ledcSetup(servoChannel, servoPwmFreq, servoResolution);
  ledcAttachPin(servoPin, servoChannel);
  ledcWrite(servoChannel, angleToDuty(centerAngle));

  ledcSetup(servo2Channel, servoPwmFreq, servoResolution);
  ledcAttachPin(servo2Pin, servo2Channel);
  ledcWrite(servo2Channel, angleToDuty(180));  // 정면

  ledcSetup(ch_L1, pwmFreq, pwmRes);
  ledcSetup(ch_L2, pwmFreq, pwmRes);
  ledcSetup(ch_R1, pwmFreq, pwmRes);
  ledcSetup(ch_R2, pwmFreq, pwmRes);

  ledcAttachPin(L_Motor1, ch_L1);
  ledcAttachPin(L_Motor2, ch_L2);
  ledcAttachPin(R_Motor1, ch_R1);
  ledcAttachPin(R_Motor2, ch_R2);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  stopMotors();
}

void loop() {
  distance = measureDistance();
  int ir1Value = analogRead(IR1);
  int ir2Value = analogRead(IR2);
  int ir3Value = analogRead(IR3);
  int ir4Value = analogRead(IR4);

  int speed = normalSpeed;
  int curAngle = centerAngle;

  stopDetected = (ir2Value < 1000 && ir3Value < 1000);

  if (!missionStarted && stopDetected) {
    delay(400);
    missionStarted = true;
    moveForward(normalSpeed);
  }

  if (missionStarted) {
    if (!obstacleEnd && !obstacleDetected && distance > 0 && distance < 21.0) {
      obstacleDetected = true;

      if (obstacleCount == 0) {
        avoidRight();
      } else if (obstacleCount == 1) {
        avoidLeft();
      } else if (obstacleCount >= 2) {
        obstacleEnd = true;
      }
      obstacleCount++;
    }

    if (!obstacleInter && obstacleEnd && stopDetected) {
      obstacleFinal();
      obstacleInter = true;
      stopDetected = false;
      ledcWrite(servo2Channel, angleToDuty(0));  // 초음파 오른쪽 회전
    }

    if (obstacleInterEnd && stopDetected && stopCnt < 2) {
      stopMotors();
      delay(1000);
      stopCnt++;
      interEnd = true;
      if (stopCnt >= 2) {
          ledcWrite(servoChannel, angleToDuty(centerAngle-10));
          moveForward(60);
          delay(500);
          parLane = true;
      }
    }

    // 평행주차 진입 조건
    if (!backParking && interEnd && distance < 10) {
      parLane = false;
      backParking = true;
      backPark();
      backParkingEnd = true;
    }

    if (backParkingEnd && !tparkingDone && stopDetected) {
      T_parking();
      tparkingDone = true;
    }

    if(tparkingDone && !finalStopDone) {
      if (!recoveryDriving) {
        recoveryDriving = true;
        recoveryStartTime = millis();
      }

      if ((millis() - recoveryStartTime > ignoreTimeAfterParking) && stopDetected) {
        stopMotors();
        delay(10000);
        finalStopDone = true;
      } 
    }
    
    if (parLane && !backParking && interEnd) {
      if (ir4Value < 1800) {
        curAngle = centerAngle + 40;
        speed = normalSpeed;
      }
      else if (ir1Value > 400 && ir1Value < 1800) {
        // 오른쪽 차선 감지 30, 35
        curAngle = centerAngle - 25;
        speed = normalSpeed;
      } 
      else if (ir1Value < 400) {
        // 오른쪽 차선 감지
        curAngle = centerAngle - 30;
        speed = normalSpeed;
      } 
      else { 
        // 평소에는 약간 오른쪽으로 주행
        curAngle = centerAngle + 15;
      }
    }

    else {
      // 일반 주행
      if (ir1Value > 800 && ir4Value < 1200) {
        curAngle = centerAngle + steerAmount;
        speed = normalSpeed;
      } else if (ir4Value > 800 && ir1Value < 1200) {
        curAngle = centerAngle - steerAmount;
        speed = normalSpeed;
      } else {
        curAngle = centerAngle;
        speed = normalSpeed;
      }
    }

    ledcWrite(servoChannel, angleToDuty(curAngle));
    moveForward(speed);
  }

  stopDetected = false;
  delay(50);
}
