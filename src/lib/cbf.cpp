// NYU Depth V2 Dataset Matlab Toolbox
// Authors: Nathan Silberman, Pushmeet Kohli, Derek Hoiem, Rob Fergus

#include "cbf.h"

#include <fstream> // TODO: remove this.
#include <iostream>
#include <stdlib.h>
#include <math.h>

// Uncomment this define for intermediate filtering results.
// #define DEBUG

#define PI 3.14159

#define UCHAR_MAX 255
#define FILTER_RAD 5

void toc(const char* message, clock_t start) {
	
#ifdef DEBUG
	double d = clock() - start;
	d = 1000 * d / CLOCKS_PER_SEC;
	printf("[%s] %10.0f\n", message, d);
#endif
}

// Args:
//   filter_size - the number of pixels in the filter.
void create_offset_array(int filter_rad, int* offsets_h, int img_height) {
  int filter_len = filter_rad * 2 + 1;
	int filter_size = filter_len * filter_len;
	
	int kk = 0;
	for (int yy = -filter_rad; yy <= filter_rad; ++yy) {
		for (int xx = -filter_rad; xx <= filter_rad; ++xx, ++kk) {
			offsets_h[kk] = yy + img_height * xx;
		}
	}
}

void calc_pyr_sizes(int* heights, int* widths, int* pyr_offsets, int orig_height, int orig_width, int num_scales) {
  int offset = 0;
  for (int scale = 0; scale < num_scales; ++scale) {
	  pyr_offsets[scale] = offset;
		
    // Calculate the size of the downsampled images.
    heights[scale] = static_cast<int>(orig_height / pow((float)2, scale));
    widths[scale] = static_cast<int>(orig_width / pow((float)2, scale));
		
		offset += heights[scale] * widths[scale];
	}
	
#ifdef DEBUG
  for (int ii = 0; ii < num_scales; ++ii) {
	  printf("Scale %d: [%d x %d], offset=%d\n", ii, heights[ii], widths[ii], pyr_offsets[ii]); 
	}
#endif
}

int get_pyr_size(int* heights, int* widths, int num_scales) {
	
  int total_pixels = 0;
  for (int ii = 0; ii < num_scales; ++ii) {
		total_pixels += heights[ii] * widths[ii];
	}
	
	return total_pixels;
}

// We're upsampling from the result matrix (which is small) to the depth matrix,
// which is larger.
// 
// For example, dst could be 480x640 and src may be 240x320.
//
// Args:
//   depth_dst - H1xW1 matrix where H1 and W1 are equal to height_dst and
//               width_dst.
void upsample_cpu(float* depth_dst, 
									bool* mask_dst,
									bool* valid_dst,
                  float* depth_src,
									float* result_src, 
									bool* mask_src,
									bool* valid_src,
                  int height_src,
									int width_src,
								  int height_dst,
									int width_dst,
									int dst_img_ind) {
  
	int num_threads = height_dst * width_dst;
  
  // Dont bother if the upsampled one isnt missing.
	if (!mask_dst[dst_img_ind]) {
	  return;
	}
	
  int x_dst = floorf((float) dst_img_ind / height_dst);
  int y_dst = fmodf(dst_img_ind, height_dst);
  
	int y_src = static_cast<int>((float) y_dst * height_src / height_dst);
	int x_src = static_cast<int>((float) x_dst * width_src / width_dst);
  
	// Finally, convert to absolute coords.
  int src_img_ind = y_src + height_src * x_src;

  if (!mask_src[src_img_ind]) {
    depth_dst[dst_img_ind] = depth_src[src_img_ind];
  } else {
    depth_dst[dst_img_ind] = result_src[src_img_ind];
  }
	
	valid_dst[dst_img_ind] = valid_src[src_img_ind];
}

