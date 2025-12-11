#ifndef SONG_H
#define SONG_H

typedef bool (*StopFunction)();

class SongPlayer {
private:
  int _buzzerPin;
  int _buttonPin;

public:
  SongPlayer(int buzzerPin, int buttonPin);
  void playSong(int songIndex);
  void playMelody(int* melody, int notes, int tempo);
};

#endif // SONG_H