#include "common.h"

#include "image.h"

#include "fileio.h"
#include "simplex.h"

#include <png.h>

Image::Image() :
  hasAlpha(false),
  rgba(nullptr),
  size(0,0)
{
}

Image::Image(const Point &size, uint8_t *data, bool alpha) :
  hasAlpha(alpha),
  rgba(data),
  size(size)
{
}

Image::Image(Image &&rhs) :
  hasAlpha(rhs.hasAlpha),
  rgba(rhs.rgba),
  size(rhs.size)
{
  rhs.rgba = nullptr;
  rhs.size = Point(0,0);
}

Image::~Image() {
  delete [] this->rgba;
}

Image Image::Noise(const Point &size, const Vector3 &scale, const Vector3 &offset) {
  uint8_t *image_data = new uint8_t[size.x*size.y*4];
  for (int y=0; y<size.y; y++) {
    for (int x=0; x<size.x; x++) {
      Vector3 pr = Vector3( x*scale.x/size.x, y*scale.y/size.y, 0 ) + offset;
      Vector3 pg = Vector3( x*scale.x/size.x, y*scale.y/size.y, 1 ) + offset;
      Vector3 pb = Vector3( x*scale.x/size.x, y*scale.y/size.y,-1 ) + offset;
      Vector3 pa = Vector3( x*scale.x/size.x, y*scale.y/size.y,-2 ) + offset;

      image_data[(x+y*size.x)*4+0] = (simplexNoise(pr)*0.5+0.5)*255;
      image_data[(x+y*size.x)*4+1] = (simplexNoise(pg)*0.5+0.5)*255;
      image_data[(x+y*size.x)*4+2] = (simplexNoise(pb)*0.5+0.5)*255;
      image_data[(x+y*size.x)*4+3] = (simplexNoise(pa)*0.5+0.5)*255;
    }
  }
  
  return Image(size, image_data, true);
}
  
Image Image::Load(const std::string &name) {
  if (name == "") return Image();
  
  Log("Loading image %s\n", name.c_str());

  FILE *fp = openAsset(name+".png");
  if (!fp) {
    perror(name.c_str());
    return Image();
  }

  png_byte header[8];
  png_structp png_ptr = nullptr;
  png_infop info_ptr = nullptr;
  png_infop end_info = nullptr;

  png_byte * image_data = nullptr;
  png_bytep * row_pointers = nullptr;
  
  int rowbytes = 0;

  bool hasAlpha = false;
  uint8_t *rgba = nullptr;
  
  // read the header
  int res = fread(header, 1, 8, fp);
  if (res != 8 || png_sig_cmp(header, 0, 8)) {
    Log("Error: could not read png header.\n");
    goto error;
  }

  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png_ptr) {
    Log("Error: png_create_read_struct returned 0.\n");
    goto error;
  }

  // create png info struct
  info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    Log("Error: png_create_info_struct returned 0.\n");
    goto error;
  }

  // create png info struct
  end_info = png_create_info_struct(png_ptr);
  if (!end_info) {
    Log("Error: png_create_info_struct returned 0.\n");
    goto error;
  }

  // the code in this if statement gets called if libpng encounters an error
  if (setjmp(png_jmpbuf(png_ptr))) {
    Log("Error: Could not read image.\n");
    goto error;
  }

  // init png reading
  png_init_io(png_ptr, fp);

  // let libpng know you already read the first 8 bytes
  png_set_sig_bytes(png_ptr, 8);

  // read all the info up to the image data
  png_read_info(png_ptr, info_ptr);

  // get info about png
  int bit_depth, color_type;
  png_uint_32 w, h;
  
  png_get_IHDR(png_ptr, info_ptr, &w, &h, &bit_depth, &color_type, NULL, NULL, NULL);

  // Update the png info struct.
  png_read_update_info(png_ptr, info_ptr);

  // Row size in bytes.
  rowbytes = png_get_rowbytes(png_ptr, info_ptr);

  // glTexImage2d requires rows to be 4-byte aligned
  rowbytes += 3 - ((rowbytes-1) % 4);

  // Allocate the image_data as a big block, to be given to opengl
  image_data = new png_byte[rowbytes * h +15];

  // row_pointers is for pointing to image_data for reading the png with libpng
  row_pointers = new png_bytep[h];

  // set the individual row_pointers to point at the correct offsets of image_data
  for (png_uint_32 i = 0; i < h; i++)
    row_pointers[h - 1 - i] = image_data + i * rowbytes;

  // read the png into image_data through row_pointers
  png_read_image(png_ptr, row_pointers);

  // convert image to rgba
  if (color_type == 6) {
    hasAlpha = true;
    rgba     = new uint8_t[w*h*4];
    ::memcpy(rgba, image_data, w*h*4);
  } else if (color_type == 3) {
    hasAlpha = true;
    rgba     = new uint8_t[w*h*4];
    
    png_colorp palette;
    int num_palette;
    
    if (png_get_PLTE(png_ptr, info_ptr, &palette, &num_palette)) {
      for (size_t i=0; i<w*h; i++) {
        rgba[i*4+0] = image_data[i]?palette[image_data[i]].red:0;
        rgba[i*4+1] = image_data[i]?palette[image_data[i]].green:0;
        rgba[i*4+2] = image_data[i]?palette[image_data[i]].blue:0;
        rgba[i*4+3] = image_data[i]?255:0;
      }
    }
  } else if (color_type == 2) {
    hasAlpha = false;
    rgba     = new uint8_t[w*h*3];
    ::memcpy(rgba, image_data, w*h*3);
  } else {
    hasAlpha = false;
    rgba     = new uint8_t[w*h*3];
    Log("Unknown color_type: %u", color_type);
  }

  // clean up
  png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
  fclose(fp);
  
  delete [] image_data;
  delete [] row_pointers;
  
  return Image(Point(w,h), rgba, hasAlpha);
  
