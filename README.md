# Portfolio — 게임 서버

C++ 게임 서버 개인 포트폴리오. 두 프로젝트를 한 저장소에서 진행한다.

| 순서 | 프로젝트 | 내용 | 상태 |
|---|---|---|---|
| **1** | [`modern-iocp/`](modern-iocp/) | 2024년 IOCP 포트폴리오를 현재 기술 스택으로 리뉴얼 | **다음 착수** |
| 2 | [`seamless-world/`](seamless-world/) | 채널 없는 심리스 오픈월드 서버. C++ 시뮬 코어 · Rust 게이트웨이 · Go 대시보드 · UE5 클라 | 대기 (P0부터) |

순서는 `modern-iocp` → `seamless-world` 로 확정됐다(2026-08-28). 병행하지 않는다.

## 이 저장소가 보여주려는 것

완성품이 아니라 **판단의 흐름**이다 — 무엇을 왜 선택했고, 무엇을 버렸고, 어떻게 측정했는가.

- 설계 결정은 [`.ai/decisions/`](.ai/decisions/) 에 **버린 안과 그 이유까지** 남긴다
- 성능 주장은 **commit hash 와 실행 환경을 포함한 측정**으로만 한다
- 실패한 접근은 지우지 않고 `experiment/*` 브랜치와 devlog 에 남긴다
- AI 는 조사·초안·리뷰까지만 쓴다. 설계 결정은 사람이 한다 ([`seamless-world/CLAUDE.md`](seamless-world/CLAUDE.md))

## AI 교차개발 하네스

`Claude 구현 → Codex 독립 리뷰` 파이프라인을 [`.ai/`](.ai/) 에 두고 그 기록을 함께 커밋한다.
프로세스 자체가 산출물이다. 진입점: [`.ai/README.md`](.ai/README.md)

## 구조

```
.ai/          작업 명세 · 독립 리뷰 · 설계 결정 · 실행 계약
.agents/      교차개발 절차 스킬
seamless-world/   ROADMAP.md 가 정본
modern-iocp/
```
