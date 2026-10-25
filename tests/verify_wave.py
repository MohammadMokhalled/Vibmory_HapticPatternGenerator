"""Decode exported files independently with Python's WAV reader (Python 3.12+)."""

from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import wave

with tempfile.TemporaryDirectory(prefix="vibmory-wave-") as temporary:
    for channels in (1, 2, 6):
        path = Path(temporary) / f"{channels}.wav"
        subprocess.run([sys.argv[1], str(path), str(channels)], check=True)
        with wave.open(str(path), "rb") as decoded:
            assert decoded.getnchannels() == channels
            assert decoded.getsampwidth() == 2
            assert decoded.getframerate() == 8000
            assert decoded.getnframes() == 80
            samples = struct.unpack("<" + "h" * 80 * channels, decoded.readframes(80))
            assert samples[:channels] == (0,) * channels
            assert samples[channels:channels * 2] == tuple(round(1000 * (i + 1) / 2 ** 0.5) for i in range(channels))
            assert samples[channels * 2:channels * 3] == tuple(1000 * (i + 1) for i in range(channels))
        raw = path.read_bytes()
        assert struct.unpack_from("<I", raw, 4)[0] == len(raw) - 8
        assert len(raw) == (68 if channels > 2 else 44) + channels * 80 * 2
print("Independent mono, stereo and multichannel PCM WAV decoding passed.")
