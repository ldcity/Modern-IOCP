# 채널 없는 심리스 오픈월드 서버 — 학습·개발 로드맵

> 통합본 v2 · 2026-08-28
> Codex 초안(단계 구조·분산 엄밀성)과 Claude 초안(시간 산술·언어 배치·병렬 안전 논증)을 통합.
> ⚠ 초판에 "Codex 원본은 `SEAMLESS_OPEN_WORLD_ROADMAP.codex-original.md` 로 보존" 이라 적었으나
> **그 파일은 생성되지 않았다**(2026-08-28 확인). Codex 초안 원문은 남아 있지 않다.

---

## 1. 이 문서의 목적

로딩도 채널도 없이 이어진 오픈월드 서버를 2년에 걸쳐 만든다. 그리고 **그걸 내가 만들었다는
사실이 저장소에 남게** 한다.

두 번째가 첫 번째만큼 중요하다. 완성품 하나가 뜬금없이 올라온 저장소는, 내용이 아무리 좋아도
"AI에게 시키고 본인은 고민하지 않은 산출물"로 읽힌다. 그러므로 이 로드맵은 **무엇을 만드나**와
**만들었다는 걸 어떻게 증명하나**를 같은 비중으로 설계한다.

증명의 재료는 커밋 개수가 아니라 **질문 · 실패 · 측정 · 결정의 흐름**이다.

### 전제

| 항목 | 값 |
|---|---|
| 시간 예산 | 주 5–10시간 (평균 7.5h → **월 약 32시간**) |
| 총 기간 | **약 24개월** |
| 시뮬레이션 코어 | C++ |
| 게이트웨이 · 부하봇 | Rust |
| 대시보드 · 툴 | Go |
| 클라이언트 | UE5 (C++) |
| 출발 | 새 저장소, 빈 상태 |
| 환경 | 집 PC에서 작업·push |

### 출발점

3년 전 학생 때 IOCP로 만든 로그인·채팅 서버가 마지막 공개 이력이다. 저수준 소켓과 스레드는
안다. 모르는 것은 **authority · 시간 · 공간 · 분산**이다. 이 로드맵은 정확히 그 네 가지를
순서대로 쌓는다.

---

## 2. 최종 목표와 완료 정의

"심리스 오픈월드 서버를 만들었다"는 다음이 전부 참일 때만 성립한다.

- 플레이어가 서버 노드 경계를 **걸어서** 넘을 때 로딩·끊김·재접속이 없다.
- 경계 너머의 플레이어가 보이고, 서로 상호작용할 수 있다.
- 메시지가 유실·중복·역순으로 도착해도 상태가 깨지지 않는다.
- 한쪽 노드가 죽어도 authority가 중복되거나 사라지지 않는다.
- 인구가 한 지역에 몰렸을 때 무엇이 병목인지 **수치로** 말할 수 있다.
- 위 전부를 재현 가능한 테스트와 벤치마크로 증명할 수 있다.

---

## 3. 시간 산술 — 먼저 정직할 것

로드맵이 실패하는 가장 흔한 이유는 기술이 아니라 산수다.

> 주 7.5시간 × 4.3주 ≈ **월 32시간**. 단계당 60–100시간이면 **단계당 2–3개월**.
> 14개 단계 → **약 24개월**.

주 7.5시간은 하루 한 시간 남짓이다. **한 단계에 두 달이 걸리는 건 뒤처진 게 아니라 정상이다.**
이 문서는 그 속도를 전제로 쓰였다.

기간보다 **완료 조건**을 우선한다. 늦어지는 것은 괜찮고, 완료 조건을 건너뛰는 것은 안 된다.

---

## 4. 개발 원칙 여섯

### 4.1 정확성을 먼저 확보한다

각 기능은 이 순서로 개발한다.

1. 가장 단순한 모델로 정확성을 확보한다
2. 자동 테스트와 계측을 추가한다
3. 병목을 측정한다
4. **한 가지 문제만** 개선한다
5. 이전 구현과 성능·복잡도를 비교한다
6. 선택 이유와 한계를 ADR로 기록한다

처음부터 lock-free · ECS · coroutine · 분산 처리 · 동적 partition을 모두 도입하지 않는다.

### 4.2 "최적"을 수치로 정의한다

아키텍처의 우열은 느낌이 아니라 아래 지표로 판단한다.

- Simulation tick rate · P50/P95/P99 tick latency
- 최대 동시 접속자 수 · World 내 최대 Actor 수 · Hotspot 내 최대 플레이어 수
- AOI당 평균 객체 수 · 플레이어당 송수신 대역폭
- Handoff P50/P95/P99 latency
- 서버 장애 후 복구 시간 · 허용 가능한 상태 유실 범위
- CPU · 메모리 · 네트워크 사용량

초기 목표치는 **가설로** 기록한다. 측정 결과에 따라 목표나 구조를 바꾸게 되면 기존 문서를
덮어쓰지 않고 **새 ADR로 변경 이유를 남긴다.**

### 4.3 느리게 만들고, 재고, 고친다

베이스라인 없는 최적화는 개선을 증명하지 못한다. "1만 엔티티에서 12ms였던 게 3ms가 됐다"가
있어야 한 줄이 된다. 최적화하고 싶은 충동은 계측이 붙은 뒤로 미룬다.

