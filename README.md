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

## 5. 개발 중 발생한 OS 호환성 문제와 해결 과정

### 문제 1:
 - 로우 모드 설정(enable_raw_mode/disable_raw_mode)에서 OS별 터미널 속성을 제어하는 근본적인 방식이 달라서 OS 호환성 문제가 발생했습니다.
     Windows 환경에서는 콘솔의 인풋 핸들(Input Handle)을 얻어 SetConsoleMode 함수를 통해 입력 모드의 플래그(Flag)를 직접 제어해야 하는 반면, 
     Linux/macOS (POSIX) 환경에서는 터미널 제어(Terminal Control) 라이브러리인 <termios.h>의 tcsetattr 함수를 사용하여 터미널 속성 구조체(struct termios) 내의 로컬 플래그를 조작해야 했습니다.

### 문제 2:
 - main함수와 관련하여 사용자 키 입력 부분에서 방향 키와 같은 특수 키 입력을 처리할 때, 
    OS별 다른 환경에서 해당 키들이 근본적으로 다른 바이트 시퀀스(키 인코딩)로 전달되어 입력 처리 로직의 OS 호환성 문제가 발생했습니다.
    Windows는 \xe0로 시작하는 하드웨어 기반의 숫자 키 코드(72, 80 등)를 사용하여 같은 특수 키를 식별하는 반면, 
    Linux/macOS는 \x1b로 시작하는 ANSI 이스케이프 시퀀스를 통해 'A', 'B' 등의 문자 명령어로 특수 키를 전달합니다. 
    이처럼 근본적으로 다른 키 인코딩 방식을 #ifdef로 분기하여 각 OS에 맞는 시퀀스 파싱 로직을 구현했으며, 
    최종적으로 내부 게임 로직에서 사용하는 공통 문자('w', 's', 'd', 'a')로 변환하여 높은 수준의 이식성을 확보했습니다.

### 문제 3:
 - sound_beep_maker(hz, ms) 함수를 구현할 때, Windows의 표준 API를 제외한 POSIX(Linux/macOS) 환경에서는 
    주파수와 지속 시간을 제어할 수 있는 표준 경고음 발생 API가 부재하여 OS 호환성 문제가 발생했습니다.
    Windows 환경에서는 표준 Beep(hz, ms) 함수를 #ifdef _WIN32 전처리기를 통해 직접 호출하여 손쉽게 구현할 수 있었습니다.
    반면, Linux/macOS (POSIX) 환경에서는 직접적인 Beep 함수가 표준으로 제공되지 않아, 
    Linux의 beep 유틸리티 사용을 고려했지만 macOS와의 비호환성 및 개별 설치 필요성 때문에 높은 이식성(Portability) 확보가 어려웠습니다. 최종적으로, 주파수(hz)와 지속 시간(ms) 제어는 포기하고, 표준 출력(stdout)으로 알림 문자(\a)를 출력하는 방식으로 대체했습니다. 
    이 방식은 터미널의 기본 경고음 기능을 활용하여 모든 POSIX 시스템에서 가장 높은 호환성을 보장하도록 printf("\a");와 fflush(stdout);를 사용하여 구현되었습니다.