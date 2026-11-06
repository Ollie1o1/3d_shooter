// Standalone soundtrack regression checks: no SDL or graphics context.
#include "audio/MusicSynth.h"
#include <cassert>
#include <cstdio>
int main() {
  const auto& menu = musicTrack(MENU_MUSIC_TRACK);
  assert(menu.root <= 45 && menu.bpm >= 140.f && menu.techno);
  for (int chord : menu.chords) assert(chord == 0);
  assert(!std::strchr(menu.bass, 'o') && !std::strchr(menu.bass, 'f'));
  MusicSynth m; std::vector<float> b(8820);
  for (int rate : {44100,48000}) {
    m.setSampleRate((float)rate);
    for (int t=0;t<MUSIC_TRACKS;++t) {
      m.setTrack(t);m.setIntensity(1.3f);m.setDrive(true);m.stinger();
      m.render(b.data(),4410); assert(m.currentTrack()==t);
      if(t != MENU_MUSIC_TRACK) m.rewind();m.render(b.data(),4410);
      m.setDrive(false);m.setSolo(false);m.setIntensity(.55f);m.setTrack(MENU_MUSIC_TRACK);
      m.render(b.data(),4410);
      assert(m.currentTrack()==MENU_MUSIC_TRACK && !m.rewinding());
      for(float v:b) assert(std::isfinite(v) && std::fabs(v)<=1.f);
      for(int k=0;k<130;++k) m.render(b.data(),4410);
      assert(m.currentTrack()==MENU_MUSIC_TRACK);
    }
  }
  std::puts("All 18 tracks: menu transitions, rewind cancellation, and sustained playback passed at 44.1/48 kHz.");
}
