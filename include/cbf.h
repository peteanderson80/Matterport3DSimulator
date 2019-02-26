// NYU Depth V2 Dataset Matlab Toolbox
// Authors: Nathan Silberman, Pushmeet Kohli, Derek Hoiem, Rob Fergus

#ifndef CBF_H_
#define CBF_H_

#include <stdint.h>

namespace cbf {

// Filters the given depth image using a Cross Bilateral Filter.
//
// Args:
//   height - height of the images.
//   width - width of the images.
//   depth - HxW row-major ordered matrix.
//   intensity - HxW row-major ordered matrix.
//   mask - HxW row-major ordered matrix.
//   result - HxW row-major ordered matrix.
//   num_scales - the number of scales at which to perform the filtering.
//   sigma_s - the space sigma (in pixels)
//   sigma_r - the range sigma (in intensity values, 0-1)
void cbf(int height, int width, uint8_t* depth, uint8_t* intensity,
         uint8_t* mask, uint8_t* result, unsigned num_scales, double* sigma_s,
         double* sigma_r);

}	 // namespace

#endif  // CBF_H_
