#include "util.h"

#include <GL/glfw.h>
#include <png.h>
#include <map>

#include <sys/time.h>
#include <iostream>

static std::map<std::string, GLuint> textures;
static time_t lastUpdate = 0;

GLuint loadTexture(const std::string &name, GLuint texture) {
  if (name == "") return 0;
 
  if (texture == 0) { 
    auto iter = textures.find(name);
    if (iter != textures.end()) {
      return iter->second;
    }
    glGenTextures(1, &texture);
    lastUpdate = time(0);
  }

  unsigned long w,h;
  FILE *fp = openAsset(name+".png");
  if (!fp) return 0;
  
  // read the header
  png_byte header[8];
  int res = fread(header, 1, 8, fp);
  if (res != 8 || png_sig_cmp(header, 0, 8))
  {
    fclose(fp);
    return 0;
  }

  png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png_ptr)
  {
    fclose(fp);
    return 0;
  }

  // create png info struct
  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr)
  {
    fprintf(stderr, "error: png_create_info_struct returned 0.\n");
    png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
    fclose(fp);
    return 0;
  }

  // create png info struct
  png_infop end_info = png_create_info_struct(png_ptr);
  if (!end_info)
  {
    png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
    fclose(fp);
    return 0;
  }

  // the code in this if statement gets called if libpng encounters an error
  if (setjmp(png_jmpbuf(png_ptr))) {
      png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
      fclose(fp);
      return 0;
  }

  // init png reading
  png_init_io(png_ptr, fp);

  // let libpng know you already read the first 8 bytes
  png_set_sig_bytes(png_ptr, 8);

  // read all the info up to the image data
  png_read_info(png_ptr, info_ptr);

  // variables to pass to get info
  int bit_depth, color_type;

  // get info about png
  png_get_IHDR(png_ptr, info_ptr, &w, &h, &bit_depth, &color_type, NULL, NULL, NULL);

  fprintf(stderr, "%lu %lu %d % d\n", w, h, bit_depth, color_type);

  // Update the png info struct.
  png_read_update_info(png_ptr, info_ptr);

  // Row size in bytes.
  int rowbytes = png_get_rowbytes(png_ptr, info_ptr);

  // glTexImage2d requires rows to be 4-byte aligned
  rowbytes += 3 - ((rowbytes-1) % 4);

  // Allocate the image_data as a big block, to be given to opengl
  png_byte * image_data = new png_byte[rowbytes * h +15];

  // row_pointers is for pointing to image_data for reading the png with libpng
  png_bytep * row_pointers = new png_bytep[h];
  
  // set the individual row_pointers to point at the correct offsets of image_data
  unsigned int i;
  for (i = 0; i < h; i++)
  {
    row_pointers[h - 1 - i] = image_data + i * rowbytes;
  }

  // read the png into image_data through row_pointers
  png_read_image(png_ptr, row_pointers);

  // Generate the OpenGL texture object
  glBindTexture(GL_TEXTURE_2D, texture);
  if (color_type == 6) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
  } else if (color_type == 3) {
    png_colorp palette;
    int num_palette;
    if (png_get_PLTE(png_ptr, info_ptr, &palette, &num_palette)) {
      uint8_t *data = new uint8_t[w*h*4];
      for (size_t i=0; i<w*h; i++) {
        data[i*4+0] = palette[image_data[i]].red;
        data[i*4+1] = palette[image_data[i]].green;
        data[i*4+2] = palette[image_data[i]].blue;
        data[i*4+3] = image_data[i]?255:0;
      }
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
      delete [] data;
    }
  } else if (color_type == 2) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, image_data);
  }
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  // clean up
  png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
  delete [] image_data;
  delete [] row_pointers;
  fclose(fp);

  textures[name] = texture;
  return texture;
}

void updateTextures() {
  time_t lastMod = lastUpdate;
  for (auto t : textures) {
    time_t mtime = getFileChangeTime(t.first+".png"); 
    if (mtime > lastUpdate) {
      GLuint res = loadTexture(t.first, t.second);
      if (res != 0 && mtime > lastMod) lastMod = mtime;
    }
  }
  lastUpdate = lastMod;
}

