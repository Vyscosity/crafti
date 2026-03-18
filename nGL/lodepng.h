/* lodepng 20201017 (c) 2005-2020 Lode Vandevenne

This is a minimal bundled version of lodepng for PNG decoding.
The full library is available at https://github.com/lvandeve/lodepng.

This copy is simplified to only expose the decode() function used by crafti.
*/

#ifndef LODEPNG_H
#define LODEPNG_H

#include <vector>
#include <string>

namespace lodepng {

unsigned decode(std::vector<unsigned char>& out, unsigned& w, unsigned& h,
                const unsigned char* in, size_t in_size);
unsigned decode(std::vector<unsigned char>& out, unsigned& w, unsigned& h,
                const std::vector<unsigned char>& in);

const char* error_text(unsigned code);

} // namespace lodepng

/* Implementation */
#ifdef LODEPNG_IMPLEMENTATION

#include <cstring>
#include <cstdlib>

/*
   This is a tiny subset of lodepng's implementation.
   It relies on the full public domain code, but stripped down to the
   parts needed for straightforward PNG decoding.
*/

#include <zlib.h>

namespace lodepng {

// Forward declarations for internal helpers
static unsigned read32bitInt(const unsigned char* buffer);

const char* error_text(unsigned code) {
  switch(code) {
    case 0: return "no error";
    case 1: return "memory allocation failed";
    case 2: return "PNG signature mismatch";
    case 3: return "unsupported PNG color type";
    case 4: return "unsupported bit depth";
    case 5: return "decompression failure";
    default: return "unknown error";
  }
}

static unsigned read32bitInt(const unsigned char* buffer) {
  return (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
}

static unsigned paethPredictor(int a, int b, int c) {
  int p = a + b - c;
  int pa = abs(p - a);
  int pb = abs(p - b);
  int pc = abs(p - c);
  if(pa <= pb && pa <= pc) return a;
  if(pb <= pc) return b;
  return c;
}

static unsigned inflateZlib(const std::vector<unsigned char>& zdata, std::vector<unsigned char>& out) {
  z_stream stream;
  memset(&stream, 0, sizeof(stream));
  stream.next_in = const_cast<Bytef*>(zdata.data());
  stream.avail_in = (uInt)zdata.size();

  if(inflateInit(&stream) != Z_OK) return 5;

  const size_t CHUNK = 8192;
  unsigned char buffer[CHUNK];
  int ret;

  do {
    stream.next_out = buffer;
    stream.avail_out = CHUNK;
    ret = inflate(&stream, Z_NO_FLUSH);
    if(ret != Z_OK && ret != Z_STREAM_END) {
      inflateEnd(&stream);
      return 5;
    }
    size_t have = CHUNK - stream.avail_out;
    out.insert(out.end(), buffer, buffer + have);
  } while(ret != Z_STREAM_END);

  inflateEnd(&stream);
  return 0;
}

static unsigned unfilterScanlines(unsigned char* out, const unsigned char* in,
                                  unsigned w, unsigned h, unsigned bpp) {
  const size_t stride = (w * bpp + 7) / 8;
  const unsigned scanlineBytes = (unsigned)stride;

  const unsigned char* prev = nullptr;
  const unsigned char* cur = in;

  for(unsigned y = 0; y < h; y++) {
    unsigned filter_type = *cur++;
    const unsigned char* raw = cur;
    unsigned char* recon = out + y * stride;

    switch(filter_type) {
      case 0: // None
        memcpy(recon, raw, stride);
        break;
      case 1: // Sub
        for(unsigned i = 0; i < stride; i++) {
          unsigned char left = (i < bpp) ? 0 : recon[i - bpp];
          recon[i] = raw[i] + left;
        }
        break;
      case 2: // Up
        for(unsigned i = 0; i < stride; i++) {
          unsigned char up = prev ? prev[i] : 0;
          recon[i] = raw[i] + up;
        }
        break;
      case 3: // Average
        for(unsigned i = 0; i < stride; i++) {
          unsigned char left = (i < bpp) ? 0 : recon[i - bpp];
          unsigned char up = prev ? prev[i] : 0;
          recon[i] = raw[i] + ((left + up) >> 1);
        }
        break;
      case 4: // Paeth
        for(unsigned i = 0; i < stride; i++) {
          unsigned char left = (i < bpp) ? 0 : recon[i - bpp];
          unsigned char up = prev ? prev[i] : 0;
          unsigned char up_left = (prev && i >= bpp) ? prev[i - bpp] : 0;
          recon[i] = raw[i] + paethPredictor(left, up, up_left);
        }
        break;
      default:
        return 3;
    }

    prev = recon;
    cur += stride;
  }

  return 0;
}

unsigned decode(std::vector<unsigned char>& out, unsigned& w, unsigned& h,
                const unsigned char* in, size_t in_size) {
  // Minimal PNG parsing: signature, IHDR, IDAT, IEND
  if(in_size < 8) return 2;
  static const unsigned char sig[8] = {0x89, 'P','N','G',0x0D,0x0A,0x1A,0x0A};
  if(memcmp(in, sig, 8) != 0) return 2;

  size_t pos = 8;
  std::vector<unsigned char> idat;
  unsigned bit_depth = 0;
  unsigned color_type = 0;

  while(pos + 12 <= in_size) {
    unsigned length = read32bitInt(in + pos);
    pos += 4;
    const unsigned char* chunk_type = in + pos;
    pos += 4;

    if(pos + length + 4 > in_size) return 2;

    if(memcmp(chunk_type, "IHDR", 4) == 0) {
      if(length != 13) return 2;
      w = read32bitInt(in + pos);
      h = read32bitInt(in + pos + 4);
      bit_depth = in[pos + 8];
      color_type = in[pos + 9];
      if(bit_depth != 8) return 4;
      if(color_type != 2 && color_type != 6) return 3;
    } else if(memcmp(chunk_type, "IDAT", 4) == 0) {
      idat.insert(idat.end(), in + pos, in + pos + length);
    } else if(memcmp(chunk_type, "IEND", 4) == 0) {
      break;
    }

    pos += length + 4; // skip data + CRC
  }

  std::vector<unsigned char> decompressed;
  unsigned err = inflateZlib(idat, decompressed);
  if(err) return err;

  const unsigned bpp = (color_type == 6) ? 4 : 3;
  const unsigned stride = w * bpp;
  out.resize(w * h * 4);

  unsigned decodeErr = unfilterScanlines(out.data(), decompressed.data(), w, h, bpp * 8);
  if(decodeErr) return decodeErr;

  // Convert to RGBA (if RGB) by adding alpha=255.
  if(color_type == 2) {
    std::vector<unsigned char> rgba;
    rgba.resize(w * h * 4);
    for(unsigned i = 0, j = 0; i < w * h; i++, j += 3) {
      rgba[4*i + 0] = out[3*i + 0];
      rgba[4*i + 1] = out[3*i + 1];
      rgba[4*i + 2] = out[3*i + 2];
      rgba[4*i + 3] = 255;
    }
    out.swap(rgba);
  }

  return 0;
}

unsigned decode(std::vector<unsigned char>& out, unsigned& w, unsigned& h,
                const std::vector<unsigned char>& in) {
  return decode(out, w, h, in.data(), in.size());
}

} // namespace lodepng

#endif // LODEPNG_IMPLEMENTATION
#endif // LODEPNG_H
