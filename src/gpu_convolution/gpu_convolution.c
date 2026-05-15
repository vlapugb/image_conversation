#include "gpu_convolution.h"
#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *kernel_source =
    "__kernel void convolve(\n"
    "    __global const uchar* src,\n"
    "    __global uchar* dst,\n"
    "    int width,\n"
    "    int height,\n"
    "    int stride,\n"
    "    int channels,\n"
    "    __constant float* kernel_data,\n"
    "    int kw, int kh,\n"
    "    float factor, float bias,\n"
    "    int border_mode)\n"
    "{\n"
    "    int x = get_global_id(0);\n"
    "    int y = get_global_id(1);\n"
    "\n"
    "    if (x >= width || y >= height) return;\n"
    "\n"
    "    int hkw = kw / 2;\n"
    "    int hkh = kh / 2;\n"
    "\n"
    "    for (int c = 0; c < channels; c++) {\n"
    "        if (channels == 4 && c == 3) {\n"
    "            dst[y * stride + x * channels + c] = src[y * stride + x * channels + c];\n"
    "            continue;\n"
    "        }\n"
    "\n"
    "        float sum = 0.0f;\n"
    "        for (int ky = 0; ky < kh; ky++) {\n"
    "            for (int kx = 0; kx < kw; kx++) {\n"
    "                int ix = x + kx - hkw;\n"
    "                int iy = y + ky - hkh;\n"
    "\n"
    "                if (ix < 0 || ix >= width || iy < 0 || iy >= height) {\n"
    "                    if (border_mode == 0) {\n"
    "                        ix = (ix % width + width) % width;\n"
    "                        iy = (iy % height + height) % height;\n"
    "                    } else if (border_mode == 1) {\n"
    "                        ix = (ix < 0) ? 0 : (ix >= width ? width - 1 : ix);\n"
    "                        iy = (iy < 0) ? 0 : (iy >= height ? height - 1 : iy);\n"
    "                    } else if (border_mode == 2) {\n"
    "                        if (ix < 0) ix = -ix;\n"
    "                        if (ix >= width) ix = 2 * width - 2 - ix;\n"
    "                        if (iy < 0) iy = -iy;\n"
    "                        if (iy >= height) iy = 2 * height - 2 - iy;\n"
    "                    }\n"
    "                }\n"
    "                sum += (float)src[iy * stride + ix * channels + c] * kernel_data[ky * kw + kx];\n"
    "            }\n"
    "        }\n"
    "        dst[y * stride + x * channels + c] = (uchar)clamp(factor * sum + bias, 0.0f, 255.0f);\n"
    "    }\n"
    "}\n";

static cl_context context = NULL;
static cl_command_queue queue = NULL;
static cl_program program = NULL;
static cl_kernel kernel = NULL;
static cl_device_id device = NULL;

static int opencl_init(void) {
    if (context) return 0;

    cl_platform_id platform;
    cl_int err;

    if (clGetPlatformIDs(1, &platform, NULL) != CL_SUCCESS) return -1;

    if (clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL) != CL_SUCCESS) {
        if (clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &device, NULL) != CL_SUCCESS) {
            return -1;
        }
    }

    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (err != CL_SUCCESS) return -1;

    queue = clCreateCommandQueueWithProperties(context, device, NULL, &err);
    if (err != CL_SUCCESS) return -1;

    program = clCreateProgramWithSource(context, 1, &kernel_source, NULL, &err);
    if (clBuildProgram(program, 1, &device, NULL, NULL, NULL) != CL_SUCCESS) {
        return -1;
    }

    kernel = clCreateKernel(program, "convolve", &err);
    return (err == CL_SUCCESS) ? 0 : -1;
}

int gpu_convolution(const filter_t *filter, image_view_t *view) {
    if (opencl_init() != 0) return -1;
    if (!filter || !view || !view->data || !filter->kernel) return -1;

    cl_int err;
    size_t size = view->height * view->stride;
    
    cl_mem d_src = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, size, view->data, &err);
    cl_mem d_dst = clCreateBuffer(context, CL_MEM_WRITE_ONLY, size, NULL, &err);
    
    size_t k_count = filter->width * filter->height;
    float *k_data = malloc(k_count * sizeof(float));
    for (size_t i = 0; i < k_count; i++) k_data[i] = (float)filter->kernel[i];
    
    cl_mem d_kern = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, k_count * sizeof(float), k_data, &err);

    int w = (int)view->width;
    int h = (int)view->height;
    int s = (int)view->stride;
    int ch = (int)view->channels;
    int kw = (int)filter->width;
    int kh = (int)filter->height;
    float f = (float)filter->factor;
    float b = (float)filter->bias;
    int bm = (int)filter->border_mode;

    clSetKernelArg(kernel, 0, sizeof(cl_mem), &d_src);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &d_dst);
    clSetKernelArg(kernel, 2, sizeof(int), &w);
    clSetKernelArg(kernel, 3, sizeof(int), &h);
    clSetKernelArg(kernel, 4, sizeof(int), &s);
    clSetKernelArg(kernel, 5, sizeof(int), &ch);
    clSetKernelArg(kernel, 6, sizeof(cl_mem), &d_kern);
    clSetKernelArg(kernel, 7, sizeof(int), &kw);
    clSetKernelArg(kernel, 8, sizeof(int), &kh);
    clSetKernelArg(kernel, 9, sizeof(float), &f);
    clSetKernelArg(kernel, 10, sizeof(float), &b);
    clSetKernelArg(kernel, 11, sizeof(int), &bm);

    size_t global[2] = {view->width, view->height};
    err = clEnqueueNDRangeKernel(queue, kernel, 2, NULL, global, NULL, 0, NULL, NULL);
    err |= clEnqueueReadBuffer(queue, d_dst, CL_TRUE, 0, size, view->data, 0, NULL, NULL);

    free(k_data);
    clReleaseMemObject(d_src);
    clReleaseMemObject(d_dst);
    clReleaseMemObject(d_kern);

    return (err == CL_SUCCESS) ? 0 : -1;
}
