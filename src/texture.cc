#include "util.h"

#include "GLee.h"
#include <png.h>
#include <map>

#include <sys/time.h>
#include <iostream>
#include "simplex.h"

static std::map<std::string, std::unique_ptr<Texture>> textures;
static time_t lastUpdate = 0;

Texture::Texture() {
  handle = 0;
}

Texture::~Texture() {
  if (handle) glDeleteTextures(1, &handle);
}

Texture::Texture(Texture &&rhs) {
  this->size = rhs.size;
  this->handle = rhs.handle;
  
  rhs.handle = 0;
  rhs.size = Point();
}

const Texture *loadTexture(const std::string &name, const Texture * tex = nullptr) {
  if (name == "") return nullptr;
  
  GLuint textureHandle = 0;
  if (tex) {
    textureHandle = tex->handle;
  } else {
    auto iter = textures.find(name);
    if (iter != textures.end()) {
      return iter->second.get();
    }
    glGenTextures(1, &textureHandle);
    lastUpdate = time(0);
    
    textures[name] = std::unique_ptr<Texture>(new Texture());
    textures[name]->handle = textureHandle;
    
    std::cerr << "loading texture " << name << " as " << textureHandle << std::endl;
  }

  unsigned long w,h;
  FILE *fp = openAsset(name+".png");
  if (!fp) return nullptr;
  
  // read the header
  png_byte header[8];
  int res = fread(header, 1, 8, fp);
  if (res != 8 || png_sig_cmp(header, 0, 8))
  {
    fclose(fp);
    return nullptr;
  }

  png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png_ptr)
  {
    std::cerr << "Error: png_create_read_struct returned 0.\n";
    fclose(fp);
    return nullptr;
  }

  // create png info struct
  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr)
  {
    std::cerr << "Error: png_create_info_struct returned 0.\n";
    png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
    fclose(fp);
    return nullptr;
  }

  // create png info struct
  png_infop end_info = png_create_info_struct(png_ptr);
  if (!end_info)
  {
    std::cerr << "Error: png_create_info_struct returned 0.\n";
    png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
    fclose(fp);
    return nullptr;
  }

  // the code in this if statement gets called if libpng encounters an error
  if (setjmp(png_jmpbuf(png_ptr))) {
    std::cerr << "Error: Could not read image.\n";
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    fclose(fp);
    return nullptr;
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
  glBindTexture(GL_TEXTURE_2D, textureHandle);
  if (color_type == 6) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
  } else if (color_type == 3) {
    png_colorp palette;
    int num_palette;
    if (png_get_PLTE(png_ptr, info_ptr, &palette, &num_palette)) {
      uint8_t *data = new uint8_t[w*h*4];
      for (size_t i=0; i<w*h; i++) {
        data[i*4+0] = image_data[i]?palette[image_data[i]].red:0;
        data[i*4+1] = image_data[i]?palette[image_data[i]].green:0;
        data[i*4+2] = image_data[i]?palette[image_data[i]].blue:0;
        data[i*4+3] = image_data[i]?255:0;
      }
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
      delete [] data;
    }
  } else if (color_type == 2) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, image_data);
  }
  glGenerateMipmapEXT(GL_TEXTURE_2D);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  // clean up
  png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
  delete [] image_data;
  delete [] row_pointers;
  fclose(fp);

  textures[name]->size = Point(w,h);
  return textures[name].get();
}

void updateTextures() {
  time_t lastMod = lastUpdate;
  for (auto &t : textures) {
    if (t.first[0] == '*') continue;
    time_t mtime = getFileChangeTime(t.first+".png"); 
    if (mtime > lastUpdate) {
      const Texture *res = loadTexture(t.first, t.second.get());
      if (res && mtime > lastMod) lastMod = mtime;
    }
  }
  lastUpdate = lastMod;
}

