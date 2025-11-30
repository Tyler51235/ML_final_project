import os
import librosa
import soundfile as sf
import numpy as np
from tqdm import tqdm

# -----------------------------------------------------
# Base directory = where this script is located
# -----------------------------------------------------
BASE_DIR = os.path.dirname(os.path.abspath(__file__))

# -----------------------------------------------------
# Relative paths inside the repo
# -----------------------------------------------------
INPUT_DIR = os.path.join(BASE_DIR, "data", "train_dataset")
OUTPUT_NOISE = os.path.join(BASE_DIR, "noise")
OUTPUT_BG = os.path.join(BASE_DIR, "background")

# Create output folders if missing
os.makedirs(OUTPUT_NOISE, exist_ok=True)
os.makedirs(OUTPUT_BG, exist_ok=True)

# -----------------------------------------------------
# Parameters
# -----------------------------------------------------
SAMPLE_RATE = 16000
WINDOW_SIZE = 2048
HOP_SIZE = 512
CLIP_LENGTH = 2.0
THRESHOLD_MULTIPLIER = 4


def detect_transient(y):
    """Detects spikes/noisy segments using RMS threshold."""
    rms = librosa.feature.rms(y=y, frame_length=WINDOW_SIZE, hop_length=HOP_SIZE)[0]
    mean_rms = np.mean(rms)
    threshold = THRESHOLD_MULTIPLIER * mean_rms

    event_indices = np.where(rms > threshold)[0]
    if len(event_indices) == 0:
        return None

    event_frame = event_indices[0]
    event_time = librosa.frames_to_time(event_frame, sr=SAMPLE_RATE, hop_length=HOP_SIZE)
    return event_time


print("INPUT DIR:", INPUT_DIR)
print("OUTPUT NOISE:", OUTPUT_NOISE)
print("OUTPUT BACKGROUND:", OUTPUT_BG)

# -----------------------------------------------------
# Main loop
# -----------------------------------------------------
if not os.path.isdir(INPUT_DIR):
    raise FileNotFoundError(f"Missing input directory: {INPUT_DIR}")

for file in tqdm(os.listdir(INPUT_DIR)):
    if not file.lower().endswith(".wav"):
        continue

    path = os.path.join(INPUT_DIR, file)

    # Load audio
    y, sr = librosa.load(path, sr=SAMPLE_RATE)

    # Detect transient/noisy event
    event_time = detect_transient(y)

    if event_time is None:
        # Save background slice
        clip = y[: int(CLIP_LENGTH * SAMPLE_RATE)]
        sf.write(os.path.join(OUTPUT_BG, f"bg_{file}"), clip, SAMPLE_RATE)

    else:
        # Save noisy slice
        center = int(event_time * SAMPLE_RATE)
        half_len = int((CLIP_LENGTH / 2) * SAMPLE_RATE)

        start = max(0, center - half_len)
        end = min(len(y), center + half_len)

        clip = y[start:end]

        sf.write(os.path.join(OUTPUT_NOISE, f"noise_{file}"), clip, SAMPLE_RATE)

print("Done!")