### 4.4 각 단계는 독립적인 실행 제품이다

단계 완료 = 아래 전부 참.

- [ ] 독립적으로 빌드된다
- [ ] 자동 테스트를 실행할 수 있다
- [ ] 데모 시나리오를 재현할 수 있다
- [ ] 성능 결과를 같은 환경에서 다시 측정할 수 있다
- [ ] 구현 범위와 **제외 범위**가 문서화되어 있다
- [ ] 알려진 한계와 다음 단계가 기록되어 있다
- [ ] Git release tag가 존재한다

### 4.5 설명하지 못하면 완료가 아니다

각 단계 종료 시 **"이 구조를 화이트보드에 그려 10분간 설명할 수 있는가."**
코드가 돌아도 설명 못 하면 미완으로 친다. AI 시대에 실력을 지키는 유일한 방법이다.

### 4.6 막히면 범위를 줄이되 순서는 바꾸지 않는다

- 한 단계가 예상의 **2배를 넘으면 범위를 줄이고 다음으로.** 중단보다 훨씬 낫다.
- 단, **4.1–4.5의 규율만은 절대 건너뛰지 않는다.** 그게 이 프로젝트의 값이다.
- 3주 이상 손을 못 댔으면 devlog에 "이번 달은 못 했다"고 쓴다. **공백도 정직한 기록이다.**
- 순서에는 이유가 있다. 막혔다고 단계를 건너뛰지 말고 그 단계의 범위를 좁힌다.

---

## 5. 언어 배치와 그 근거

여러 언어를 쓴 게 아니라 **각 언어가 정당한 자리에 있어야** 한다. 면접에서 갈리는 지점이다.

| 언어 | 자리 | 근거 |
|---|---|---|
| **C++** | 시뮬레이션 코어 | 틱 예산이 지배하는 곳. 캐시 레이아웃과 무락 병렬이 성능을 정한다 |
| **Rust** | 게이트웨이 · 부하봇 | IO 바운드이고 프로세스가 분리돼 언어 경계가 깨끗하다 |
| **Go** | 운영 대시보드 · 툴 | 개발 속도가 가치인 자리 |
| **UE5 C++** | 클라이언트 | 서버 권위 검증과 최종 데모 |

**게이트웨이를 Rust로 두는 것은 취향이 아니라 아키텍처다.** P10에서 플레이어가 노드 A에서 B로
넘어갈 때, 클라가 시뮬 노드에 직결돼 있으면 재접속이 일어나고 그 순간 끊김이 보인다. 접점이
게이트웨이로 고정돼 있어야 뒤에서 노드가 바뀌어도 클라는 모른다. **게이트웨이는 심리스의
필수 부품**이고, 그래서 "Rust 써보려고 끼워넣은 것"이 되지 않는다.

---

## 6. 전체 일정

누적 개월은 예산대로 진행했을 때 그 단계가 **끝나는** 시점이다.

| # | 단계 | 기간 | 누적 | 언어 | Release |
|---|---|---:|---:|---|---|
| P0 | 개발 기반 | 2–3주 | 0.7 | C++ | `v0.0-project-foundation` |
| P1 | 네트워크 기반 | 3–4주 | 1.5 | C++ | `v0.1-network-foundation` |
| P2 | 단일 스레드 Authoritative World | 4–6주 | 2.7 | C++ | `v0.2-single-thread-world` |
| P3 | 공간 색인과 AOI | 6–8주 | 4.3 | C++ | `v0.3-spatial-aoi` |
| P4 | Fixed Tick · 입력 순서 · Replay | 3–5주 | 5.2 | C++ | `v0.4-fixed-tick-replay` |
| P5 | Actor · Component · Mailbox | 4–6주 | 6.4 | C++ | `v0.5-actor-component-mailbox` |
| P6 | 멀티코어 공간 병렬화 | 8–10주 | 8.4 | C++ · **Rust** | `v0.6-section-parallel` |
| **P7** | **UE5 클라이언트** | 8–10주 | **10.5** | UE5 C++ | `v0.7-ue5-client` |
| P8 | 멀티 프로세스 기반 | 4–6주 | 11.7 | C++ · **Rust** | `v0.8-multi-process-foundation` |
| P9 | District 경계 Ghost | 6–8주 | 13.3 | C++ | `v0.9-border-ghost` |
| **P10** | **Authority Handoff** | 8–10주 | **15.4** | C++ · Rust | `v1.0-authority-handoff` |
| P11 | 장애 복구 | 6–8주 | 17.0 | C++ | `v1.1-fault-tolerant-handoff` |
| P12 | Hotspot과 동적 부하 분산 | 8–12주 | 19.3 | C++ · **Go** | `v1.2-dynamic-partitioning` |
| P13 | Persistence · 전역 서비스 · 최종 검증 | 6–8주 | 20.9 | 전체 | `v1.3-seamless-open-world-demo` |

여유 3개월을 더해 **약 24개월**. 굵게 표시한 셋이 보여줄 것이 생기는 지점이다.

### 권장 학습 순서 (왜 이 순서인가)

