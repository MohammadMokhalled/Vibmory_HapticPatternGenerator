"""Release gates reject unrelated builds and refuse published-release changes."""

from copy import deepcopy
from pathlib import Path
import sys
import tempfile
import unittest
import zipfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from create_draft_release import extract_runtime, validate_build, validate_existing_release


class ReleaseTests(unittest.TestCase):
    def setUp(self):
        self.repository = "owner/project"
        self.sha = "a" * 40
        self.run = {"repository": {"full_name": self.repository}, "head_repository": {"full_name": self.repository},
                    "head_sha": self.sha, "path": ".github/workflows/build.yml", "event": "push", "status": "in_progress"}
        self.jobs = [{"name": "Core / Linux / sanitizers", "conclusion": "success"},
                     {"name": "Windows desktop and portable executable", "conclusion": "success"}]
        self.metadata = {"version": "2.0.0", "sourceRevision": self.sha}

    def test_only_matching_version_tags_and_source_commits_are_accepted(self):
        for tag in ("v2.0.0", "v2.0.0-rc.1"):
            validate_build(self.repository, self.run, self.jobs, self.metadata, tag)
        for tag in ("v2.0.1", "v2.0.0 --publish", "--draft", "v2.0.0/other"):
            with self.assertRaises(ValueError):
                validate_build(self.repository, self.run, self.jobs, self.metadata, tag)
        with self.assertRaises(ValueError):
            validate_build(self.repository, self.run, self.jobs, {**self.metadata, "sourceRevision": "b" * 40}, "v2.0.0")

    def test_failed_missing_or_untrusted_jobs_cannot_create_a_release(self):
        for update in ({"event": "pull_request"}, {"status": "completed", "conclusion": "failure"},
                       {"head_repository": {"full_name": "fork/project"}}, {"path": "unrelated.yml"}):
            run = {**deepcopy(self.run), **update}
            with self.assertRaises(ValueError):
                validate_build(self.repository, run, self.jobs, self.metadata, "v2.0.0")
        for jobs in ([], self.jobs[:1], [self.jobs[0], {**self.jobs[1], "conclusion": "failure"}]):
            with self.assertRaises(ValueError):
                validate_build(self.repository, self.run, jobs, self.metadata, "v2.0.0")

    def test_only_this_builds_own_draft_can_be_retried(self):
        marker = "<!-- vibmory-build:123:abc -->"
        validate_existing_release(None, marker)
        validate_existing_release({"draft": True, "body": marker}, marker)
        for release in ({"draft": False, "body": marker}, {"draft": True, "body": "another draft"}):
            with self.assertRaises(ValueError):
                validate_existing_release(release, marker)

    def test_runtime_archive_cannot_write_outside_its_root(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            path = root / "runtime.zip"
            with zipfile.ZipFile(path, "w") as archive:
                archive.writestr("Vibmory/../../outside", b"bad")
            with self.assertRaises(ValueError):
                extract_runtime(path, root / "extract")
            self.assertFalse((root / "outside").exists())


if __name__ == "__main__":
    unittest.main()
