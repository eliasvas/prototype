#ifndef AUDIO_UTIL_H__
#define AUDIO_UTIL_H__
#include "base/base_inc.h"

// Some references for when I ACTUALLY wanna do audio
// https://blog.demofox.org/2012/05/14/diy-synthesizer-chapter-1-sound-output/
// https://lisyarus.github.io/blog/posts/audio-mixing.html
// https://www.youtube.com/watch?v=udbA7u1zYfc (WAV spec)

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
    .chunk_id = "RIFF",
    .chunk_size = data_size_in_bytes + 36,
    .format = "WAVE",
    .subchunk1_id = "fmt ",
    .subchunk1_size = 16,
    .audio_format = 1,
    .num_channels = num_channels,
    .sample_rate = sample_rate,
    .byte_rate = sample_rate * num_channels * bits_per_sample / 8,
    .block_align = num_channels * bits_per_sample / 8,
    .bits_per_sample = bits_per_sample,
    .subchunk2_id = "data",
    .subchunk2_size = data_size_in_bytes,
  };
  fwrite(&wheader,sizeof(Wav_Header), 1, file);
  fwrite(data, data_size_in_bytes, 1, file);

  fclose(file);
  return true;
}


#endif
