#!/usr/bin/env python3
"""1단계: 어떤 영상을 몇 개, 어떤 해상도·길이로 만들지 계획을 세운다 (metadata.csv 생성).

용량 한도(--budget-gb)에 맞춰 영상 길이를 자동으로 조절한다.
이 파일이 '수급처가 보내 준 메타데이터' 역할을 한다.

예)  python3 tools/plan.py --count 3000 --budget-gb 18
"""
import argparse
import csv
import random
from datetime import date, timedelta
from pathlib import Path

# 해상도별 1초당 용량(MB). FFmpeg 합성 영상(H.264 crf 28 + AAC 96k)으로 직접 측정한 값.
RESOLUTIONS = {
    720:  dict(width=1280, mb_per_sec=0.17, median=20, max_sec=600),
    1080: dict(width=1920, mb_per_sec=0.30, median=18, max_sec=300),
    2160: dict(width=3840, mb_per_sec=1.00, median=10, max_sec=120),   # 4K
}
MIN_SEC = 6

ADJ = ["푸른", "깊은", "낯선", "오래된", "작은", "붉은", "조용한", "잊힌", "빛나는", "차가운", "마지막", "보이지 않는",
       "끝없는", "따뜻한", "부서진", "숨겨진", "새벽의", "한밤의", "우리의", "먼", "흔들리는", "하얀", "검은", "황금빛",
       "외로운", "눈부신", "비밀의", "달콤한", "거친", "고요한"]
NOUN = ["항로", "정원", "도서관", "바다", "도시", "우주", "편지", "기억", "약속", "계절", "여름", "겨울", "섬", "기차",
        "골목", "하늘", "숲", "별", "등대", "거울", "파도", "노래", "시간", "얼굴", "문", "길", "방", "밤", "소년", "소녀",
        "탐정", "요리사", "왕국", "신호", "궤도", "사막", "강", "다리", "창문", "온도"]
SEQUEL = ["", " 2", " 3"]
GENRES = ["드라마", "SF", "판타지", "스릴러", "코미디", "로맨스", "액션", "애니메이션", "다큐멘터리", "공포"]
RATINGS = ["ALL", "12", "15", "19"]
SURNAME = list("김이박최정강조윤장임한오서신권황안송류홍")
GIVEN = ["하늘", "도윤", "서윤", "우진", "지민", "예린", "시우", "하준", "수아", "민재", "유나", "태오", "지호", "나연", "현우", "다은"]
TAGS = ["모험", "성장", "가족", "우정", "사랑", "복수", "미스터리", "일상", "청춘", "생존", "여행", "음악", "음식", "역사", "재난"]