const Texture * 
noiseTexture(const Point &size, const Vector3 &scale = Vector3(1,1,1), const Vector3 &offset = Vector3()) {
  float *image_data = new float[size.x*size.y*4];
  for (int y=0; y<size.y; y++) {
    for (int x=0; x<size.x; x++) {
      Vector3 pr = Vector3( x*scale.x/size.x, y*scale.y/size.y, 0 ) + offset;
      Vector3 pg = Vector3( x*scale.x/size.x, y*scale.y/size.y, 1 ) + offset;
      Vector3 pb = Vector3( x*scale.x/size.x, y*scale.y/size.y,-1 ) + offset;
      
      image_data[(x+y*size.x)*4+0] = simplexNoise(pr)*0.5+0.5;
      image_data[(x+y*size.x)*4+1] = simplexNoise(pg)*0.5+0.5;
      image_data[(x+y*size.x)*4+2] = simplexNoise(pb)*0.5+0.5;
      image_data[(x+y*size.x)*4+3] = 1;
    }
  }
  
  unsigned int texture = 0;  
  glGenTextures(1, &texture);
  
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_FLOAT, image_data);
  glGenerateMipmapEXT(GL_TEXTURE_2D);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  delete [] image_data;
  
  std::stringstream str;
  str << "*" << size << scale << offset;
  
  std::string name = str.str();
  
  std::cerr << "generated noise texture " << name << " as " << texture << std::endl;
  
  textures[name] = std::unique_ptr<Texture>(new Texture());
  textures[name]->handle = texture;
  textures[name]->size = size;
  
  return textures[name].get();
}

int saveImage(const std::string &fileName, size_t w, size_t h, const uint8_t *rgb) {
  FILE *fp;
  png_structp png_ptr = NULL;
  png_infop info_ptr = NULL;
  size_t x, y;
  png_byte ** row_pointers = NULL;
  
  int status = -1;
  int pixel_size = 3;
  int depth = 8;
    
  fp = fopen (fileName.c_str(), "wb");
  if (! fp) {
    goto fopen_failed;
  }
  
  png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (png_ptr == NULL) {
    goto png_create_write_struct_failed;
  }
    
  info_ptr = png_create_info_struct (png_ptr);
  if (info_ptr == NULL) {
     goto png_create_info_struct_failed;
  }
    
  /* Set up error handling. */
  if (setjmp (png_jmpbuf (png_ptr))) {
    goto png_failure;
  }
    
  /* Set image attributes. */

  png_set_IHDR (png_ptr,
                info_ptr,
                w,
                h,
                depth,
                PNG_COLOR_TYPE_RGB,
                PNG_INTERLACE_NONE,
                PNG_COMPRESSION_TYPE_DEFAULT,
                PNG_FILTER_TYPE_DEFAULT);
    
  /* Initialize rows of PNG. */

  row_pointers = (png_byte**)png_malloc (png_ptr, h * sizeof (png_byte *));
  for (y = 0; y < h; ++y) {
    png_byte *row = (png_byte*)png_malloc (png_ptr, sizeof (uint8_t) * w * pixel_size);
    row_pointers[h-y-1] = row;
    for (x = 0; x < w; ++x) {
      *row++ = rgb[(x+y*w)*3+0];
      *row++ = rgb[(x+y*w)*3+1];
      *row++ = rgb[(x+y*w)*3+2];
    }
  }
  
  /* Write the image data to "fp". */

  png_init_io (png_ptr, fp);
  png_set_rows (png_ptr, info_ptr, row_pointers);
  png_write_png (png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

  /* The routine has successfully written the file, so we set
     "status" to a value which indicates success. */

  status = 0;
    
  for (y = 0; y < h; y++) {
    png_free (png_ptr, row_pointers[y]);
  }
  png_free (png_ptr, row_pointers);
    
png_failure:
png_create_info_struct_failed:

  png_destroy_write_struct (&png_ptr, &info_ptr);
    
png_create_write_struct_failed:

  fclose (fp);
  
fopen_failed:
    return status;
}