```
네트워크 안정성 → 단일 스레드 authoritative → 시간과 입력 순서 → 객체 수명
→ 공간 색인 → AOI → 멀티코어 → 클라이언트 → 멀티 프로세스
→ Ghost → Handoff → 장애 복구 → 동적 partition
```

**멀티코어보다 authority를 먼저** 이해하고, **handoff보다 ghost를 먼저** 구현하며,
**동적 partition보다 고정 District를 먼저** 완성한다.

---

## 7. 단계 상세

### P0 — 개발 기반 (2–3주 · ~20h)

앞으로 2년간 쓸 바닥을 깐다.

**구현 범위**
- 저장소 생성, README에 **목표와 비목표** 각 한 문단
- CMake 빌드, GitHub Actions CI (빌드 + 테스트)
- 테스트 프레임워크 (GoogleTest 또는 Catch2)
- **벤치 하네스** — 수치를 CSV로 떨구고 그래프까지 그리는 스크립트
- `docs/adr/`, `docs/devlog/`, `docs/milestone/` 디렉토리와 템플릿
- 고정틱 루프만 도는 빈 서버

**완료 조건**
- 클론 → 빌드 → 실행하면 30Hz로 돌고 3초마다 틱 통계를 찍는다
- CI 초록 · devlog 1편 · `ADR-0001 Authoritative Server`

> 🔴 **함정** — 여기서 완벽주의에 빠지면 프로젝트가 죽는다. 인프라는 나중에 고칠 수 있다.
> **3주 안에 끝내고 넘어간다.**

---

### P1 — 네트워크 기반 (3–4주 · ~30h)

**구현 범위**
- TCP accept와 Session 수명
- Packet framing (길이 접두 + 부분 수신 조립)
- Bounded send queue · slow client 정책
- Heartbeat · disconnect 감지
- 테스트 클라이언트

**반드시 다룰 문제**
- 분할 수신 / 결합 수신
- Send queue 포화 시 정책 (드롭 vs 연결 종료)
- Half-open 연결
- Disconnect 시점의 객체 수명

**의도적으로 제외** — 암호화, 압축, RUDP, 재접속 세션 복원. 나중에 ADR과 함께 도입한다.

**완료 조건**
- Fragmented packet 테스트 통과
- 동시 접속 N에서 대역폭·지연 측정치
- Slow client가 서버 전체를 지연시키지 않는다

**ADR** — 3년 전 IOCP 코드와 지금 asio 코드의 비교. *무엇이 달라졌고 왜.*
남의 기술이 아니라 **본인 성장의 직접 증거**라 포트폴리오에서 가장 설득력 있는 한 장이 된다.

---

### P2 — 단일 스레드 Authoritative World (4–6주 · ~45h)

**핵심 불변식** — 이 단계에서 박아두고 끝까지 유지한다.
- 클라이언트는 **요청만** 한다. 상태는 서버가 정한다.
- Entity 상태 변경은 World 스레드에서만 일어난다.
- 파괴는 즉시가 아니라 **지연 파괴**(deferred destruction)로 프레임 경계에서 처리한다.

**구현 범위**
- Entity ID 발급과 수명
- Player spawn / despawn / disconnect cleanup
- Input command queue
- Authoritative movement (좌표·속도·상태 검증)
- Monster와 HP, 단순 공격

**완료 조건**
- 비정상 입력(순간이동, 속도 초과, 범위 밖) 테스트가 전부 거부된다
- Disconnect 직후 남는 참조가 없다
- 봇 이동으로 1천 Entity가 안정적으로 돈다

---

### P3 — 공간 색인과 AOI (6–8주 · ~60h)

> 심리스의 1단계. 1000명이 접속해도 각자는 주변 50명만 받는다.

**공간 색인** — 첫 구현은 uniform grid. 대안(spatial hash · quadtree · loose quadtree ·
BVH · octree · sweep and prune)은 구현 또는 비교 문서로 학습한다.

**AOI를 3단계로 진화시킨다** — 한 번에 최종형을 만들지 않는다. 이 진화 자체가 git history다.

| 차수 | 방식 | 요지 |
|---|---|---|
| 1차 | **Full snapshot** | 주변 셀 조회 → 보이는 Entity 전체를 매번 전송. 비효율적이지만 검증이 쉽다 |
| 2차 | **Delta replication** | 이전 visible set과 차집합 → `Enter` / `Leave` / `Update` |
| 3차 | **Cell subscription** | 뷰어가 셀을 구독, 피관찰자가 그 셀 구독자의 송신 버퍼에 발행 |

**비교할 지표** — 세 방식 각각에 대해 CPU · 메모리 할당 · 대역폭 · 구현 복잡도를 측정한다.

**AOI는 양방향으로 나눈다.** "내가 보는 것"(뷰어)과 "누가 나를 보는가"(피관찰자)는 같은 사건의
양면이라, 방향별 enter/leave 셀 오프셋 테이블 하나를 양쪽이 공유할 수 있다.

**테스트**
- 직선 · 대각선 셀 이동 / AOI 경계 왕복 / 순간이동
- Spawn · Despawn / 중복 Enter / 누락 Leave
- Update가 Enter보다 먼저 도착하는 경우

