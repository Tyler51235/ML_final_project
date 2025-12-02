import tensorflow as tf
import numpy as np
from pathlib import Path

# Paths
MODEL_H5 = Path("models/sound_model.h5")   # your trained model from notebook
TFLITE_PATH = Path("models/model_multiclass_int8.tflite")
HEADER_PATH = Path("include/model_data.h")

def convert_to_tflite_int8(keras_model_path, tflite_out):
    model = tf.keras.models.load_model(keras_model_path)

    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    converter.optimizations = [tf.lite.Optimize.DEFAULT]
    # if you want full int8 you can add representative_dataset later.
    tflite_model = converter.convert()

    tflite_out.write_bytes(tflite_model)
    print(f"[INFO] Saved TFLite model -> {tflite_out}")

    return tflite_model

def tflite_to_c_array(tflite_bytes, var_name="g_model"):
    # Format as C array
    hex_array = ", ".join(f"0x{b:02x}" for b in tflite_bytes)
    out = []
    out.append("#pragma once\n")
    out.append("#include <cstddef>\n\n")
    out.append(f"extern const unsigned char {var_name}[];\n")
    out.append(f"extern const size_t {var_name}_len;\n\n")
    out.append(f"const unsigned char {var_name}[] = {{\n  {hex_array}\n}};\n")
    out.append(f"const size_t {var_name}_len = sizeof({var_name});\n")
    return "\n".join(out)

def main():
    if not MODEL_H5.exists():
        raise FileNotFoundError(f"Cannot find {MODEL_H5}, make sure your model is saved there.")

    tflite_bytes = convert_to_tflite_int8(MODEL_H5, TFLITE_PATH)
    header_text = tflite_to_c_array(tflite_bytes, var_name="g_model")

    HEADER_PATH.parent.mkdir(parents=True, exist_ok=True)
    HEADER_PATH.write_text(header_text)
    print(f"[INFO] Wrote header -> {HEADER_PATH}")

if __name__ == "__main__":
    main()
