"""Exercise the CLI report with old logs and marked developer sessions."""
import json
import pathlib
import subprocess
import sys
import tempfile
import unittest


class RunLogReport(unittest.TestCase):
    def test_old_rows_and_new_retry_rows(self):
        old = dict(how="quit", mode="STORY", difficulty=1, seconds=30,
                   killedBy="", damageTaken={}, guns={}, kills={}, ttk={},
                   stuns=0, finishers=0, finisherHeal=0, overdrives=0,
                   overdriveSeconds=0, chapter=3, section=2,
                   sectionSeconds={"2": 30}, lostSeconds={}, lostAt=[])
        retry = dict(old, seconds=0, how="retry", retries=1,
                     sectionSeconds={}, parryAttempts=3, parrySuccesses=1)
        capture = dict(old, capture=True, seconds=600, parryAttempts=99)
        with tempfile.TemporaryDirectory() as directory:
            path = pathlib.Path(directory) / "runs.jsonl"
            path.write_text("\n".join(json.dumps(r) for r in [old, retry, capture]))
            def report(*args):
                return subprocess.check_output([sys.executable, "tools/runlog.py", str(path), *args], text=True)
            result = report()
            self.assertIn("2 stretches, 0.5 min", result)
            self.assertIn("3 punch/guard presses, 1 successful parry contacts, 1 retries", result)
            self.assertIn("1 retries", result)
            self.assertIn("3 stretches, 10.5 min", report("--include-dev"))
            self.assertIn("1 stretches, 0.0 min", report("--last", "1"))


if __name__ == "__main__":
    unittest.main()
