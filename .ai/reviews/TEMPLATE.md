# Review: TASK-YYYYMMDD-short-name

- Reviewer: Codex
- Reviewed baseline:
- Reviewed head/worktree:
- Verdict: Approve | Request changes | Blocked

## Independent checks

구현자의 결론을 복사하지 말고 직접 읽거나 실행한 검사를 기록한다.

| Check | Result | Evidence |
|---|---|---|
| | | |

## Findings

심각도는 다음 기준을 사용한다.

- P0: 데이터·보안·서비스에 즉각 치명적이며 배포 불가
- P1: 정상 사용에서도 큰 오류 또는 교착·크래시 가능, 병합 전 해결
- P2: 제한 조건에서의 correctness·유지보수 문제
- P3: 비차단 개선

각 발견은 다음 형식으로 작성한다.

### [P?] 제목

- File/line:
- Evidence or reproduction:
- Impact:
- Suggested safe direction:

발견이 없으면 `No blocking findings`라고 명시하되 실행하지 못한 검사를 남긴다.

## Acceptance criteria audit

- [ ] 각 완료 조건을 독립 확인했다.
- [ ] 불변식과 실패 경로를 확인했다.
- [ ] 구현자가 생략한 검사를 검토했다.
- [ ] 미커밋 사용자 변경을 침범하지 않았다.

## Residual risks

승인 후에도 남는 위험과 후속 task를 적는다.
