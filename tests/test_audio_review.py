"""Offline integration checks for real audio payloads, credentials and caching.

No tests call the real API or require an API key.
"""
import base64
import contextlib
import hashlib
import importlib.util
import io
import json
import os
from pathlib import Path
import stat
import struct
import tempfile
import unittest
from unittest.mock import patch
import urllib.error
import wave

spec = importlib.util.spec_from_file_location("review_audio", Path(__file__).resolve().parents[1] / "tools/review_audio.py")
review = importlib.util.module_from_spec(spec)
spec.loader.exec_module(review)


class Response(io.BytesIO):
    def __init__(self, value):
        super().__init__(json.dumps(value).encode())
        self.headers = {"x-request-id": "request-test"}


class AudioReview(unittest.TestCase):
    def setUp(self):
        self.folder = tempfile.TemporaryDirectory()
        self.addCleanup(self.folder.cleanup)
        self.root = Path(self.folder.name)
        self.path = self.root / "recording.wav"
        self.pcm = b"".join(struct.pack("<hh", i * 13, -i * 7) for i in range(250))
        with wave.open(str(self.path), "wb") as f:
            f.setparams((2, 2, 100, 0, "NONE", "not compressed"))
            f.writeframes(self.pcm)
        self.clip = next(review.audio_clips(self.path, 1))

    def completion(self, text="Heard: two impacts with silence between them.", reason="stop"):
        return {"id": "completion-test", "model": "gpt-audio-1.5", "usage": {"total_tokens": 123},
                "choices": [{"finish_reason": reason, "message": {"content": text}}]}

    def test_wav_splitting_keeps_every_stereo_sample_and_remainder(self):
        clips = list(review.audio_clips(self.path, 1))
        self.assertEqual([(c["start"], c["end"]) for c in clips], [(0, 1), (1, 2), (2, 2.5)])
        pcm = bytearray()
        for clip in clips:
            with wave.open(io.BytesIO(clip["data"])) as f:
                self.assertEqual((f.getnchannels(), f.getsampwidth(), f.getframerate()), (2, 2, 100))
                pcm.extend(f.readframes(f.getnframes()))
        self.assertEqual(pcm, self.pcm)

    def test_payload_contains_actual_decodable_audio_and_text_only_output(self):
        payload = review.make_payload(self.clip, review.MODEL, "Compare idle and attacks.", 1600)
        self.assertEqual(payload["modalities"], ["text"])
        self.assertFalse(payload["store"])
        audio = payload["messages"][1]["content"][1]
        self.assertEqual(audio["type"], "input_audio")
        self.assertEqual(audio["input_audio"]["format"], "wav")
        data = base64.b64decode(audio["input_audio"]["data"])
        self.assertEqual(hashlib.sha256(data).hexdigest(), self.clip["sha256"])
        with wave.open(io.BytesIO(data)) as f:
            self.assertEqual(f.readframes(f.getnframes()), self.pcm[:400])

    def test_transport_uses_audio_endpoint_and_authorization_without_persisting_key(self):
        key = "sk-test-secret-never-log"
        with patch.object(review.urllib.request, "urlopen", return_value=Response(self.completion("Heard: " + key))) as send:
            record, cached = review.review_one(self.clip, key, review.MODEL, "", 1600, self.root)
        request = send.call_args.args[0]
        self.assertEqual(request.full_url, "https://api.openai.com/v1/chat/completions")
        self.assertEqual(request.get_header("Authorization"), "Bearer " + key)
        self.assertIn("input_audio", request.data.decode())
        self.assertFalse(cached)
        self.assertEqual(record["request_id"], "request-test")
        review.write_report(self.root, [record])
        for path in self.root.rglob("*"):
            if path.is_file() and path.suffix in (".json", ".md"):
                self.assertNotIn(key, path.read_text())
                self.assertNotIn(base64.b64encode(self.clip["data"]).decode(), path.read_text())

    def test_cache_prevents_duplicate_paid_calls_and_context_invalidates_it(self):
        with patch.object(review, "api_request", return_value=(self.completion(), "request-test")) as send:
            first, cached = review.review_one(self.clip, "test-key", review.MODEL, "", 1600, self.root)
            self.assertFalse(cached)
            second, cached = review.review_one(self.clip, "test-key", review.MODEL, "", 1600, self.root)
            self.assertTrue(cached)
            self.assertEqual(first, second)
            self.assertEqual(send.call_count, 1)
            review.review_one(self.clip, "test-key", review.MODEL, "Listen for fatigue.", 1600, self.root)
            self.assertEqual(send.call_count, 2)

    def test_audio_change_invalidates_cache_even_at_same_path(self):
        modified = dict(self.clip, data=self.clip["data"][:-2] + b"\x00\x00")
        modified["sha256"] = hashlib.sha256(modified["data"]).hexdigest()
        self.assertNotEqual(review.fingerprint(self.clip, review.MODEL, "", 1600),
                            review.fingerprint(modified, review.MODEL, "", 1600))

    def test_incomplete_output_is_saved_and_never_silently_retried(self):
        with patch.object(review, "api_request", return_value=(self.completion("Partial review", "length"), "request-test")) as send:
            record, _ = review.review_one(self.clip, "test-key", review.MODEL, "", 1600, self.root)
            self.assertFalse(record["complete"])
            review.write_report(self.root, [record])
            self.assertIn("INCOMPLETE", (self.root / "review.md").read_text())
            again, cached = review.review_one(self.clip, "test-key", review.MODEL, "", 1600, self.root)
            self.assertTrue(cached)
            self.assertFalse(again["complete"])
            self.assertEqual(send.call_count, 1)

    def test_corrupt_cache_stops_without_a_new_paid_request(self):
        identity = review.fingerprint(self.clip, review.MODEL, "", 1600)
        review.write_json(self.root / "responses" / (identity + ".json"), {"fingerprint": identity})
        with patch.object(review, "api_request") as send:
            with self.assertRaisesRegex(review.ReviewError, "cached review is invalid"):
                review.review_one(self.clip, "test-key", review.MODEL, "", 1600, self.root)
            send.assert_not_called()

    def test_dry_run_needs_no_key_and_cannot_call_network(self):
        output = self.root / "output"
        with patch.object(review, "load_key", side_effect=AssertionError("credentials accessed")), \
             patch.object(review, "api_request", side_effect=AssertionError("network accessed")), \
             contextlib.redirect_stdout(io.StringIO()):
            self.assertEqual(review.main([str(self.path), "--dry-run", "--clip-seconds", "1", "--output", str(output)]), 0)
        plan = json.loads((output / "plan.json").read_text())
        self.assertEqual(len(plan["clips"]), 3)
        self.assertEqual(plan["audio_seconds"], 2.5)
        self.assertNotIn("data", plan["clips"][0])
        self.assertFalse((output / "review.md").exists())

    def test_configuration_uses_hidden_input_owner_permissions_and_environment_precedence(self):
        config = self.root / "config.local.audio.json"
        with patch.object(review.getpass, "getpass", return_value="sk-local-example"), \
             patch.dict(os.environ, {}, clear=True), contextlib.redirect_stdout(io.StringIO()) as out:
            review.configure(config)
            self.assertEqual(review.load_key(config), "sk-local-example")
        self.assertNotIn("sk-local-example", out.getvalue())
        if os.name == "posix":
            self.assertEqual(stat.S_IMODE(config.stat().st_mode), 0o600)
        with patch.dict(os.environ, {"OPENAI_API_KEY": "sk-environment-example"}):
            self.assertEqual(review.load_key(config), "sk-environment-example")

    def test_missing_key_fails_before_reading_recordings_or_sending_anything(self):
        with patch.object(review, "load_key", side_effect=review.ReviewError("No API key configured.")), \
             patch.object(review, "audio_clips") as read_audio, \
             patch.object(review, "api_request") as send, contextlib.redirect_stderr(io.StringIO()):
            self.assertEqual(review.main([]), 1)
        read_audio.assert_not_called()
        send.assert_not_called()

    def test_check_validates_model_metadata_without_uploading_audio(self):
        with patch.object(review, "load_key", return_value="test-key"), \
             patch.object(review, "api_request", return_value=({"id": review.MODEL}, "request-test")) as send, \
             patch.object(review, "audio_clips") as read_audio, contextlib.redirect_stdout(io.StringIO()):
            self.assertEqual(review.main(["--check"]), 0)
        send.assert_called_once_with("/models/" + review.MODEL, "test-key", timeout=30)
        read_audio.assert_not_called()
        with patch.object(review, "api_request") as send, contextlib.redirect_stderr(io.StringIO()):
            self.assertEqual(review.main(["--check", "--model", ""]), 1)
        send.assert_not_called()

    def test_http_errors_are_actionable_and_do_not_echo_credentials_or_retry(self):
        for status, code, hint in [(401, "invalid_api_key", "rejected"), (403, "", "access"),
                                    (429, "insufficient_quota", "credits"), (429, "rate_limit", "rate limit")]:
            error = urllib.error.HTTPError("https://api.openai.com", status, "error", {},
                io.BytesIO(json.dumps({"error": {"code": code, "message": "sk-test-secret"}}).encode()))
            with patch.object(review.urllib.request, "urlopen", side_effect=error) as send:
                with self.assertRaises(review.ReviewError) as failure:
                    review.api_request("/chat/completions", "sk-test-secret", {})
            self.assertIn(hint, str(failure.exception))
            self.assertNotIn("sk-test-secret", str(failure.exception))
            self.assertEqual(send.call_count, 1)

    def test_invalid_or_truncated_wav_is_rejected(self):
        self.path.write_bytes(b"not a WAV")
        with self.assertRaises(review.ReviewError):
            list(review.audio_clips(self.path))

    def test_refusal_and_empty_responses_are_not_reviews(self):
        for response in ({}, {"choices": []}, self.completion(""),
                         {"choices": [{"message": {"refusal": "declined"}}]}):
            with self.assertRaises(review.ReviewError):
                review.extract_review(response)

    def test_failed_second_clip_keeps_first_real_review(self):
        output = self.root / "partial"
        with patch.object(review, "load_key", return_value="test-key"), \
             patch.object(review, "api_request", side_effect=[(self.completion(), "request-test"), review.ReviewError("rate limit")]) as send, \
             contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
            self.assertEqual(review.main([str(self.path), "--clip-seconds", "1", "--output", str(output)]), 1)
        self.assertEqual(send.call_count, 2)
        self.assertIn("two impacts", (output / "review.md").read_text())
        self.assertEqual(len(list((output / "responses").glob("*.json"))), 1)


if __name__ == "__main__":
    unittest.main()