**완료 조건**
- **대역폭 before/after 그래프** — 전체 브로드캐스트 O(N²) vs AOI. 1000명 밀집 시 클라당 초당 바이트
- AOI 전이 테스트 자동화
- 세 방식 중 선택 근거 ADR
- AOI 질의와 전투 공간 질의의 책임 분리

> 🔴 **함정** — 뷰어(시각화) 없이 AOI를 디버깅하려는 것. **여기서 경량 2D 디버그 뷰어를
> 만든다** (SDL2 + ImGui 권장). 버리는 물건이 아니다 — 앞으로 셀 격자 · AOI 범위 · 섹션 경계 ·
> 이주 순간을 보여줄 도구이고, 그 용도로는 UE5보다 이쪽이 낫다. 끝까지 쓴다.
>
> 경계에서 왔다갔다 하는 엔티티를 여기서 처음 만난다. 기록만 해두고 P6/P12에서 제대로 푼다.

---

### P4 — Fixed Tick · 입력 순서 · Replay (3–5주 · ~35h)

> 분산 디버깅의 근간. 여기를 건너뛰면 P9 이후의 버그를 영원히 못 잡는다.

**구분해야 할 시간 다섯**
- Wall-clock time · Simulation tick · Network receive time · Client input time · Persistence time

게임 로직이 wall-clock을 **직접 읽지 않도록** simulation clock을 주입한다.

**구현 범위**
- Fixed simulation tick + time accumulator + 최대 catch-up 횟수
- Input sequence · server tick 번호 · snapshot tick 번호
- Random seed 관리
- Replay 입력 기록 · 핵심 state checksum

**Replay 모델**

```
초기 상태 + Tick별 Input Commands + Random Seed
    → Replay → State Checksum 비교
```

처음부터 모든 부동소수점의 bitwise determinism을 목표로 하지 않아도 된다. **이동과 전투 같은
핵심 상태부터** 재현한다.

**완료 조건**
- 같은 초기 상태와 입력에서 핵심 checksum이 일치한다
- Duplicate · stale · out-of-order input 테스트가 있다
- 긴 프레임과 catch-up 정책이 문서화되어 있다
- ★ **실제 버그 하나를 replay로 재현한다** ← 이게 진짜 완료 조건

---

### P5 — Actor · Component · Mailbox (4–6주 · ~45h)

**비교할 설계** — 상속 계층 / 컴포넌트 조합 / ECS. 셋의 장단을 실제로 겪고 ADR로 남긴다.

**핵심 원칙**
- 객체는 자기 상태만 직접 변경한다
- 남의 상태 변경은 **요청**으로 보낸다
- 프레임 중 구조 변경(생성·파괴·소속 이동)은 정의된 phase에서만

**권장 객체 상태** — Spawning / Active / Frozen / Migrating / Destroying.
Frozen과 Migrating은 P10에서 쓰인다. **미리 자리를 만들어 둔다.**

---

### P6 — 멀티코어 공간 병렬화 (8–10주 · ~75h) ★

> 코어를 늘리면 실제로 빨라지고, **그게 안전하다는 것을 증명한다.**

세 방식을 **실제로 구현해 비교한다.** 하나를 골라 시작하지 않는다 — 비교가 산출물이다.

| 실험 | 방식 | 장점 | 단점 |
|---|---|---|---|
| **A** | Section mailbox — 각 Section이 자기 Entity만 변경, cross-section은 메시지 | Ownership 명확 | 경계 지연과 메시지 비용 |
| **B** | Barrier phase — Input→Movement→⊥→Index→⊥→Combat→⊥→Replication | 검증하기 쉬움 | Barrier 비용, 느린 worker가 전체 지연 |
| **C** | Conflict graph / Wave DAG — 가까운 Section은 순서화, 먼 것만 병렬 | 낮은 lock 비용 | **안전성 증명이 어렵다** |

#### 실험 C를 고른다면 — 안전 논증

기하 규정 방식의 핵심은 하나다.

> 🔴 **위험한 쌍은 "서로 만지는 둘"이 아니라 "같은 것을 만지는 둘"이다.**

```
섹션 1        섹션 2        섹션 3
  A ─────────▶  T  ◀───────── B
       R              R
```

A와 B는 서로를 못 본다. 그런데 둘 다 T의 HP와 송신 버퍼에 쓴다. 이웃(gap 1)만 직렬화하면
A와 B는 gap 2라 **동시에 돌고**, 조용한 데이터 레이스가 된다.

같은 대상에 쓰는 두 작성자는 그 대상을 사이에 두고 최대 2R = **섹션 두 칸**까지 벌어진다.
따라서 규정은 **gap ≤ 2 직렬화**여야 하고, 그러면 안전 조건이 한 문장으로 압축된다.

> **남의 상태를 만지는 도달거리가 섹션 한 변 이하면, 락 없이 안전하다.**

실현은 mod 3 색칠이다. x, y를 각각 3으로 나눈 나머지로 wave를 배정하면 같은 wave는 좌표 차가
3의 배수 = gap ≥ 3이라 마음껏 동시 실행할 수 있고, 다른 wave는 오름차순으로만 의존시켜
사이클이 생기지 않는다. 그리고 **부팅 시 gap≤2인 모든 쌍이 실제로 순서화됐는지 전수 검증**한다 —
그래야 "락 없이 안전하다"가 주장이 아니라 **부팅 로그**가 된다.

