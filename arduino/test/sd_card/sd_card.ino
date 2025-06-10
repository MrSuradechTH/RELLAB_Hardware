#include <SPI.h>
#include <SD.h>

File myFile;
const int CS = 5;

void write_file(const char * path, const char * message){
  myFile = SD.open(path, FILE_WRITE);
  if (myFile) {
    Serial.printf("Writing to %s ", path);
    myFile.println(message);
    myFile.close();
    Serial.println("completed.");
  }else {
    Serial.println("error opening file ");
    Serial.println(path);
  }
}

void read_file(const char * path){
  myFile = SD.open(path);
  if (myFile) {
     Serial.printf("Reading file from %s\n", path);
    while (myFile.available()) {
      Serial.write(myFile.read());
    }
    myFile.close();
  } 
  else {
    Serial.println("error opening test.txt");
  }
}

int cout_line(const char * path) {
  int lines = 0;
  myFile = SD.open(path);
  if (myFile) {
    while (myFile.available()) {
      if (myFile.read() == '\n') {
        lines++;
      }
    }
    myFile.close();
  } else {
    Serial.println("error opening file for counting lines");
  }
  return lines;
}

void read_line(const char * path, int lineNumber) {
  int currentLine = 0;
  myFile = SD.open(path);
  if (myFile) {
    while (myFile.available()) {
      String line = myFile.readStringUntil('\n');
      if (currentLine == lineNumber) {
        Serial.println(line);
        break;
      }
      currentLine++;
    }
    myFile.close();
  } else {
    Serial.println("error opening file for reading line");
  }
}

void write_line(const char * path, const char * message, int lineNumber) {
  File tempFile = SD.open("/temp.txt", FILE_WRITE);
  myFile = SD.open(path);
  int currentLine = 0;
  if (myFile && tempFile) {
    while (myFile.available()) {
      String line = myFile.readStringUntil('\n');
      if (currentLine == lineNumber) {
        tempFile.println(message);
      } else {
        tempFile.println(line);
      }
      currentLine++;
    }
    if (currentLine <= lineNumber) {
      tempFile.println(message);
    }
    myFile.close();
    tempFile.close();
    SD.remove(path);
    SD.rename("/temp.txt", path);
  } else {
    Serial.println("error opening file for writing line");
  }
}

void insert_line(const char * path, const char * message) {
  myFile = SD.open(path, FILE_WRITE);
  if (myFile) {
    myFile.seek(myFile.size());
    myFile.print("\n");
    myFile.print(message);
    myFile.close();
  } else {
    Serial.println("error opening file for inserting line");
  }
}

void RemoveLine(const char * path, int lineNumber) {
  File tempFile = SD.open("/temp.txt", FILE_WRITE);
  myFile = SD.open(path);
  int currentLine = 0;
  if (myFile && tempFile) {
    while (myFile.available()) {
      String line = myFile.readStringUntil('\n');
      if (currentLine != lineNumber) {
        tempFile.println(line);
      }
      currentLine++;
    }
    myFile.close();
    tempFile.close();
    SD.remove(path);
    SD.rename("/temp.txt", path);
  } else {
    Serial.println("error opening file for removing line");
  }
}

void move_all_strings(const char * sourcePath, const char * destPath) {
  File sourceFile = SD.open(sourcePath);
  File destFile = SD.open(destPath, FILE_WRITE);
  if (sourceFile && destFile) {
    while (sourceFile.available()) {
      String line = sourceFile.readStringUntil('\n');
      destFile.println(line);
    }
    sourceFile.close();
    destFile.close();
    Serial.printf("All strings moved from %s to %s successfully.\n", sourcePath, destPath);
  } else {
    Serial.println("error opening files for moving strings");
  }
}

void RemoveFile(const char * path) {
  if (SD.exists(path)) {
    SD.remove(path);
    Serial.printf("File %s removed successfully.\n", path);
  } else {
    Serial.printf("File %s does not exist.\n", path);
  }
}

void setup() {
  Serial.begin(2000000);
  delay(500);
  while (!Serial) { ; }
  Serial.println("Initializing SD card...");
  if (!SD.begin(CS)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");

  write_file("/test.txt", "testing text");
  read_file("/test.txt");
  RemoveLine("/test.txt", 0);
  read_file("/test.txt");
  
  int lineCount = cout_line("/test.txt");
  Serial.printf("Number of lines: %d\n", lineCount);

  write_line("/test.txt", "new line text", lineCount + 1);
  write_line("/test.txt", "new line text", lineCount + 1);
  write_line("/test.txt", "new line text", lineCount + 1);
  read_file("/test.txt");

  read_line("/test.txt", 1);
}

void loop() {
}