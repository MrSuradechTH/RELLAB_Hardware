const int led_green = 17;
const int led_yellow = 16;
const int led_red = 15;

void setup() {
    pinMode(led_green, OUTPUT);
    pinMode(led_yellow, OUTPUT);
    pinMode(led_red, OUTPUT);
}

void loop() {
    digitalWrite(led_green, HIGH);
    digitalWrite(led_yellow, LOW);
    digitalWrite(led_red, LOW);
    delay(1000);

    digitalWrite(led_green, LOW);
    digitalWrite(led_yellow, HIGH);
    digitalWrite(led_red, LOW);
    delay(1000);

    digitalWrite(led_green, LOW);
    digitalWrite(led_yellow, LOW);
    digitalWrite(led_red, HIGH);
    delay(1000);

    digitalWrite(led_green, LOW);
    digitalWrite(led_yellow, LOW);
    digitalWrite(led_red, LOW);
    delay(1000);

    digitalWrite(led_green, HIGH);
    digitalWrite(led_yellow, HIGH);
    digitalWrite(led_red, HIGH);
    delay(1000);

    digitalWrite(led_green, LOW);
    digitalWrite(led_yellow, LOW);
    digitalWrite(led_red, LOW);
    delay(1000);
}