#### 필수 불변식

- Entity authority는 Section 하나에만 존재한다
- Entity는 한 simulation frame에 **최대 한 번** tick한다
- 구조 변경은 정의된 frame phase에서만 수행한다
- Cross-section write 규칙을 우회할 수 없다
- **상호작용 거리와 scheduler의 안전 범위가 일치한다**

#### 계측

Worker utilization · Section별 Actor 수와 처리 시간 · Task timeline ·
P50/P95/P99 tick latency · False sharing · Cross-section 메시지 수 · Hotspot의 critical path

#### 완료 조건

- 단일 스레드 기준 구현과 결과를 비교한다
- 코어 1/2/4/8 **스케일링 곡선** (선형 대비 %)
- **ThreadSanitizer 클린**
- 이동 중 중복 · 누락 tick이 없다
- 균등 분포와 hotspot 부하를 **별도로** 측정한다
- **병렬화가 이득을 내기 시작하는 Actor 수**를 기록한다

> 🔴 **함정** — TSan 없이 "잘 되는데?"라고 믿기. 데이터 레이스는 크래시가 아니라 **조용한 오염**이다.
> 그리고 실부하 없는 스케일링 수치는 무의미하다. **Rust 부하봇(tokio)을 이 단계에서 함께 만든다.**

---

### P7 — UE5 클라이언트 (8–10주 · ~75h)

> 남에게 보여줄 수 있는 것. 그리고 P9–P10을 눈으로 검증할 수단.

멀티 프로세스 **전에** 붙인다. 그래야 Ghost와 Handoff를 만들 때 이미 시각 확인 수단이 있고,
최종 데모 영상을 P10에서 바로 뽑을 수 있다.

**구현 범위** — UE5 프로젝트, 서버 연결, 서버 권위 이동 + 클라 보간, 다른 플레이어 렌더

**완료 조건** — 걸어다니고 서로 보이는 1–2분 영상

**ADR** — 보간 방식 · 클라 예측을 넣을지. **이 단계에선 안 넣는 걸 권장**. P10이 더 급하다.

> 🔴 **함정** — **엔진에 빠져 서버를 놓는 것.** 타임박스를 엄격히 건다. 10주를 넘기면 되는
> 데까지만 하고 잘라낸다. 여기는 보여주기 마일스톤이지 목적지가 아니다.

---

### P8 — 멀티 프로세스 기반 (4–6주 · ~45h)

아직 handoff를 구현하지 않고 **역할부터 분리**한다.

```
Client
  │
  ▼
Gateway (Rust)
  ├─ World Node A
  └─ World Node B

Coordinator
```

| 역할 | 책임 |
|---|---|
| **Gateway** | 클라 연결 유지, 인증, 노드 라우팅, 세션 상태. **클라 접점을 고정한다** |
| **World Node** | District 하나를 authoritative하게 시뮬레이션 |
| **Coordinator** | District ↔ Node 배치표, 노드 생존 감시, 분쟁 판정 |

**내부 메시지 공통 정보** — 모든 노드 간 메시지에 다음을 싣는다. 나중에 전부 필요해진다.
`MessageId · TransferId(해당 시) · SourceNode · TargetNode · AuthorityEpoch · SimulationTick · Sequence`

**완료 조건**
- 클라가 어느 노드에 붙었는지 모른다
- 시뮬 노드를 재시작해도 클라 연결이 유지된다(또는 자동 복구된다)
- 노드 간 메시지 지연·손실을 계측한다

> 🔴 **함정** — C++ ↔ Rust 프로토콜 이중 관리. 스키마를 **한 곳에서 생성**하지 않으면 양쪽이
> 조용히 어긋난다.

---

### P9 — District 경계 Ghost (6–8주 · ~60h)

서로 다른 노드의 경계를 연결하되 **authority는 아직 이전하지 않는다.**

```
Node A                              Node B
Authoritative Actor A ─snapshot──▶  Ghost A
Ghost B              ◀─snapshot──   Authoritative Actor B
```

**Ghost 규칙**
- Read-only다
- 이동이나 HP를 **최종 확정하지 않는다**
- DB 저장 주체가 **아니다**
- 로컬 AOI나 판정 후보로는 사용할 수 있다
- 변경 요청은 authority node에 전달한다

**Ghost 상태 필드**
`EntityId · AuthorityNodeId · AuthorityEpoch · SnapshotSequence · SimulationTick ·
Transform · Velocity · ReplicationState · LastReceivedTime`

**구현 순서**
1. District 경계를 고정한다
2. 경계 주변 Actor snapshot을 전송한다
3. 상대 노드에서 Ghost를 생성한다
4. Ghost 상태를 갱신한다
5. 경계에서 멀어지면 Ghost를 제거한다
6. 클라 AOI에 local Actor와 Ghost를 함께 노출한다
7. 중복 · 역순 snapshot을 폐기한다

**테스트**
- Snapshot reorder / duplicate / 일부 유실
- Create보다 Update가 먼저 도착
- Destroy보다 늦은 Update 도착
- 노드 연결 단절과 stale Ghost
- Actor의 빠른 경계 왕복