def with_particle(word):
    """'복수' → '복수와', '사랑' → '사랑과' (받침에 따라 조사 선택)"""
    return word + ("과" if (ord(word[-1]) - 0xAC00) % 28 else "와")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--count", type=int, default=3000, help="영상 개수")
    ap.add_argument("--budget-gb", type=float, default=18.0,
                    help="원본 영상 합계 용량 한도(GB). HLS 변환본이 비슷한 크기로 한 벌 더 생기므로 버킷 한도의 45% 정도로 잡는다")
    ap.add_argument("--mix", default="60,33,7", help="720p,1080p,4K 개수 비율(%%)")
    ap.add_argument("--out", default="metadata.csv")
    ap.add_argument("--seed", type=int, default=42, help="같은 값이면 항상 같은 계획이 나온다")
    args = ap.parse_args()

    rnd = random.Random(args.seed)
    mix = [float(x) for x in args.mix.split(",")]
    heights = rnd.choices([720, 1080, 2160], weights=mix, k=args.count)

    # 1) 길이를 '짧은 영상이 많고 긴 영상이 조금' 있는 분포로 뽑는다
    raw = [rnd.lognormvariate(0, 0.8) * RESOLUTIONS[h]["median"] for h in heights]

    # 2) 합계 용량이 한도에 맞도록 길이를 같은 비율로 늘이거나 줄인다
    budget_mb = args.budget_gb * 1024
    scale = 1.0
    for _ in range(30):
        durations = [max(MIN_SEC, min(RESOLUTIONS[h]["max_sec"], round(d * scale))) for h, d in zip(heights, raw)]
        total_mb = sum(d * RESOLUTIONS[h]["mb_per_sec"] for h, d in zip(heights, durations))
        if abs(total_mb - budget_mb) / budget_mb < 0.01:
            break
        scale *= budget_mb / total_mb

    titles = [f"{a} {n}{s}" for s in SEQUEL for a in ADJ for n in NOUN]
    rnd.shuffle(titles)
    if args.count > len(titles):
        ap.error(f"제목 조합은 최대 {len(titles)}개입니다")

    today = date.today()
    rows = []
    for i in range(args.count):
        h, dur = heights[i], durations[i]
        genre, tags = rnd.choice(GENRES), rnd.sample(TAGS, 2)
        year = rnd.randint(1995, today.year)
        # 판권 기간: 대부분 서비스 중, 2%는 이미 만료, 2%는 아직 시작 전 → CMS가 걸러내는지 확인용
        r = rnd.random()
        if r < 0.02:
            start, end = today - timedelta(days=700), today - timedelta(days=rnd.randint(1, 60))
        elif r < 0.04:
            start, end = today + timedelta(days=rnd.randint(30, 120)), today + timedelta(days=800)
        else:
            start, end = today - timedelta(days=rnd.randint(30, 400)), today + timedelta(days=rnd.randint(180, 1000))
        rows.append(dict(
            cid=f"C{i + 1:05d}", title=titles[i], genre=genre, release_year=year,
            age_rating=rnd.choice(RATINGS),
            actors=", ".join(rnd.choice(SURNAME) + rnd.choice(GIVEN) for _ in range(2)),
            tags=", ".join(tags),
            description=f"{titles[i]}에 얽힌 {with_particle(tags[0])} {tags[1]}의 이야기. ({genre}, {year})",
            width=RESOLUTIONS[h]["width"], height=h, duration_sec=dur,
            est_mb=round(dur * RESOLUTIONS[h]["mb_per_sec"], 1),
            license_start=start.isoformat(), license_end=end.isoformat(),
        ))

    out = Path(args.out)
    with out.open("w", newline="", encoding="utf-8-sig") as f:
        w = csv.DictWriter(f, fieldnames=rows[0].keys())
        w.writeheader()
        w.writerows(rows)

    print(f"{out} 저장: {len(rows)}개\n")
    print(f"{'해상도':<8}{'개수':>7}{'가장 짧은':>10}{'가장 긴':>9}{'평균':>8}{'총 길이':>10}{'예상 용량':>11}")
    grand_mb = grand_sec = 0
    for h in (720, 1080, 2160):
        ds = [r["duration_sec"] for r in rows if r["height"] == h]
        if not ds:
            continue
        mb = sum(ds) * RESOLUTIONS[h]["mb_per_sec"]
        grand_mb += mb
        grand_sec += sum(ds)
        label = "4K" if h == 2160 else f"{h}p"
        print(f"{label:<10}{len(ds):>8}{min(ds):>11}초{max(ds):>9}초{sum(ds) / len(ds):>8.0f}초"
              f"{sum(ds) / 3600:>9.1f}시간{mb / 1024:>10.1f} GB")
    print(f"{'합계':<9}{len(rows):>8}{'':>33}{grand_sec / 3600:>9.1f}시간{grand_mb / 1024:>10.1f} GB")
    print(f"\n원본 약 {grand_mb / 1024:.1f} GB + HLS 변환본 약 {grand_mb / 1024:.1f} GB = 버킷 사용량 약 {grand_mb / 512:.0f} GB 예상")


if __name__ == "__main__":
    main()
