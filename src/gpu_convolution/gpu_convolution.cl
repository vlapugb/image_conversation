static int resolve_index(int value, int limit, int border_mode) {
  if (limit <= 1) {
    return 0;
  }

  if (border_mode == 1) {
    return value < 0 ? 0 : (value >= limit ? limit - 1 : value);
  }

  if (border_mode == 2) {
    const int period = 2 * (limit - 1);
    int reflected = value % period;

    if (reflected < 0) {
      reflected += period;
    }

    return reflected >= limit ? period - reflected : reflected;
  }

  int wrapped = value % limit;
  return wrapped < 0 ? wrapped + limit : wrapped;
}

static uchar rounded_u8(float value) {
  return (uchar)clamp(floor(value + 0.5f), 0.0f, 255.0f);
}

__kernel void convolve_rows(__global const uchar *source,
                            __global uchar *destination,
                            int width,
                            int height,
                            int stride,
                            int channels,
                            __constant float *kernel_data,
                            int kernel_width,
                            int kernel_height,
                            float factor,
                            float bias,
                            int border_mode,
                            int y_begin,
                            int row_count) {
  const int x = get_global_id(0);
  const int row = get_global_id(1);

  if (x >= width || row >= row_count) {
    return;
  }

  const int y = y_begin + row;
  const int half_kernel_width = kernel_width / 2;
  const int half_kernel_height = kernel_height / 2;
  const int dst_index = y * stride + x * channels;

  for (int channel = 0; channel < channels; ++channel) {
    if (channels == 4 && channel == 3) {
      destination[dst_index + channel] = source[dst_index + channel];
      continue;
    }

    float sum = 0.0f;

    for (int kernel_y = 0; kernel_y < kernel_height; ++kernel_y) {
      const int source_y =
        resolve_index(y + kernel_y - half_kernel_height, height, border_mode);

      for (int kernel_x = 0; kernel_x < kernel_width; ++kernel_x) {
        const int source_x =
          resolve_index(x + kernel_x - half_kernel_width, width, border_mode);
        const int src_index = source_y * stride + source_x * channels + channel;
        const float kernel_value =
          kernel_data[kernel_y * kernel_width + kernel_x];

        sum += (float)source[src_index] * kernel_value;
      }
    }

    destination[dst_index + channel] = rounded_u8(factor * sum + bias);
  }
}
