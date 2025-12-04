# Nuguri game

## 1. 10조
- 팀원:
    - 20223087 김대영 (조장)
    - 20223149 황대겸 (조원)
    - 20243123 이지현 (조원)
    - 20253165 정철진 (조원)

## 2. GitHub 주소
https://github.com/Gongdang0314/nuguri

## 3. 컴파일 및 실행 방법

### Windows
- 컴파일
    gcc -o nuguri.exe nuguri.c
- 실행
    nuguri.exe

### Linux/macOS
- 컴파일
    gcc -o nuguri nuguri.c
- 실행
    ./nuguri

## 4. 구현 기능

### 필수 기능
- 크로스 플랫폼 지원
- 생명력 시스템
- 타이틀 화면 및 엔딩 화면
- 기존 버그 수정

### 추가 기능
- 사운드 효과
- 동적 맵 할당

### 게임 스크린샷
| **오프닝** | **게임 클리어** |
| :---: | :---: |
| <img width="1723" height="914" alt="image" src="https://github.com/user-attachments/assets/7d770270-4c63-4e65-9620-1f8a85de6f16" /> | <img width="1722" height="919" alt="image" src="https://github.com/user-attachments/assets/a19b0238-1009-4785-87e3-73faeee139e9" /> |
| **스테이지 1** | **스테이지 2** |
| <img width="1729" height="914" alt="image" src="https://github.com/user-attachments/assets/77b0c9ac-7f80-414b-8705-e51b6a295f6a" /> | <img width="1727" height="919" alt="image" src="https://github.com/user-attachments/assets/ab269295-b543-4e50-a149-d4ccbb7d1fa3" /> |

## 5. 개발 중 발생한 OS 호환성 문제와 해결 과정

### 문제 1: 터미널 로우 모드 설정 차이
 - 게임 조작을 위해 엔터 키 입력 없이 즉시 입력을 받는 raw_mode 구현 방식이 os마다 근본적으로 달라 os 호환성 문제가 발생했습니다.
 - 해결: 
  - Linux/macOS (POSIX) 환경에서는 터미널 제어(Terminal Control) 라이브러리인 <termios.h>의 tcsetattr 함수를 사용하여 터미널 속성 구조체(struct termios) 내의 로컬 플래그를 조작하였습니다.
  - Windows 환경에서는 복잡한 콘솔 핸들 설정을 사용하는 대신 <conio.h> 의 _getch() 함수 자체가 별도의 설정 없이도 raw_mode처럼 동작한다는 점을 활용하여 간단하게 호환성 문제를 해결했습니다.

### 문제 2:
 - main함수와 관련하여 사용자 키 입력 부분에서 방향 키와 같은 특수 키 입력을 처리할 때, 
    OS별 다른 환경에서 해당 키들이 근본적으로 다른 바이트 시퀀스(키 인코딩)로 전달되어 입력 처리 로직의 OS 호환성 문제가 발생했습니다.
 - 해결:
  - Windows는 \xe0로 시작하는 하드웨어 기반의 숫자 키 코드(72, 80 등)를 사용하여 같은 특수 키를 식별하는 반면, 
    Linux/macOS는 \x1b로 시작하는 ANSI 이스케이프 시퀀스를 통해 'A', 'B' 등의 문자 명령어로 특수 키를 전달합니다. 
    이처럼 근본적으로 다른 키 인코딩 방식을 #ifdef로 분기하여 각 OS에 맞는 시퀀스 파싱 로직을 구현했으며, 
    최종적으로 내부 게임 로직에서 사용하는 공통 문자('w', 's', 'd', 'a')로 변환하여 높은 수준의 이식성을 확보했습니다.

### 문제 3:
 - sound_beep_maker(hz, ms) 함수를 구현할 때, Windows의 표준 API를 제외한 POSIX(Linux/macOS) 환경에서는 
    주파수와 지속 시간을 제어할 수 있는 표준 경고음 발생 API가 부재하여 OS 호환성 문제가 발생했습니다.
 - 해결: 
  - Windows 환경에서는 표준 Beep(hz, ms) 함수를 #ifdef _WIN32 전처리기를 통해 직접 호출하여 손쉽게 구현할 수 있었습니다.
    반면, Linux/macOS (POSIX) 환경에서는 직접적인 Beep 함수가 표준으로 제공되지 않아, 
    Linux의 beep 유틸리티 사용을 고려했지만 macOS와의 비호환성 및 개별 설치 필요성 때문에 높은 이식성(Portability) 확보가 어려웠습니다. 최종적으로, 주파수(hz)와 지속 시간(ms) 제어는 포기하고, 표준 출력(stdout)으로 알림 문자(\a)를 출력하는 방식으로 대체했습니다. 
    이 방식은 터미널의 기본 경고음 기능을 활용하여 모든 POSIX 시스템에서 가장 높은 호환성을 보장하도록 printf("\a");와 fflush(stdout);를 사용하여 구현되었습니다.

### 문제 4:
 - Windows의 기본 콘솔 인코딩(CP949) 환경에서 특수문자나 한글이 깨지는 현상이 발생했습니다.
 - 해결:
  - Windows 환경일 경우 SetConsoleOutputCP(CP_UTF8) 함수를 호출하여 콘솔 출력 인코딩을 UTF-8로 강제 설정함으로써 os 간 출력 결과가 동일하도록 맞추었습니다.
  - Linux/macos는 터미널 환경의 기본 설정이 이미 UTF-8을 지원하도록 표준화되어 있기 때문에 따로 설정하지 않아도 정상적으로 출력됩니다.

### 문제 5:
 - 1. Delay 함수에서 Linux/macos는 마이크로초 단위를 사용하는 반면 윈도우의 Sleep() 함수는 밀리초를 사용하여 기준이 달랐습니다.
 - 2. 윈도우의 기본 함수인 Sleep()은 약간의 오차가 발생합니다. 이런 오차는 게임처럼 섬세한 딜레이가 필요한 상황에서는 화면이 끊기거나 느려져보이는 원인이 됩니다.
 - 해결:
  - 통일된 함수 제작: 어떤 OS에서든 delay(ms) 형태로 밀리초만 입력하면 알아서 작동하도록 래퍼 함수를 만들었습니다.
  - Windows (고정밀 타이머 도입): 오차가 큰 기본 Sleep 함수 대신 훨씬 정밀한 CreateWaitableTimer를 도입하여 부드러운 프레임 처리를 구현했습니다.
  - Linux/macOS: 정밀도가 이미 충분히 높은 usleep을 사용하되 입력받은 밀리초에 1000을 곱해 마이크로초로 단위를 맞춰주었습니다.
