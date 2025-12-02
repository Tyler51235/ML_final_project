#include <Arduino.h>
#include <I2S.h>
#include "sound_detector.h"

#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"

// ------------------------------
// TensorFlow Model Storage
// ------------------------------
extern const unsigned char Sound_model[];
extern const int Sound_model_len;

// ------------------------------
// TensorFlow Runtime
// ------------------------------
constexpr int kTensorArenaSize = 200 * 1024;
static uint8_t tensorArena[kTensorArenaSize];

tflite::MicroInterpreter* interpreter;
TfLiteTensor* inputTensor;
TfLiteTensor* outputTensor;
tflite::ErrorReporter* errorReporter;

// ------------------------------
// Microphone Settings
// ------------------------------
#define SAMPLE_RATE 16000
#define NUM_SAMPLES 16000   // 1-second window for model

void initMicrophone() {
    // Your mic setup exactly as you posted
    I2S.setAllPins(-1, 42, 41, -1, -1);

    if (!I2S.begin(PDM_MONO_MODE, SAMPLE_RATE, 16)) {
        Serial.println("Failed to init I2S mic!");
        while (1);
    }

    Serial.println("Microphone initialized.");
}

// ------------------------------
// TensorFlow Model Setup
// ------------------------------
void soundDetectorInit() {
    initMicrophone();

    static tflite::MicroErrorReporter micro_error_reporter;
    errorReporter = &micro_error_reporter;

    const tflite::Model* model = tflite::GetModel(Sound_model);
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        Serial.println("Model schema version mismatch!");
        return;
    }

    static tflite::AllOpsResolver resolver;

    interpreter = new tflite::MicroInterpreter(
        model, resolver, tensorArena, kTensorArenaSize, errorReporter);

    interpreter->AllocateTensors();

    inputTensor  = interpreter->input(0);
    outputTensor = interpreter->output(0);

    Serial.println("Sound ML model loaded.");
}

// ------------------------------
// Fill Model Input: 1-second audio
// ------------------------------
void recordAudioForModel() {
    int16_t* buffer = (int16_t*)inputTensor->data.int16;

    for (int i = 0; i < NUM_SAMPLES; i++) {
        int sample = I2S.read();

        if (sample == 0 || sample == -1 || sample == 1) {
            i--; 
            continue;
        }

        buffer[i] = (int16_t)sample;
    }
}

// ------------------------------
// Run Inference + Decide Danger
// ------------------------------
bool isDangerousSound() {
    recordAudioForModel();

    if (interpreter->Invoke() != kTfLiteOk) {
        Serial.println("Inference failed!");
        return false;
    }

    float dangerProb = 0.0;

    // CASE 1: Model output is [safe, dangerous]
    if (outputTensor->dims->data[1] == 2) {
        dangerProb = outputTensor->data.f[1];
    }
    // CASE 2: Model output is sigmoid → 1 neuron
    else {
        dangerProb = outputTensor->data.f[0];
    }

    Serial.print("Danger probability = ");
    Serial.println(dangerProb);

    return (dangerProb > 0.70);
}