**완료 조건**
- 경계 양쪽 Actor가 서로 보인다
- Ghost가 authoritative state를 변경할 수 **없다**
- Stale Ghost timeout이 동작한다
- Sequence와 epoch로 오래된 메시지를 폐기한다
- Ghost 대역폭과 갱신 지연을 측정한다

---

### P10 — Authority Handoff (8–10주 · ~80h) ★ 심리스의 핵심

#### 상태 머신

```
Source Node                           Target Node

Owned
  │
  ├── PrepareTransfer ───────────────▶ Preparing
  │                                      │
  │◀──────────── Ready ──────────────────┤
  │
Frozen
  │
  ├── Commit(epoch+1, state) ─────────▶ Owned
  │                                      │
Ghost ◀──────── CommitAck ───────────────┤
```

#### 전송에 필요한 데이터

Entity ID · **Transfer ID** · **Authority epoch** · Source/Target District와 Node ·
기준 simulation tick · Full Actor snapshot · **마지막 처리 input sequence** · **미처리 입력** ·
소유·결합된 하위 객체 · Transfer timeout

#### 핵심 불변식 넷

| 불변식 | 내용 |
|---|---|
| **Single authority** | 정상 상태에서 Entity의 authoritative owner는 **정확히 하나** |
| **Idempotency** | 같은 Transfer ID 메시지를 여러 번 처리해도 결과가 같다 |
| **Fencing** | 더 낮은 authority epoch를 가진 노드는 상태를 확정하거나 배포하지 **못한다** |
| **Input preservation** | Handoff 전후에 입력이 중복 적용되거나 유실되지 않는다 |

#### 최초 버전의 제한 — 스코프를 좁히는 장치

첫 handoff는 **단독 플레이어 이동만** 지원한다. 아래는 "handoff 불가"로 시작해도 된다.

거래 중 · Inventory transaction 중 · 탈것 탑승 · 소환수 보유 · 투사체 발사 중 ·
전투 중 · 파티 단위 이동

이걸 명시적으로 문서화하는 것 자체가 설계 능력의 증거다.

#### 실패 정책

| 실패 시점 | 초기 정책 |
|---|---|
| Prepare 전 | Source가 계속 소유 |
| Prepare 후 Ready 전 | Timeout 후 Source 유지 |
| Source freeze 후 Commit 전 | Rollback 또는 같은 Transfer 재전송 |
| Target commit 후 Ack 유실 | 같은 Transfer ID로 재전송 (멱등) |
| Commit 직후 Source 장애 | Target이 더 높은 epoch로 소유 |
| 양쪽 연결 단절 | Lease 또는 Coordinator 판정 |

#### 완료 조건

- ★ **클라 재접속 없이 District를 이동한다**
- 입력 중복 · 유실 테스트가 있다
- Prepare / Commit / Ack 유실 테스트가 있다
- 동일 Transfer 메시지의 반복 처리가 멱등적이다
- 동시 authority를 **탐지한다**
- 빠른 경계 왕복이 안전하다
- Handoff P50/P95/P99를 측정한다

**최종 산출물** — **경계를 걸어서 넘는 영상과 서버 로그를 나란히.**

---

### P11 — 장애 복구 (6–8주 · ~60h)

**도입할 개념** — Idempotency · Retry와 timeout · Lease · Epoch와 fencing token ·
Heartbeat · Failure detector · Transfer write-ahead record · Snapshot과 replay · Reconciliation

**장애 주입 도구** — 개발용 transport에 다음을 넣는다. 이게 있어야 장애를 *재현*할 수 있다.

```
drop probability · duplicate probability · reorder window
fixed latency · jitter · disconnect after N messages · process kill point
```

**장애 시나리오**
- Gateway 종료 / Source World Node 종료 / Target World Node 종료
- Coordinator 일시 중단 / 노드 간 network partition
- Handoff 메시지 중복 / 지연된 이전 epoch 메시지
- 저장 성공 후 응답 유실 / DB 지연 중 logout

**완료 조건**
- 장애 테스트가 자동화되어 있다
- **오래된 epoch의 write가 차단된다**
- 재전송으로 중복 Entity가 생성되지 않는다
- 복구 정책과 데이터 유실 범위가 문서화되어 있다
- 발견한 주요 장애에 **postmortem이 작성되어 있다**

---

### P12 — Hotspot과 동적 부하 분산 (8–12주 · ~90h)

채널 없는 월드의 현실적 병목은 **전체 접속자 수보다 특정 지역의 인구 집중**이다.

```
평상시                     월드 보스 발생
District A: 1,000          District A:   200
District B: 1,000          District B: 8,000  ← Hotspot
District C: 1,000          District C:   300
```

**관측 지표** — Section별 Actor 수 · Section별 tick CPU · AOI pair 수 · Packet 생성량 ·
AI/pathfinding 비용 · Cross-section interaction 수 · Cross-node traffic · Handoff 빈도

**단계적 접근** — 처음부터 4단계를 구현하지 않는다.
1. 서버 시작 시 고정 District 재배치
2. 런타임 District 전체 이동
3. 인접 Section 묶음 이동
4. 동적 Cell/Partition 분할과 병합

