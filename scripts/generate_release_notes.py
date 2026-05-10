#!/usr/bin/env python3
import json
import os
import re
import subprocess
import sys
import urllib.request


HF_MODELS = [
    "sberbank-ai/rugpt3small_based_on_gpt2",
    "google/flan-t5-base",
]


def run(cmd: list[str]) -> str:
    return subprocess.check_output(cmd, text=True).strip()


def get_app_version(makefile_path: str = "Makefile") -> str:
    with open(makefile_path, "r", encoding="utf-8") as f:
        text = f.read()
    m = re.search(r"^APP_VERSION\s*:?=\s*([^#\s]+)", text, flags=re.M)
    if not m:
        raise RuntimeError("APP_VERSION not found in Makefile")
    return m.group(1).strip()


def get_commit_messages(limit: int = 20) -> list[str]:
    log = run(["git", "log", f"-n{limit}", "--pretty=format:%s"])
    return [line for line in log.splitlines() if line]


def build_fallback_summary(version: str, commits: list[str]) -> str:
    bullets = "\n".join([f"- {c}" for c in commits[:8]]) or "- Технические улучшения и стабилизация."
    return (
        f"Релиз v{version} сфокусирован на стабильности, автоматизации и улучшении процесса сборки.\n\n"
        f"Основные изменения:\n{bullets}\n\n"
        "Рекомендация по обновлению: установите новую сборку и перезапустите приложение."
    )


def _extract_generated_text(data: object) -> str | None:
    if isinstance(data, list) and data:
        item = data[0]
        if isinstance(item, dict):
            text = item.get("generated_text") or item.get("summary_text")
            if isinstance(text, str) and text.strip():
                return text.strip()
    if isinstance(data, dict):
        text = data.get("generated_text") or data.get("summary_text")
        if isinstance(text, str) and text.strip():
            return text.strip()
    return None


def try_model(model: str, prompt: str, token: str) -> str | None:
    payload = json.dumps(
        {
            "inputs": prompt,
            "parameters": {
                "max_new_tokens": 280,
                "temperature": 0.3,
                "return_full_text": False,
            },
            "options": {"wait_for_model": True},
        }
    ).encode("utf-8")

    headers = {"Content-Type": "application/json"}
    if token:
        headers["Authorization"] = f"Bearer {token}"

    req = urllib.request.Request(
        url=f"https://api-inference.huggingface.co/models/{model}",
        data=payload,
        headers=headers,
        method="POST",
    )

    try:
        with urllib.request.urlopen(req, timeout=45) as resp:
            data = json.loads(resp.read().decode("utf-8"))
    except Exception:
        return None

    return _extract_generated_text(data)


def is_probably_russian(text: str) -> bool:
    return bool(re.search(r"[А-Яа-яЁё]", text))


def build_ai_summary(version: str, commits: list[str]) -> str | None:
    token = os.getenv("HF_API_TOKEN", "").strip()

    prompt = (
        "Напиши релиз-ноты на русском языке для Nintendo Switch homebrew проекта. "
        "Верни обычный текст в формате: краткое вступление, затем список пунктов с улучшениями/исправлениями, "
        "и в конце одна строка с рекомендацией по обновлению.\n"
        f"Версия: {version}\n"
        "Последние коммиты:\n" + "\n".join(f"- {c}" for c in commits[:12])
    )

    for model in HF_MODELS:
        text = try_model(model, prompt, token)
        if text and is_probably_russian(text):
            return text
    return None


def main() -> int:
    out = sys.argv[1] if len(sys.argv) > 1 else "release-notes.md"
    version = get_app_version()
    commits = get_commit_messages()

    ai_summary = build_ai_summary(version, commits)
    summary = ai_summary or build_fallback_summary(version, commits)

    content = f"# Ryazha-Status-Monitor v{version}\n\n{summary}\n"
    with open(out, "w", encoding="utf-8") as f:
        f.write(content)

    print(f"Wrote {out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
