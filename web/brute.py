"""Check candidate Hack the North profile URLs with an existing Playwright browser.

Use only with permission from the event/site owner. The checker is deliberately
sequential and bounded so a word-list mistake does not create an unbounded
request storm.
"""

from __future__ import annotations

import argparse
import itertools
import sys
import time
from pathlib import Path

from playwright.sync_api import Error as PlaywrightError
from playwright.sync_api import TimeoutError as PlaywrightTimeoutError
from playwright.sync_api import sync_playwright


DEFAULT_BASE_URL = "https://my.hackthenorth.com/qr/2026/{}"
DEFAULT_CDP_URL = "http://127.0.0.1:9222"
DEFAULT_WORKING_FILE = "working_profiles.txt"
DEFAULT_NON_WORKING_FILE = "non_working_profiles.txt"
PROFILE_MARKER = "INTERESTS"
KNOWN_PROFILES = (
	"mint-crisp-salmon-ginger",
	"lemon-cedar-koala-cricket",
)


def read_words(path: Path) -> list[str]:
	"""Read one non-empty word per line, ignoring comments."""
	words = []
	for line in path.read_text(encoding="utf-8").splitlines():
		word = line.split("#", 1)[0].strip()
		if word:
			words.append(word)
	return words


def candidate_slugs(args: argparse.Namespace) -> list[str]:
	candidates = list(args.candidate)

	if args.word_file:
		if len(args.word_file) != 4:
			raise ValueError("--word-file must be supplied exactly four times")
		word_lists = [read_words(path) for path in args.word_file]
		combinations = itertools.product(*word_lists)
		candidates.extend("-".join(parts) for parts in combinations)

	if args.include_known:
		candidates.extend(KNOWN_PROFILES)

	unique = list(dict.fromkeys(slug.strip("/ ") for slug in candidates if slug.strip("/ ")))
	if len(unique) > args.max_candidates:
		raise ValueError(
			f"{len(unique)} candidates exceed --max-candidates={args.max_candidates}"
		)
	return unique


def check_profile(page, url: str, timeout_ms: int) -> tuple[bool, str]:
	"""Return (is_working, detail) for one candidate URL."""
	try:
		response = page.goto(url, wait_until="domcontentloaded", timeout=timeout_ms)
		status = response.status if response is not None else None

		try:
			page.get_by_text(PROFILE_MARKER, exact=True).wait_for(
				state="visible", timeout=timeout_ms
			)
			marker_found = True
		except PlaywrightTimeoutError:
			marker_found = False

		is_working = status is not None and 200 <= status < 300 and marker_found
		detail = f"status={status}, marker={marker_found}"
		return is_working, detail
	except PlaywrightError as error:
		return False, f"browser_error={type(error).__name__}"


def parse_args() -> argparse.Namespace:
	parser = argparse.ArgumentParser(description=__doc__)
	parser.add_argument(
		"--candidate",
		action="append",
		default=[],
		help="A full four-word slug; may be supplied more than once.",
	)
	parser.add_argument(
		"--word-file",
		action="append",
		type=Path,
		default=[],
		help="A file containing one word per line; supply exactly four files.",
	)
	parser.add_argument("--include-known", action="store_true")
	parser.add_argument("--cdp-url", default=DEFAULT_CDP_URL)
	parser.add_argument("--base-url", default=DEFAULT_BASE_URL)
	parser.add_argument("--working-file", type=Path, default=Path(DEFAULT_WORKING_FILE))
	parser.add_argument(
		"--non-working-file",
		type=Path,
		default=Path(DEFAULT_NON_WORKING_FILE),
	)
	parser.add_argument("--delay", type=float, default=1.0, help="Seconds between checks.")
	parser.add_argument("--timeout-ms", type=int, default=15_000)
	parser.add_argument("--max-candidates", type=int, default=10_000)
	return parser.parse_args()


def main() -> int:
	args = parse_args()
	try:
		slugs = candidate_slugs(args)
	except (OSError, ValueError) as error:
		print(f"error: {error}", file=sys.stderr)
		return 2

	if not slugs:
		print("error: provide --candidate or four --word-file arguments", file=sys.stderr)
		return 2

	args.working_file.touch()
	args.non_working_file.touch()
	seen = {
		line.strip()
		for output_file in (args.working_file, args.non_working_file)
		for line in output_file.read_text(encoding="utf-8").splitlines()
		if line.strip()
	}

	with sync_playwright() as playwright:
		try:
			browser = playwright.chromium.connect_over_cdp(args.cdp_url)
		except PlaywrightError as error:
			print(
				f"error: could not connect to {args.cdp_url}; start Chromium with remote debugging first: {error}",
				file=sys.stderr,
			)
			return 1

		if not browser.contexts:
			print("error: the CDP browser has no context", file=sys.stderr)
			return 1

		context = browser.contexts[0]
		created_page = not context.pages
		page = context.pages[0] if context.pages else context.new_page()
		try:
			for index, slug in enumerate(slugs, start=1):
				url = args.base_url.format(slug)
				if url in seen:
					print(f"[{index}/{len(slugs)}] skipped {url}")
					continue

				is_working, detail = check_profile(page, url, args.timeout_ms)
				output_file = args.working_file if is_working else args.non_working_file
				with output_file.open("a", encoding="utf-8") as handle:
					handle.write(f"{url}\n")
				seen.add(url)
				label = "working" if is_working else "non-working"
				print(f"[{index}/{len(slugs)}] {label}: {url} ({detail})", flush=True)
				if index < len(slugs):
					time.sleep(max(args.delay, 0))
		finally:
			if created_page:
				page.close()

	return 0


if __name__ == "__main__":
	raise SystemExit(main())