**Thrashing 방지** — Hysteresis · Minimum residency time · Migration cooldown ·
Migration cost 추정 · Expected gain 기준 · Handoff rate limit

#### 🔴 공간 분할로 해결할 수 없는 경우

같은 작은 공간의 **모든 플레이어가 서로 상호작용**하면 계산량과 network fan-out이 본질적으로
증가한다. 500명이 반경 50m에 모이면 공간 분할로는 못 쪼갠다 — 서로 다 봐야 하니까.

> **심리스는 월드 *크기* 문제를 풀지 *밀도* 문제를 풀지 않는다.**
> 이 구분을 처음에 안 하면, 심리스를 다 만들고도 "왜 여전히 터지지"가 된다.

대응 후보 — 관심 영역 제한 · 중요도 기반 replication · Update frequency LOD · AI LOD ·
군중 단순화 · 전투 판정 그룹화 · 투사체 집계 · 게임 디자인상의 밀도 제한

**완료 조건**
- 균등 분포와 hotspot을 **구분해** 측정한다
- Partition 이동 비용을 측정한다
- 부하 변동에 의한 반복 이동이 제한된다
- **Dynamic partition을 사용하지 *않을* 조건도 문서화한다**

---

### P13 — Persistence · 전역 서비스 · 최종 검증 (6–8주 · ~60h)

**분리 순서** — 인메모리 → 단일 DB → 서비스 분리. 한 번에 마이크로서비스로 가지 않는다.

**전역 서비스 후보** — 인증 · 캐릭터 · 인벤토리 · 우편 · 길드 · 채팅 · 랭킹.
**분리 기준**: 트랜잭션 경계가 World tick과 분리 가능한가.

**관측 스택** — Prometheus 익스포터 + Grafana + **Go 대시보드**
(섹션별 부하 · 이주율 · AOI enter/leave · handoff 지연)

#### 최종 부하·장애 시나리오

| 시나리오 | 검증 대상 |
|---|---|
| 균등 분산 | 기준선 |
| 도시 집중 | Hotspot 대응 |
| 월드 보스 | 극단 밀집 |
| 대규모 경계 이동 | Handoff 처리량 |
| 경계 전투 | Ghost + authority 상호작용 |
| 순간이동 폭주 | Thrashing 방지 |
| 장애 중 Handoff | 복합 실패 |

**결과 보고 형식** — 환경(하드웨어·커밋 해시) · 시나리오 정의 · 지표 표 · 그래프 ·
병목 분석 · **처음 예상과 달랐던 점** · 남은 한계

---

## 8. Git history 운영 원칙

> 좋은 history는 커밋 수가 많은 history가 아니라 **질문 · 실패 · 측정 · 결정의 흐름이 보이는**
> history다.

### 기능 하나의 권장 흐름

```
docs(aoi): describe snapshot replication baseline
test(aoi): add visibility transition scenarios
feat(aoi): implement full snapshot query
bench(aoi): record 10k actor baseline
docs(adr): propose cell subscription delta
feat(aoi): add enter and leave deltas
fix(aoi): handle diagonal cell transition
bench(aoi): compare snapshot and delta bandwidth
refactor(aoi): remove obsolete snapshot path
```

이 흐름이 증명하는 것 — 문제를 먼저 정의했다 · 정확성 테스트를 생각했다 · 비교 기준을 만들었다 ·
측정 후 개선했다 · 실패와 버그를 발견했다 · 최종 선택 이유를 설명할 수 있다.

### 피할 커밋 메시지

```
update · fix · final · final2 · server complete · massive refactoring
```

좋은 커밋은 이 질문에 한 문장으로 답할 수 있어야 한다.

> 이 커밋은 **어떤 동작이나 불변식을 하나** 추가하거나 수정했는가?

예: `fix(handoff): reject commit messages with stale authority epoch`

### Branch와 release

- `main`은 항상 빌드와 테스트가 통과하는 상태로 유지
- 기능은 짧은 feature branch에서
- Milestone마다 release tag와 release note
- Benchmark 결과에는 **commit hash와 실행 환경**을 포함
- 🔴 **과거의 실패를 숨기기 위해 history를 다시 쓰지 않는다**
- 막힌 접근은 지우지 말고 `experiment/*` 브랜치로 남기고 devlog에 왜 접었는지 쓴다

### 커밋 리듬

주 3–8커밋. **폭발적 커밋 뒤 몇 주 공백이 가장 의심스러운 패턴이다.**
집에서만 push하므로 로컬 커밋을 잘게 쪼개는 규율이 필요하다.

### 주 1회 devlog

3–5줄이어도 된다. 이번 주에 뭘 알아냈고 뭐가 안 됐는지.
**가장 싸고 가장 강력한 증거.** 못 한 주에는 "이번 주는 못 했다"고 쓴다.

---

## 9. AI 활용과 본인 설계 증명

AI 사용을 숨기는 것보다 **개발자가 무엇을 이해하고 결정했는지 증명**하는 것이 중요하다.

