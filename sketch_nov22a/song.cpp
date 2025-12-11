#include "HardwareSerial.h"
#include "song.h"
#include "melodies.h"

#include <stdlib.h>
#include <Arduino.h>

extern int* songs[];
extern int songTempos[];
extern int songLengths[];
extern int numSongs;

SongPlayer::SongPlayer(int buzzerPin, int buttonPin) {
    _buzzerPin = buzzerPin;
    _buttonPin = buttonPin;
}

void SongPlayer::playMelody(int* melody, int notes, int tempo) {
    int wholenote = (60000 * 4) / tempo;
    int divider = 0, noteDuration = 0;

    for (int thisNote = 0; thisNote < notes * 2; thisNote = thisNote + 2) {
        if (digitalRead(_buttonPin) == HIGH) {
            Serial.println("Stopped by button!");
            noTone(_buzzerPin);
            return;
        }

        divider = melody[thisNote + 1];
        
        if (divider > 0) {
            noteDuration = (wholenote) / divider;
        } else if (divider < 0) {
            noteDuration = (wholenote) / abs(divider);
            noteDuration *= 1.5;
        }

        tone(_buzzerPin, melody[thisNote], noteDuration * 0.9);

        unsigned long startNoteTime = millis();
        while (millis() - startNoteTime < noteDuration) {
            if (digitalRead(_buttonPin) == HIGH) {
                Serial.println("Stopped by button!");
                noTone(_buzzerPin);
                return;
            }
        }
        
        noTone(_buzzerPin);
    }
}


void SongPlayer::playSong(int songIndex) {
    if ((songIndex < 0) || (songIndex > numSongs - 1)) return;

    int* melody = songs[songIndex];

    int songLength = songLengths[songIndex];
    int notesCount = songLength / 2;

    int tempo = songTempos[songIndex];

    playMelody(melody, notesCount, tempo);
}