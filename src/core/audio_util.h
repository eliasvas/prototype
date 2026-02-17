#ifndef AUDIO_UTIL_H__
#define AUDIO_UTIL_H__
#include "base/base_inc.h"

// Some references for when I ACTUALLY wanna do audio
// https://blog.demofox.org/2012/05/14/diy-synthesizer-chapter-1-sound-output/
// https://lisyarus.github.io/blog/posts/audio-mixing.html
// https://www.youtube.com/watch?v=udbA7u1zYfc (WAV spec)

// Mixer
// https://www.youtube.com/watch?v=UuqcgQxpfO8&list=PLEMXAbCVnmY4UakJTODzmY-6rSPKkuLr5 (Handmade Hero audio)
// https://ruby0x1.github.io/machinery_bloa_archive/post/writing-a-low-level-sound-system/index.html

// TODO: WAV loading
// TODO: Audio subsystem (based on SDL_Audio ok?)

typedef struct {
  // RIFF chunk
  u8 chunk_id[4];
  u32 chunk_size;
  u8 format[4];

  // fmt sub-chunk
  u8 subchunk1_id[4];
  u32 subchunk1_size;
  u16 audio_format;
  u16 num_channels;
  u32 sample_rate;
  u32 byte_rate;
  u16 block_align;
  u16 bits_per_sample;

  // data sub-chunk
  u8 subchunk2_id[4];
  u32 subchunk2_size;

  // Then just fwrite the WAV data!
} Wav_Header;

// TODO: Can we wrap the fwrite/FILE* stuff away? I don't like em.

static b32 write_sample_wav_file(const char *filename, void *data, u32 data_size_in_bytes, 
    u16 num_channels, u32 sample_rate, u32 bits_per_sample) {
  FILE *file = fopen(filename, "w+b");
  if (!file) {
    return false;
  }
  Wav_Header wheader = (Wav_Header) {
    .chunk_id = {0x52, 0x49, 0x46, 0x46}, // "RIFF"
    .chunk_size = data_size_in_bytes + 36,
    .format = {0x57, 0x41, 0x56, 0x45}, // "WAVE"
    .subchunk1_id = {0x66, 0x6D, 0x74, 0x20}, // "fmt "
    .subchunk1_size = 16,
    .audio_format = 1,
    .num_channels = num_channels,
    .sample_rate = sample_rate,
    .byte_rate = sample_rate * num_channels * bits_per_sample / 8,
    .block_align = num_channels * bits_per_sample / 8,
    .bits_per_sample = bits_per_sample,
    .subchunk2_id = {0x64, 0x61, 0x74, 0x61}, // "data"
    .subchunk2_size = data_size_in_bytes,
  };
  fwrite(&wheader,sizeof(Wav_Header), 1, file);
  fwrite(data, data_size_in_bytes, 1, file);

  fclose(file);
  return true;
}


#if 0
  // Audio test
  u32 sample_rate = 44100; 
  u32 num_seconds = 4;
  u32 num_channels = 2;

  u32 num_samples = sample_rate * num_channels * num_seconds;
  s32* wdata = arena_push_array(gs->frame_arena, s32, num_samples);

  s32 sample_value1 = 0;
  s32 sample_value2 = 0;
  for (u32 i = 0; i < num_samples; i+=2) {
    sample_value1 += 8000000;
    sample_value2 += 1200000;
    wdata[i] = sample_value1;
    wdata[i+1] = sample_value2;
  }

  assert(write_sample_wav_file("build/hello.wav", wdata, num_samples * sizeof(wdata[0]), num_channels, sample_rate, sizeof(wdata[0])*8)); 
#endif

#endif