// Args:
//   depth - the depth image, a HxW vector
//   intensity - the intensity image, a HxW vector.
//   is_missing - a binary mask specifying whether each pixel is missing
//                (and needs to be filled in) or not.
//   valid_in - a mask specifying which of the input values are allowed
//              to be used for filtering.
//   valid_out - a mask specifying which of the output values are allowed
//               to be used for future filtering.
//   result - the result of the filtering operation, a HxW matrix.
//   abs_inds - the absolute indices (into depth, intensity, etc) which
//              need filtering.
//   offsets - vector of offsets from the current abs_ind to be used for
//             filtering.
//   guassian - the values (weights) of the gaussian filter corresponding
//              to the offset matrix.
void cbf_cpu(const float* depth, const float* intensity, bool* is_missing,
            bool* valid_in, bool* valid_out, float* result,
						const int* abs_inds,
						const int* offsets,
						const float* gaussian_space,
						int height,
						int width,
						int filter_rad,
						float sigma_s,
						float sigma_r,
						int numThreads,
						int idx) {
	
	int abs_ind = abs_inds[idx];
  
  int src_Y = abs_ind % height;
  int src_X = abs_ind / height;
	
	int filter_len = filter_rad * 2 + 1;
	int filter_size = filter_len * filter_len;
	
	float weight_sum = 0;
	float value_sum = 0;
	
	float weight_intensity_sum = 0;
  
	float gaussian_range[filter_size];
	float gaussian_range_sum = 0;
  
	for (int ii = 0; ii < filter_size; ++ii) {
    // Unfortunately we need to double check that the radii are correct
    // unless we add better processing of borders.
    
	  int abs_offset = abs_ind + offsets[ii]; // THESE ARE CALC TWICE.
    
    int dst_Y = abs_offset % height;
    int dst_X = abs_offset / height;
    
		if (abs_offset < 0 || abs_offset >= (height * width)
        || abs(src_Y-dst_Y) > FILTER_RAD || abs(src_X-dst_X) > FILTER_RAD) {
		  continue;
      
    // The offsets are into ANY part of the image. So they MAY be accessing
    // a pixel that was originally missing. However, if that pixel has been
    // filled in, then we can still use it.
		} else if (is_missing[abs_offset] && !valid_in[abs_offset]) {
			continue;
		}

    float vv = intensity[abs_offset] - intensity[abs_ind];


    gaussian_range[ii] = exp(-(vv * vv) / (2*sigma_r * sigma_r));
    gaussian_range_sum += gaussian_range[ii];
	}
  
  int count = 0;
  
	for (int ii = 0; ii < filter_size; ++ii) {
    // Get the Absolute offset into the image (1..N where N=H*W)
		int abs_offset = abs_ind + offsets[ii];
    int dst_Y = abs_offset % height;
    int dst_X = abs_offset / height;
		if (abs_offset < 0 || abs_offset >= (height * width)
        || abs(src_Y-dst_Y) > FILTER_RAD || abs(src_X-dst_X) > FILTER_RAD) {
		  continue;
		} else if (is_missing[abs_offset] && !valid_in[abs_offset]) {
		  continue;
		}
    
    ++count;
    
		weight_sum += gaussian_space[ii] * gaussian_range[ii];
		value_sum += depth[abs_offset] * gaussian_space[ii] * gaussian_range[ii];
	}
  
	if (weight_sum == 0) {
		return;
	}
  
  if (isnan(weight_sum)) {
    printf("*******************\n");
    printf(" Weight sum is NaN\n");
    printf("*******************\n");
  }

	value_sum /= weight_sum;
	
	result[abs_ind] = value_sum;
  
	valid_out[abs_ind] = 1;
}

// Args:
//   filter_size - the number of pixels in the filter.
void create_spatial_gaussian(int filter_rad, float sigma_s, float* gaussian_h) {
  int filter_len = filter_rad * 2 + 1;
	int filter_size = filter_len * filter_len;

	float sum = 0;
	int kk = 0;
	for (int yy = -filter_rad; yy <= filter_rad; ++yy) {
		for (int xx = -filter_rad; xx <= filter_rad; ++xx, ++kk) {
			gaussian_h[kk] = exp(-(xx*xx + yy*yy) / (2*sigma_s * sigma_s));
			sum += gaussian_h[kk];
		}
	}

	for (int ff = 0; ff < filter_size; ++ff) {
		gaussian_h[ff] /= sum;
	}
}