| AI가 보조해도 되는 것 | 본인이 반드시 결정할 것 |
|---|---|
| 설계 대안 조사 | **상태 불변식** |
| 테스트 사례 제안 | **Authority 경계** |
| 반복적인 boilerplate 초안 | **Thread ownership** |
| 코드 리뷰 | **실패와 rollback 정책** |
| 문서 표현 개선 | **성능 목표와 일관성 수준** |
| 장애 시나리오 발굴 | **대안 선택 이유 · Benchmark 해석** |

ADR과 Design Question의 핵심은 채택안이 아니라 **버린 안과 그 이유**다.
AI 산출물에는 "고려했지만 안 한 것"이 없다. 이게 가장 강한 판별 신호다.

---

## 10. 문서 템플릿

### 10.1 Design Question

```markdown
# Handoff 중 입력은 누가 처리하는가?

## 문제
Source freeze 이후 Target commit 전에도 client 입력이 도착한다.

## 후보
1. Source가 buffer한다.
2. Gateway가 buffer한다.
3. Target에 speculative forwarding한다.

## 선택
초기 버전에서는 Gateway가 Transfer ID별로 buffer한다.

## 이유
## 단점
## 재검토 조건
Handoff P95가 목표치를 넘거나 Gateway memory 비용이 문제가 될 때.
```

### 10.2 Milestone

```markdown
# v0.x Milestone Name

## 목표
## 구현 범위
## 제외 범위
## 핵심 불변식
## Architecture
## 대안과 선택 이유
## Test Scenario
## Benchmark Environment
## Benchmark Result
## 발견한 문제
## 처음 잘못 판단한 부분     ← 가장 중요
## 알려진 한계
## 다음 단계
```

### 10.3 Postmortem

```markdown
# Duplicate authority after lost CommitAck

## 현상
## 재현 조건
## 영향
## 근본 원인
## 임시 수정
## 구조적 수정
## 추가한 테스트
## 남은 위험
```

---

## 11. 심리스 개념 압축 — 막혔을 때 여기부터

**근본 문제는 하나다.** 월드를 여러 실행 단위로 쪼개면 액터는 자기 것만 만지지 않는다 —
남의 HP를 깎고, 남의 송신버퍼에 밀어넣고, 다른 섹션의 목록에 자기를 넣는다. 셋 다 자기 실행
단위 **밖의 메모리에 쓰는 행위**다.

**답은 셋뿐이다.** 락 · 큐(mailbox) · 기하 규정. (P6 실험 A/B/C)

**기하 규정의 함정** — 위험한 쌍은 "서로 만지는 둘"이 아니라 **같은 것을 만지는 둘**이다.

**AOI는 양방향으로 나눈다.** 뷰어와 피관찰자는 같은 사건의 양면이라 테이블 하나를 공유한다.

**게이트웨이가 있어야 심리스가 심리스다.** 클라가 시뮬 노드에 직결되면 노드가 바뀔 때
재접속이 보인다.

**분산에서는 epoch가 진실을 정한다.** 메시지는 늦게·중복해서·역순으로 온다. 어느 쪽이
최신인지 판단하는 유일한 근거가 epoch와 sequence다.

**심리스가 못 푸는 것 = 밀집.** 월드 크기 문제와 밀도 문제는 다른 문제다.

---

## 12. 첫 8주 실행 계획

| 주 | 할 일 |
|---|---|
| **1** | 저장소 생성 · 목표와 **비목표** 작성 · CMake · 테스트 프레임워크와 CI · `ADR-0001 Authoritative Server` |
| **2** | TCP accept와 Session · Packet framing · 테스트 클라이언트 · Fragmented packet 테스트 |
| **3** | Bounded send queue · Disconnect 수명 · Heartbeat · Slow client 제한 · 네트워크 부하 측정 |
| **4** | Single World thread · Entity ID · Player spawn/despawn · Input command queue |
| **5** | Authoritative movement · 좌표·속도·상태 검증 · 비정상 입력 테스트 · 봇 이동 |
| **6** | Monster와 HP · 단순 공격 · Deferred destruction · Disconnect cleanup |
| **7** | Uniform spatial grid · Cell 이동 · 반경 질의 · 공간 색인 정확성 테스트 |
| **8** | AOI full snapshot · Enter/Leave 비교 구현 · 1천·1만 Entity benchmark · Milestone 회고 |

**1주차 목표는 코드가 아니라 굴러가는 저장소다.**

---

## 13. 마지막 — 이 프로젝트가 답해야 할 질문

최종 결과물의 가치는 코드 규모가 아니라 아래를 명확히 설명할 수 있는지에 달려 있다.

1. **누가 상태를 소유하는가?**
2. **어떤 순서로 상태가 변하는가?**
3. **두 서버의 판단이 충돌하면 누가 이기는가?**
4. **메시지가 유실되거나 중복되면 어떻게 되는가?**
5. **한 지역에 부하가 몰리면 무엇이 병목인가?**
6. **성능을 위해 어떤 정확성 또는 복잡성 비용을 지불했는가?**
7. **최초 설계와 실제 측정 결과가 어떻게 달랐는가?**

이 과정을 순서대로 남기면 저장소는 완제품 하나를 전시하는 공간이 아니라,
**저수준 네트워크 서버 경험에서 출발해 분산 심리스 월드 설계 능력을 획득한 개발 기록**이 된다.
