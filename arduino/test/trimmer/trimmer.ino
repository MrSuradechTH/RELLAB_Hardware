const int trimmer[] = {32, 35, 34, 39, 36};
const float max_trimmer_value = 4085;
const float min_trimmer_value = 10;
const float out_max_trimmer_value = 0.50;
const float out_min_trimmer_value = -0.50;
const int trimmer_averaging = 25;
static float trimmer_value[sizeof(trimmer) / sizeof(trimmer[0])];

void update_trimmer_value() {
    for (int i = 0; i < sizeof(trimmer) / sizeof(trimmer[0]); i++) {
        for (int j = 0; j < trimmer_averaging; j++) {
            trimmer_value[i] = (((((constrain(analogRead(trimmer[i]) - min_trimmer_value, 0, max_trimmer_value - min_trimmer_value) / (max_trimmer_value - min_trimmer_value)) * (out_max_trimmer_value - out_min_trimmer_value)) + out_min_trimmer_value) * -1) + trimmer_value[i]) / 2;
        }
    }
}

void setup() {
    Serial.begin(2000000);
    for (int i = 0; i < sizeof(trimmer) / sizeof(trimmer[0]); i++) {
        pinMode(trimmer[i], INPUT);
    }
}

void loop() {
    update_trimmer_value();
    for (int i = 0; i < sizeof(trimmer) / sizeof(trimmer[0]); i++) {
        Serial.println("Trimmer[" + String(i) + "] value : " + String(trimmer_value[i], 2));
    }
    delay(1000);
}