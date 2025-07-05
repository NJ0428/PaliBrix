# PaliBrix

PaliBrix는 Android NDK를 활용한 모바일 게임 프로젝트입니다. C++로 작성된 게임 로직과 OpenGL ES를 사용한 렌더링 시스템을 통해 고성능의 게임플레이를 제공합니다.

## 프로젝트 구조

```
PaliBrix/
├── app/                      # 안드로이드 앱 모듈
│   ├── src/
│   │   ├── main/
│   │   │   ├── cpp/         # Native 게임 로직 (C++)
│   │   │   ├── java/        # Android UI 및 JNI 브릿지
│   │   │   ├── res/         # Android 리소스 파일
│   │   │   └── assets/      # 게임 에셋
│   │   ├── androidTest/     # 안드로이드 계측 테스트
│   │   └── test/            # 단위 테스트
│   └── build.gradle.kts      # 앱 모듈 빌드 설정
└── build.gradle.kts          # 프로젝트 빌드 설정
```

## 기술 스택

- **언어**: Kotlin, C++
- **그래픽스**: OpenGL ES
- **빌드 시스템**: Gradle (Kotlin DSL)
- **네이티브 개발**: Android NDK
- **최소 SDK**: Android 6.0 (API 23)

## 주요 컴포넌트

### Native 코드 (C++)

- `Game.cpp/h`: 게임의 핵심 로직 구현
- `Renderer.cpp/h`: OpenGL ES 기반 렌더링 시스템
- `TextureAsset.cpp/h`: 텍스처 리소스 관리
- `AndroidOut.cpp/h`: Android 로깅 유틸리티
- `JniBridge.cpp`: JNI 인터페이스 구현

### Android (Kotlin)

- `MainActivity.kt`: 게임의 메인 액티비티
- `activity_main.xml`: 게임 UI 레이아웃

## 빌드 및 실행 방법

1. 필수 요구사항:

   - Android Studio
   - Android NDK
   - CMake
   - Android SDK (API 23 이상)

2. 프로젝트 설정:

   ```bash
   # 프로젝트 클론
   git clone https://github.com/NJ0428/PaliBrix.git
   cd PaliBrix
   ```

3. Android Studio에서 실행:
   - Android Studio에서 프로젝트 열기
   - Sync Project with Gradle Files 실행
   - Run 'app' 선택하여 실행

## 라이선스

이 프로젝트는 MIT 라이선스 하에 배포됩니다. 자세한 내용은 LICENSE 파일을 참조하세요.