error:

  png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
  fclose(fp);
  return Image();
}

void 
Image::Save(const std::string &fileName) {
  png_structp png_ptr       = nullptr;
  png_infop   info_ptr      = nullptr;
  png_byte ** row_pointers  = nullptr;

  FILE *fp = fopen (fileName.c_str(), "wb");
  if (! fp) {
    perror(fileName.c_str());
    goto fopen_failed;
  }

  png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (png_ptr == NULL) {
    Log("png_create_write_struct failed\n");
    goto png_create_write_struct_failed;
  }

  info_ptr = png_create_info_struct (png_ptr);
  if (info_ptr == NULL) {
    Log("png_create_info_struct failed\n");
    goto png_create_info_struct_failed;
  }

  /* Set up error handling. */
  if (setjmp (png_jmpbuf (png_ptr))) {
    goto png_failure;
  }

  /* Set image attributes. */

  png_set_IHDR (png_ptr, info_ptr,
                size.x,  size.y,   8,
                this->hasAlpha ? PNG_COLOR_TYPE_RGB_ALPHA : PNG_COLOR_TYPE_RGB,
                PNG_INTERLACE_NONE,
                PNG_COMPRESSION_TYPE_DEFAULT,
                PNG_FILTER_TYPE_DEFAULT);

  /* Initialize rows of PNG. */

  row_pointers = (png_byte**)png_malloc (png_ptr, size.y * sizeof (png_byte *));
  for (int y = 0; y < size.y; ++y) {
    int pixel_size = this->hasAlpha ? 4 : 3;
    
    png_byte *row = (png_byte*)png_malloc (png_ptr, sizeof (uint8_t) * size.x * pixel_size);
    row_pointers[size.y-y-1] = row;
    
    for (int x = 0; x < size.x; ++x) {
      *row++ = rgba[(x+y*size.x)*pixel_size+0];
      *row++ = rgba[(x+y*size.x)*pixel_size+1];
      *row++ = rgba[(x+y*size.x)*pixel_size+2];
      if (this->hasAlpha) *row++ = rgba[(x+y*size.x)*pixel_size+3];
    }
  }

  /* Write the image data to "fp". */

  png_init_io (png_ptr, fp);
  png_set_rows (png_ptr, info_ptr, row_pointers);
  png_write_png (png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

  for (int y = 0; y < size.y; y++) 
    png_free (png_ptr, row_pointers[y]);

  png_free (png_ptr, row_pointers);
  
png_failure:
png_create_info_struct_failed:

  png_destroy_write_struct (&png_ptr, &info_ptr);

png_create_write_struct_failed:

  fclose (fp);

fopen_failed:
  ;
}