// Counts the number of missing pixels in the given mask. Note that the mask
// MUST already be in the appropriate offset location.
// 
// Args:
//   height - the heigh of the image at the current scale.
//   width - the width of the image at the current scale.
//   mask - pointer into the mask_ms_d matrix. The offset has already been
//          calculated.
//   abs_inds_h - pre-allocated GPU memory location.
int get_missing_pixel_coords(int height, int width, bool* mask, int* abs_inds_to_filter_h) {
  int num_pixels = height * width;
	
	int num_missing_pixels = 0;
	for (int nn = 0; nn < num_pixels; ++nn) {
    if (mask[nn]) {
	  	abs_inds_to_filter_h[num_missing_pixels] = nn;
			++num_missing_pixels;
		}
	}

	return num_missing_pixels;
}

static void savePGM(bool* imf, const char *name, int height, int width) {
	int NN = height * width;
	uint8_t im[NN];
	
	for (int nn = 0; nn < NN; ++nn) {
    // First convert to X,Y
    int y = nn % height;
    int x = floor(nn / height);
    
    // Then back to Abs Inds
    int mm = y * width + x;
    
		im[mm] = uint8_t(255*imf[nn]);
	}
	
 std::ofstream file(name, std::ios::out | std::ios::binary);
	
 file << "P5\n" << width << " " << height << "\n" << UCHAR_MAX << "\n";
 file.write((char *)&im, width * height * sizeof(uint8_t));
}

static void savePGM(float* imf, const char *name, int height, int width) {
	int NN = height * width;
	uint8_t im[NN];
	
	for (int nn = 0; nn < NN; ++nn) {
    // First convert to X,Y
    int y = nn % height;
    int x = floor(nn / height);
    
    // Then back to Abs Inds
    int mm = y * width + x;
    
		im[mm] = uint8_t(255*imf[nn]);
	}
	
 std::ofstream file(name, std::ios::out | std::ios::binary);
	
 file << "P5\n" << width << " " << height << "\n" << UCHAR_MAX << "\n";
 file.write((char *)&im, width * height * sizeof(uint8_t));
}

void filter_at_scale(float* depth_h,
										 float* intensity_h,
										 bool* mask_h,
										 bool* valid_h,
                     float* result_h,
										 int* abs_inds_to_filter_h, 
										 int height,
										 int width,
										 float sigma_s,
										 float sigma_r) {
	
  int filter_rad = FILTER_RAD;
	int filter_size = 2 * filter_rad + 1;
	int F = filter_size * filter_size;
	
  // Create the offset array.
	int* offsets_h = (int*) malloc(F * sizeof(int));
  create_offset_array(filter_rad, offsets_h, height);
	
	// Create the gaussian.
  float* gaussian_h = (float*) malloc(F * sizeof(float));
	create_spatial_gaussian(filter_rad, sigma_s, gaussian_h);
	
	// ************************************************
	// We need to be smart about how we do this, so rather
  // than execute the filter for EVERY point in the image,
  // we will only do it for the points missing depth information.
	// ************************************************
  
	int num_missing_pixels = get_missing_pixel_coords(height, width, mask_h, abs_inds_to_filter_h);
#ifdef DEBUG
  printf("Num Missing Pixels: %d\n", num_missing_pixels);
#endif
  
  clock_t start_filter = clock();	

  // We should not be writing into the same value for 'valid' that we're passing in.
  bool* valid_in = (bool*) malloc(height * width * sizeof(bool));
  for (int i = 0; i < height * width; ++i) {
    valid_in[i] = valid_h[i];
  }
  
  for (int i = 0; i < num_missing_pixels; ++i) {
		cbf_cpu(depth_h,
						intensity_h,
						mask_h,
            valid_in,
						valid_h,
						result_h,
						abs_inds_to_filter_h,
						offsets_h,
						gaussian_h,
						height,
						width,
						filter_rad,
						sigma_s,
						sigma_r,
						num_missing_pixels,
						i);
	}
  
	toc("FILTER OP", start_filter);
	
  free(valid_in);
	free(offsets_h);
	free(gaussian_h);
}

void cbf::cbf(int height, int width, uint8_t* depth, uint8_t* intensity,
              uint8_t* mask_h, uint8_t* result, unsigned num_scales,
              double* sigma_s, double* sigma_r) {
	
  clock_t start_func = clock();
	
	int pyr_heights[num_scales];
	int pyr_widths[num_scales];
	int pyr_offsets[num_scales];
	calc_pyr_sizes(&pyr_heights[0], &pyr_widths[0], &pyr_offsets[0], height, width, num_scales);
	
	// Allocate the memory needed for the absolute missing pixel indices. We'll
	// allocate the number of bytes required for the largest image, since the
	// smaller ones obviously fit inside of it.
  int N = height * width;
	int* abs_inds_to_filter_h = (int*) malloc(N * sizeof(int));
	
	int pyr_size = get_pyr_size(&pyr_heights[0], &pyr_widths[0], num_scales);

	// ************************
	//   CREATING THE PYRAMID
	// ************************
	clock_t	start_pyr = clock();

	// NEG TIME.
  float* depth_ms_h = (float*) malloc(pyr_size * sizeof(float));
	float* intensity_ms_h = (float*) malloc(pyr_size * sizeof(float));
	bool* mask_ms_h = (bool*) malloc(pyr_size * sizeof(bool));
	float* result_ms_h = (float*) malloc(pyr_size * sizeof(float));
	bool* valid_ms_h = (bool*) malloc(pyr_size * sizeof(bool));

	for (int nn = 0; nn < N; ++nn) {
    depth_ms_h[nn] = depth[nn] / 255.0;
		intensity_ms_h[nn] = intensity[nn] / 255.0;
		mask_ms_h[nn] = mask_h[nn];
		valid_ms_h[nn] = !mask_h[nn];
    result_ms_h[nn] = 0;
	}
	
	float* depth_ms_h_p = depth_ms_h + pyr_offsets[1];
	float* intensity_ms_h_p	= intensity_ms_h + pyr_offsets[1];
	bool* mask_ms_h_p	= mask_ms_h + pyr_offsets[1];
	bool* valid_ms_h_p = valid_ms_h + pyr_offsets[1];
	float* result_ms_h_p = result_ms_h + pyr_offsets[1];
  
	for (int scale = 1; scale < num_scales; ++scale) {
    for (int xx = 0; xx < pyr_widths[scale]; ++xx) {
	    for (int yy = 0; yy < pyr_heights[scale]; ++yy, ++depth_ms_h_p, ++intensity_ms_h_p, ++mask_ms_h_p, ++result_ms_h_p, ++valid_ms_h_p) {
			  int abs_yy = static_cast<int>(((float)yy / pyr_heights[scale]) * height);
				int abs_xx = static_cast<int>(((float)xx / pyr_widths[scale]) * width);
			  int img_offset = abs_yy + height * abs_xx;
				*depth_ms_h_p = depth_ms_h[img_offset];
				*intensity_ms_h_p = intensity_ms_h[img_offset];
				*mask_ms_h_p = mask_h[img_offset];
				*valid_ms_h_p = !mask_h[img_offset];
        *result_ms_h_p = 0;
			}
		}
	}

	// *********************************
	//   RUN THE ACTUAL FILTERING CODE
	// *********************************
  
	for (int scale = num_scales - 1; scale >= 0; --scale) {
#ifdef DEBUG
	  printf("Filtering at scale %d, [%dx%d]\n", scale, pyr_heights[scale], pyr_widths[scale]);
    
    char filename1[50];
    sprintf(filename1, "missing_pixels_before_filtering_scale%d.pgm", scale);
    // Now that we've performed the filtering, save the intermediate image.
    savePGM(mask_ms_h + pyr_offsets[scale], filename1, pyr_heights[scale], pyr_widths[scale]);
    
    char filename2[50];
    sprintf(filename2, "valid_pixels_before_filtering_scale%d.pgm", scale);
    // Now that we've performed the filtering, save the intermediate image.
    savePGM(valid_ms_h + pyr_offsets[scale], filename2, pyr_heights[scale], pyr_widths[scale]);
    
    sprintf(filename2, "valid_intensity_before_filtering_scale%d.pgm", scale);
    // Now that we've performed the filtering, save the intermediate image.
    savePGM(intensity_ms_h + pyr_offsets[scale], filename2, pyr_heights[scale], pyr_widths[scale]);
    
    sprintf(filename2, "depth_before_filtering_scale%d.pgm", scale);
    // Now that we've performed the filtering, save the intermediate image.
    savePGM(depth_ms_h + pyr_offsets[scale], filename2, pyr_heights[scale], pyr_widths[scale]);
#endif
    
    filter_at_scale(depth_ms_h + pyr_offsets[scale],
		                intensity_ms_h + pyr_offsets[scale],
										mask_ms_h + pyr_offsets[scale],
										valid_ms_h + pyr_offsets[scale],
										result_ms_h + pyr_offsets[scale],
		                abs_inds_to_filter_h,
		                pyr_heights[scale],
										pyr_widths[scale],
										sigma_s[scale],
										sigma_r[scale]);
    
    
#ifdef DEBUG
    sprintf(filename2, "valid_pixels_after_filtering_scale%d.pgm", scale);
    // Now that we've performed the filtering, save the intermediate image.
    savePGM(valid_ms_h + pyr_offsets[scale], filename2, pyr_heights[scale], pyr_widths[scale]);
#endif
    
#ifdef DEBUG
    char filename[50];
    sprintf(filename, "depth_after_filtering_scale%d.pgm", scale);
    // Now that we've performed the filtering, save the intermediate image.
    savePGM(result_ms_h + pyr_offsets[scale], filename, pyr_heights[scale], pyr_widths[scale]);
#endif
    
		if (scale == 0) {
		  continue;
		}

		// Now, we need to upsample the resulting depth and store it in the next
		// highest location.
		int num_missing_pixels = pyr_heights[scale-1] * pyr_widths[scale-1];
		
#ifdef DEBUG
		printf("Upsampling %d\n", num_missing_pixels);
#endif
		for (int i = 0; i < num_missing_pixels; ++i) {
			upsample_cpu(depth_ms_h + pyr_offsets[scale-1],
									 mask_ms_h + pyr_offsets[scale-1],
									 valid_ms_h + pyr_offsets[scale-1],
                   depth_ms_h + pyr_offsets[scale],
									 result_ms_h + pyr_offsets[scale],
									 mask_ms_h + pyr_offsets[scale],
									 valid_ms_h + pyr_offsets[scale],
									 pyr_heights[scale],
									 pyr_widths[scale],
									 pyr_heights[scale-1],
									 pyr_widths[scale-1],
									 i);
		}
    
    
#ifdef DEBUG
    sprintf(filename, "up_depth_after_filtering_scale%d.pgm", scale);
    // Now that we've performed the filtering, save the intermediate image.
    savePGM(depth_ms_h + pyr_offsets[scale-1], filename, pyr_heights[scale-1], pyr_widths[scale-1]);
    
    sprintf(filename, "up_valid_after_filtering_scale%d.pgm", scale);
    // Now that we've performed the filtering, save the intermediate image.
    savePGM(valid_ms_h + pyr_offsets[scale-1], filename, pyr_heights[scale-1], pyr_widths[scale-1]);
#endif
	}
	
	// Copy the final result from the device.
	for (int nn = 0; nn < N; ++nn) {
		if (mask_ms_h[nn]) {
		  result[nn] = static_cast<uint8_t>(result_ms_h[nn] * 255);
		} else {
			result[nn] = depth[nn];
		}
	}
	
	free(depth_ms_h);
	free(intensity_ms_h);
	free(mask_ms_h);
	free(result_ms_h);
	free(valid_ms_h);
	free(abs_inds_to_filter_h);
											
	toc("Entire Function", start_func);
}

