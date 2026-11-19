/* Vulkan draw-list backend. The swapchain and texture callbacks are owned by
 * one app, and called on that app's window thread. */
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define VK_NO_PROTOTYPES
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../platform/cui_platform.h"

/* Loader entry points, grouped by dispatch level. */
#define CUI_VK_GLOBAL(X)  \
    X(vkCreateInstance)
#define CUI_VK_INSTANCE(X)  \
    X(vkDestroyInstance) \
    X(vkGetDeviceProcAddr) \
    X(vkCreateWin32SurfaceKHR) \
    X(vkDestroySurfaceKHR) \
    X(vkEnumeratePhysicalDevices) \
    X(vkGetPhysicalDeviceProperties) \
    X(vkGetPhysicalDeviceMemoryProperties) \
    X(vkGetPhysicalDeviceQueueFamilyProperties) \
    X(vkGetPhysicalDeviceSurfaceSupportKHR) \
    X(vkGetPhysicalDeviceSurfaceFormatsKHR) \
    X(vkGetPhysicalDeviceSurfacePresentModesKHR) \
    X(vkGetPhysicalDeviceSurfaceCapabilitiesKHR) \
    X(vkCreateDevice)
#define CUI_VK_DEVICE(X)  \
    X(vkDestroyDevice) \
    X(vkAcquireNextImageKHR) \
    X(vkAllocateCommandBuffers) \
    X(vkAllocateDescriptorSets) \
    X(vkAllocateMemory) \
    X(vkBeginCommandBuffer) \
    X(vkBindBufferMemory) \
    X(vkBindImageMemory) \
    X(vkCmdBeginRenderPass) \
    X(vkCmdBindDescriptorSets) \
    X(vkCmdBindIndexBuffer) \
    X(vkCmdBindPipeline) \
    X(vkCmdBindVertexBuffers) \
    X(vkCmdCopyBufferToImage) \
    X(vkCmdCopyImageToBuffer) \
    X(vkCmdDraw) \
    X(vkCmdDrawIndexed) \
    X(vkCmdEndRenderPass) \
    X(vkCmdPipelineBarrier) \
    X(vkCmdPushConstants) \
    X(vkCmdSetScissor) \
    X(vkCmdSetViewport) \
    X(vkCreateBuffer) \
    X(vkCreateCommandPool) \
    X(vkCreateDescriptorPool) \
    X(vkCreateDescriptorSetLayout) \
    X(vkCreateFence) \
    X(vkCreateFramebuffer) \
    X(vkCreateGraphicsPipelines) \
    X(vkCreateImage) \
    X(vkCreateImageView) \
    X(vkCreatePipelineLayout) \
    X(vkCreateRenderPass) \
    X(vkCreateSampler) \
    X(vkCreateSemaphore) \
    X(vkCreateShaderModule) \
    X(vkCreateSwapchainKHR) \
    X(vkDestroyBuffer) \
    X(vkDestroyCommandPool) \
    X(vkDestroyDescriptorPool) \
    X(vkDestroyDescriptorSetLayout) \
    X(vkDestroyFence) \
    X(vkDestroyFramebuffer) \
    X(vkDestroyImage) \
    X(vkDestroyImageView) \
    X(vkDestroyPipeline) \
    X(vkDestroyPipelineLayout) \
    X(vkDestroyRenderPass) \
    X(vkDestroySampler) \
    X(vkDestroySemaphore) \
    X(vkDestroyShaderModule) \
    X(vkDestroySwapchainKHR) \
    X(vkEndCommandBuffer) \
    X(vkFreeDescriptorSets) \
    X(vkFreeMemory) \
    X(vkGetBufferMemoryRequirements) \
    X(vkGetDeviceQueue) \
    X(vkGetImageMemoryRequirements) \
    X(vkGetSwapchainImagesKHR) \
    X(vkMapMemory) \
    X(vkQueuePresentKHR) \
    X(vkQueueSubmit) \
    X(vkQueueWaitIdle) \
    X(vkResetFences) \
    X(vkUnmapMemory) \
    X(vkUpdateDescriptorSets) \
    X(vkWaitForFences)

#define CUI_VK_FRAMES 2
/* Small UI frames are cheaper to upload with two contiguous copies. */
#define CUI_VK_MIN_INSTANCES 1024
#define VK_OK(call) ((call) == VK_SUCCESS)

typedef struct {
    VkBuffer handle;
    VkDeviceMemory memory;
    void *map;
    VkDeviceSize size;
} vk_buffer;

typedef struct {
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
} vk_image;

typedef struct {
    vk_image image;
    VkDescriptorPool pool;
    VkDescriptorSet set;
    int w, h, bpp;
} vk_texture;

typedef struct {
    VkCommandBuffer cmd;
    VkFence done;
    VkSemaphore acquired;
    vk_buffer vertices, indices, staging;
} vk_frame;

typedef struct {
    float rect[4], uv[4], local[4], half[2], radii[4], params[2];
    cui_color top, bottom, border;
} vk_quad;

typedef struct {
    uint32_t first, instances, mesh_vertices;
    int32_t vertex_offset;
} vk_draw;

typedef struct {
    VkImage image;
    VkBufferImageCopy copy;
    bool initial;
} vk_upload;

typedef struct {
    VkImageView view;
    VkFramebuffer fb;
    /* Reacquiring this image also retires its previous presentation wait. */
    VkSemaphore rendered;
} vk_backbuffer;

struct cui_vk_renderer {
    HMODULE loader;
    struct {
        PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
#define VK_FN(name) PFN_##name name;
        CUI_VK_GLOBAL(VK_FN)
        CUI_VK_INSTANCE(VK_FN)
        CUI_VK_DEVICE(VK_FN)
#undef VK_FN
    } vk;
    VkInstance instance;
    VkSurfaceKHR surface;
    VkPhysicalDevice physical;
    VkDevice device;
    VkPhysicalDeviceMemoryProperties memory;
    uint32_t family;
    VkQueue queue;
    VkFormat format;
    VkColorSpaceKHR colorspace;
    VkPresentModeKHR mode;
    VkSwapchainKHR swap;
    VkExtent2D extent;
    int width, height;
    uint32_t back_count, next;
    vk_backbuffer *backs;
    bool recreate, failed, hidden;
    VkCommandPool commands;
    VkCommandBuffer immediate;
    vk_frame frames[CUI_VK_FRAMES];
    VkRenderPass pass;
    VkRenderPass capture_pass;
    VkPipeline pipeline;
    VkPipeline quad_pipeline;
    VkPipelineLayout layout;
    VkDescriptorSetLayout set_layout;
    VkSampler sampler;
    VkDescriptorPool *pools;
    size_t pool_count;
    vk_texture *textures;
    size_t texture_count;
    cui_texture white;
    vk_upload *uploads;
    size_t upload_count, upload_capacity;
    uint8_t *upload_pixels;
    size_t upload_size, upload_pixel_capacity;
    vk_draw *draws;
    size_t draw_capacity;
    size_t geometry_bytes;
    VkDeviceSize quad_offset;
    bool compact_geometry, instancing;
};

static uint32_t memory_type(cui_vk_renderer *r, uint32_t bits, VkMemoryPropertyFlags flags) {
    for (uint32_t i = 0; i < r->memory.memoryTypeCount; i++)
        if ((bits & (1u << i)) && (r->memory.memoryTypes[i].propertyFlags & flags) == flags) return i;
    return UINT32_MAX;
}

static void free_buffer(cui_vk_renderer *r, vk_buffer *b) {
    if (b->map) r->vk.vkUnmapMemory(r->device, b->memory);
    r->vk.vkDestroyBuffer(r->device, b->handle, NULL);
    r->vk.vkFreeMemory(r->device, b->memory, NULL);
    memset(b, 0, sizeof *b);
}

static bool buffer(cui_vk_renderer *r, vk_buffer *b, VkDeviceSize size, VkBufferUsageFlags usage) {
    if (size <= b->size) return true;
    free_buffer(r, b);
    VkBufferCreateInfo info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    info.size = size;
    info.usage = usage;
    if (!VK_OK(r->vk.vkCreateBuffer(r->device, &info, NULL, &b->handle))) return false;
    VkMemoryRequirements req;
    r->vk.vkGetBufferMemoryRequirements(r->device, b->handle, &req);
    VkMemoryAllocateInfo alloc = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    alloc.allocationSize = req.size;
    VkMemoryPropertyFlags mapped = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    VkResult allocated = VK_ERROR_OUT_OF_DEVICE_MEMORY;
    uint32_t types = req.memoryTypeBits;
    if (usage & (VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT)) {
        /* Prefer GPU-local geometry while retaining direct, coherent CPU writes. */
        alloc.memoryTypeIndex = memory_type(r, types, mapped | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if (alloc.memoryTypeIndex != UINT32_MAX) {
            types &= ~(1u << alloc.memoryTypeIndex);
            allocated = r->vk.vkAllocateMemory(r->device, &alloc, NULL, &b->memory);
        }
    }
    if (allocated == VK_ERROR_OUT_OF_DEVICE_MEMORY) {
        alloc.memoryTypeIndex = memory_type(r, types, mapped);
        if (alloc.memoryTypeIndex != UINT32_MAX)
            allocated = r->vk.vkAllocateMemory(r->device, &alloc, NULL, &b->memory);
    }
    if (allocated != VK_SUCCESS ||
        !VK_OK(r->vk.vkBindBufferMemory(r->device, b->handle, b->memory, 0)) ||
        !VK_OK(r->vk.vkMapMemory(r->device, b->memory, 0, size, 0, &b->map))) {
        free_buffer(r, b);
        return false;
    }
    b->size = size;
    return true;
}

static bool image_view(cui_vk_renderer *r, VkImage image, VkFormat format, VkImageView *out) {
    VkImageViewCreateInfo view = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    view.image = image;
    view.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view.format = format;
    view.subresourceRange = (VkImageSubresourceRange){ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    return VK_OK(r->vk.vkCreateImageView(r->device, &view, NULL, out));
}

static bool make_image(cui_vk_renderer *r, vk_image *im, uint32_t w, uint32_t h,
                       VkFormat format, VkImageUsageFlags usage) {
    VkImageCreateInfo info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    info.imageType = VK_IMAGE_TYPE_2D;
    info.format = format;
    info.extent = (VkExtent3D){ w, h, 1 };
    info.mipLevels = info.arrayLayers = 1;
    info.samples = VK_SAMPLE_COUNT_1_BIT;
    info.usage = usage;
    if (!VK_OK(r->vk.vkCreateImage(r->device, &info, NULL, &im->image))) return false;
    VkMemoryRequirements req;
    r->vk.vkGetImageMemoryRequirements(r->device, im->image, &req);
    VkMemoryAllocateInfo alloc = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    alloc.allocationSize = req.size;
    alloc.memoryTypeIndex = memory_type(r, req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    return alloc.memoryTypeIndex != UINT32_MAX &&
        VK_OK(r->vk.vkAllocateMemory(r->device, &alloc, NULL, &im->memory)) &&
        VK_OK(r->vk.vkBindImageMemory(r->device, im->image, im->memory, 0)) &&
        image_view(r, im->image, format, &im->view);
}

static void free_image(cui_vk_renderer *r, vk_image *im) {
    r->vk.vkDestroyImageView(r->device, im->view, NULL);
    r->vk.vkDestroyImage(r->device, im->image, NULL);
    r->vk.vkFreeMemory(r->device, im->memory, NULL);
}

static void barrier(cui_vk_renderer *r, VkCommandBuffer cmd, VkImage image,
                    VkImageLayout old, VkImageLayout next,
                    VkPipelineStageFlags src, VkPipelineStageFlags dst,
                    VkAccessFlags read, VkAccessFlags write) {
    VkImageMemoryBarrier b = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    b.srcAccessMask = read;
    b.dstAccessMask = write;
    b.oldLayout = old;
    b.newLayout = next;
    b.srcQueueFamilyIndex = b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.image = image;
    b.subresourceRange = (VkImageSubresourceRange){ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    r->vk.vkCmdPipelineBarrier(cmd, src, dst, 0, 0, NULL, 0, NULL, 1, &b);
}

static bool begin(cui_vk_renderer *r, VkCommandBuffer cmd) {
    VkCommandBufferBeginInfo info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    /* The pool permits vkBeginCommandBuffer to reset an executable buffer. */
    return VK_OK(r->vk.vkBeginCommandBuffer(cmd, &info));
}

static bool wait_vk(cui_vk_renderer *r) {
    return VK_OK(r->vk.vkQueueWaitIdle(r->queue));
}

static bool finish_immediate(cui_vk_renderer *r) {
    VkSubmitInfo submit = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &r->immediate;
    return VK_OK(r->vk.vkEndCommandBuffer(r->immediate)) &&
        VK_OK(r->vk.vkQueueSubmit(r->queue, 1, &submit, VK_NULL_HANDLE)) && wait_vk(r);
}

static bool upload(cui_vk_renderer *r, vk_texture *t, bool initial,
                   int x, int y, int w, int h, const void *pixels, int stride) {
    /* Caller-owned pixels can change as soon as the texture callback returns. */
    size_t row = (size_t)w * t->bpp;
    if (r->upload_size > SIZE_MAX - 3) return false;
    size_t offset = (r->upload_size + 3) & ~(size_t)3;
    if (row * h > SIZE_MAX - offset) return false;
    size_t size = offset + row * h;
    if (size > r->upload_pixel_capacity) {
        size_t capacity = size <= SIZE_MAX / 2 ? size * 2 : size;
        uint8_t *data = realloc(r->upload_pixels, capacity);
        if (!data) return false;
        r->upload_pixels = data;
        r->upload_pixel_capacity = capacity;
    }
    if (r->upload_count == r->upload_capacity) {
        if (r->upload_capacity > SIZE_MAX / sizeof *r->uploads / 2) return false;
        size_t capacity = r->upload_capacity ? r->upload_capacity * 2 : 8;
        vk_upload *jobs = realloc(r->uploads, capacity * sizeof *jobs);
        if (!jobs) return false;
        r->uploads = jobs;
        r->upload_capacity = capacity;
    }
    uint8_t *dst = r->upload_pixels + offset;
    if (!pixels) memset(dst, 0, row * h);
    else if ((size_t)stride == row) memcpy(dst, pixels, row * h);
    else for (int i = 0; i < h; i++)
        memcpy(dst + i * row, (const uint8_t *)pixels + (size_t)i * stride, row);

    VkBufferImageCopy copy = {0};
    copy.bufferOffset = offset;
    copy.imageSubresource = (VkImageSubresourceLayers){ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
    copy.imageOffset = (VkOffset3D){ x, y, 0 };
    copy.imageExtent = (VkExtent3D){ w, h, 1 };
    r->uploads[r->upload_count++] = (vk_upload){ t->image.image, copy, initial };
    r->upload_size = size;
    return true;
}

static bool stage_uploads(cui_vk_renderer *r, vk_frame *f) {
    if (!r->upload_count) return true;
    /* The frame fence also retires this staging buffer. */
    if (r->upload_size > f->staging.size) {
        size_t size = r->upload_size;
        if (size <= SIZE_MAX / 2) size += size / 2;
        if (!buffer(r, &f->staging, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT)) return false;
    }
    memcpy(f->staging.map, r->upload_pixels, r->upload_size);
    return true;
}

static void record_uploads(cui_vk_renderer *r, vk_frame *f, VkCommandBuffer cmd) {
    for (size_t i = 0; i < r->upload_count; i++) {
        const vk_upload *job = &r->uploads[i];
        barrier(r, cmd, job->image,
                job->initial ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                job->initial ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT :
                    (VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT),
                VK_PIPELINE_STAGE_TRANSFER_BIT, job->initial ? 0 : (VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT),
                VK_ACCESS_TRANSFER_WRITE_BIT);
        r->vk.vkCmdCopyBufferToImage(cmd, f->staging.handle, job->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &job->copy);
        barrier(r, cmd, job->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);
    }
}

static bool descriptor(cui_vk_renderer *r, vk_texture *t) {
    VkDescriptorSetAllocateInfo alloc = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    alloc.descriptorSetCount = 1;
    alloc.pSetLayouts = &r->set_layout;
    const size_t existing = r->pool_count;
    for (size_t i = 0; i <= existing; i++) {
        if (i == r->pool_count) {
            VkDescriptorPool *p = realloc(r->pools, (i + 1) * sizeof *p);
            if (!p) return false;
            r->pools = p;
            VkDescriptorPoolSize size = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 64 };
            VkDescriptorPoolCreateInfo info = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
            info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
            info.maxSets = 64;
            info.poolSizeCount = 1;
            info.pPoolSizes = &size;
            if (!VK_OK(r->vk.vkCreateDescriptorPool(r->device, &info, NULL, &p[i]))) return false;
            r->pool_count++;
        }
        alloc.descriptorPool = r->pools[i];
        VkResult result = r->vk.vkAllocateDescriptorSets(r->device, &alloc, &t->set);
        if (result == VK_SUCCESS) {
            t->pool = alloc.descriptorPool;
            VkDescriptorImageInfo image = { r->sampler, t->image.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
            VkWriteDescriptorSet write = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
            write.dstSet = t->set;
            write.descriptorCount = 1;
            write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            write.pImageInfo = &image;
            r->vk.vkUpdateDescriptorSets(r->device, 1, &write, 0, NULL);
            return true;
        }
        if (result != VK_ERROR_OUT_OF_POOL_MEMORY && result != VK_ERROR_FRAGMENTED_POOL) return false;
    }
    return false;
}

static cui_texture tex_create(void *user, int w, int h, cui_texture_format format, const void *pixels) {
    cui_vk_renderer *r = user;
    if (w <= 0 || h <= 0 || w > INT_MAX / 4 || (size_t)w > SIZE_MAX / 4 / (size_t)h) return 0;
    size_t i = 0;
    while (i < r->texture_count && r->textures[i].set) i++;
    if (i == r->texture_count) {
        vk_texture *p = realloc(r->textures, (i + 1) * sizeof *p);
        if (!p) return 0;
        r->textures = p;
        r->textures[i] = (vk_texture){0};
        r->texture_count++;
    }
    vk_texture t = {0};
    t.w = w; t.h = h; t.bpp = (int)format;
    VkFormat fmt = format == CUI_TEXTURE_R8 ? VK_FORMAT_R8_UNORM : VK_FORMAT_R8G8B8A8_UNORM;
    if (!make_image(r, &t.image, w, h, fmt, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT) ||
        !descriptor(r, &t) || !upload(r, &t, true, 0, 0, w, h, pixels, w * t.bpp)) {
        if (t.pool) r->vk.vkFreeDescriptorSets(r->device, t.pool, 1, &t.set);
        free_image(r, &t.image);
        return 0;
    }
    r->textures[i] = t;
    return (cui_texture)(i + 1);
}

static vk_texture *texture(cui_vk_renderer *r, cui_texture id) {
    return id && id <= r->texture_count && r->textures[id - 1].set ? &r->textures[id - 1] : NULL;
}

static void tex_update(void *user, cui_texture id, int x, int y, int w, int h, const void *pixels, int stride) {
    cui_vk_renderer *r = user;
    vk_texture *t = texture(r, id);
    if (t && pixels && x >= 0 && y >= 0 && w > 0 && h > 0 &&
        w <= t->w - x && h <= t->h - y && stride >= w * t->bpp)
        if (!upload(r, t, false, x, y, w, h, pixels, stride))
            fprintf(stderr, "cui: Vulkan texture upload failed\n");
}

static void tex_destroy(void *user, cui_texture id) {
    cui_vk_renderer *r = user;
    vk_texture *t = texture(r, id);
    if (!t) return;
    size_t pending = 0;
    for (size_t i = 0; i < r->upload_count; i++)
        if (r->uploads[i].image != t->image.image) r->uploads[pending++] = r->uploads[i];
    r->upload_count = pending;
    if (!pending) r->upload_size = 0;
    wait_vk(r);
    r->vk.vkFreeDescriptorSets(r->device, t->pool, 1, &t->set);
    free_image(r, &t->image);
    *t = (vk_texture){0};
}

cui_renderer_iface cui_vulkan_iface(cui_vk_renderer *r) {
    return (cui_renderer_iface){ r, tex_create, tex_update, tex_destroy };
}
/* Embedded shaders. Regenerate each SPIR-V array from the GLSL above it. */
/* Vertex shader (cui.vert)

#version 450
layout(location = 0) in vec2 a_pos;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in vec2 a_local;
layout(location = 3) in vec2 a_half;
layout(location = 4) in vec4 a_radii;
layout(location = 5) in vec2 a_params;
layout(location = 6) in vec4 a_color;
layout(location = 7) in vec4 a_bcolor;
layout(push_constant) uniform Frame { vec2 u_viewport; };
layout(location = 0) out vec2 v_uv;
layout(location = 1) out vec2 v_local;
layout(location = 2) flat out vec2 v_half;
layout(location = 3) flat out vec4 v_radii;
layout(location = 4) flat out vec2 v_params;
layout(location = 5) out vec4 v_color;
layout(location = 6) flat out vec4 v_bcolor;
void main() {
  v_uv = a_uv; v_local = a_local; v_half = a_half; v_radii = a_radii;
  v_params = a_params; v_color = a_color; v_bcolor = a_bcolor;
  gl_Position = vec4(a_pos.x / u_viewport.x * 2.0 - 1.0, a_pos.y / u_viewport.y * 2.0 - 1.0, 0.0, 1.0);
}
*/
static const uint32_t cui_vk_vert[] = {
    0x07230203, 0x00010000, 0x0008000b, 0x00000044, 0x00000000, 0x00020011, 0x00000001, 0x0006000b,
    0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001,
    0x0015000f, 0x00000000, 0x00000004, 0x6e69616d, 0x00000000, 0x00000009, 0x0000000b, 0x0000000d,
    0x0000000e, 0x00000010, 0x00000011, 0x00000015, 0x00000017, 0x00000019, 0x0000001a, 0x0000001c,
    0x0000001d, 0x0000001f, 0x00000020, 0x00000027, 0x0000002a, 0x00030003, 0x00000002, 0x000001c2,
    0x00040005, 0x00000004, 0x6e69616d, 0x00000000, 0x00040005, 0x00000009, 0x76755f76, 0x00000000,
    0x00040005, 0x0000000b, 0x76755f61, 0x00000000, 0x00040005, 0x0000000d, 0x6f6c5f76, 0x006c6163,
    0x00040005, 0x0000000e, 0x6f6c5f61, 0x006c6163, 0x00040005, 0x00000010, 0x61685f76, 0x0000666c,
    0x00040005, 0x00000011, 0x61685f61, 0x0000666c, 0x00040005, 0x00000015, 0x61725f76, 0x00696964,
    0x00040005, 0x00000017, 0x61725f61, 0x00696964, 0x00050005, 0x00000019, 0x61705f76, 0x736d6172,
    0x00000000, 0x00050005, 0x0000001a, 0x61705f61, 0x736d6172, 0x00000000, 0x00040005, 0x0000001c,
    0x6f635f76, 0x00726f6c, 0x00040005, 0x0000001d, 0x6f635f61, 0x00726f6c, 0x00050005, 0x0000001f,
    0x63625f76, 0x726f6c6f, 0x00000000, 0x00050005, 0x00000020, 0x63625f61, 0x726f6c6f, 0x00000000,
    0x00060005, 0x00000025, 0x505f6c67, 0x65567265, 0x78657472, 0x00000000, 0x00060006, 0x00000025,
    0x00000000, 0x505f6c67, 0x7469736f, 0x006e6f69, 0x00070006, 0x00000025, 0x00000001, 0x505f6c67,
    0x746e696f, 0x657a6953, 0x00000000, 0x00070006, 0x00000025, 0x00000002, 0x435f6c67, 0x4470696c,
    0x61747369, 0x0065636e, 0x00070006, 0x00000025, 0x00000003, 0x435f6c67, 0x446c6c75, 0x61747369,
    0x0065636e, 0x00030005, 0x00000027, 0x00000000, 0x00040005, 0x0000002a, 0x6f705f61, 0x00000073,
    0x00040005, 0x0000002f, 0x6d617246, 0x00000065, 0x00060006, 0x0000002f, 0x00000000, 0x69765f75,
    0x6f707765, 0x00007472, 0x00030005, 0x00000031, 0x00000000, 0x00040047, 0x00000009, 0x0000001e,
    0x00000000, 0x00040047, 0x0000000b, 0x0000001e, 0x00000001, 0x00040047, 0x0000000d, 0x0000001e,
    0x00000001, 0x00040047, 0x0000000e, 0x0000001e, 0x00000002, 0x00030047, 0x00000010, 0x0000000e,
    0x00040047, 0x00000010, 0x0000001e, 0x00000002, 0x00040047, 0x00000011, 0x0000001e, 0x00000003,
    0x00030047, 0x00000015, 0x0000000e, 0x00040047, 0x00000015, 0x0000001e, 0x00000003, 0x00040047,
    0x00000017, 0x0000001e, 0x00000004, 0x00030047, 0x00000019, 0x0000000e, 0x00040047, 0x00000019,
    0x0000001e, 0x00000004, 0x00040047, 0x0000001a, 0x0000001e, 0x00000005, 0x00040047, 0x0000001c,
    0x0000001e, 0x00000005, 0x00040047, 0x0000001d, 0x0000001e, 0x00000006, 0x00030047, 0x0000001f,
    0x0000000e, 0x00040047, 0x0000001f, 0x0000001e, 0x00000006, 0x00040047, 0x00000020, 0x0000001e,
    0x00000007, 0x00030047, 0x00000025, 0x00000002, 0x00050048, 0x00000025, 0x00000000, 0x0000000b,
    0x00000000, 0x00050048, 0x00000025, 0x00000001, 0x0000000b, 0x00000001, 0x00050048, 0x00000025,
    0x00000002, 0x0000000b, 0x00000003, 0x00050048, 0x00000025, 0x00000003, 0x0000000b, 0x00000004,
    0x00040047, 0x0000002a, 0x0000001e, 0x00000000, 0x00030047, 0x0000002f, 0x00000002, 0x00050048,
    0x0000002f, 0x00000000, 0x00000023, 0x00000000, 0x00020013, 0x00000002, 0x00030021, 0x00000003,
    0x00000002, 0x00030016, 0x00000006, 0x00000020, 0x00040017, 0x00000007, 0x00000006, 0x00000002,
    0x00040020, 0x00000008, 0x00000003, 0x00000007, 0x0004003b, 0x00000008, 0x00000009, 0x00000003,
    0x00040020, 0x0000000a, 0x00000001, 0x00000007, 0x0004003b, 0x0000000a, 0x0000000b, 0x00000001,
    0x0004003b, 0x00000008, 0x0000000d, 0x00000003, 0x0004003b, 0x0000000a, 0x0000000e, 0x00000001,
    0x0004003b, 0x00000008, 0x00000010, 0x00000003, 0x0004003b, 0x0000000a, 0x00000011, 0x00000001,
    0x00040017, 0x00000013, 0x00000006, 0x00000004, 0x00040020, 0x00000014, 0x00000003, 0x00000013,
    0x0004003b, 0x00000014, 0x00000015, 0x00000003, 0x00040020, 0x00000016, 0x00000001, 0x00000013,
    0x0004003b, 0x00000016, 0x00000017, 0x00000001, 0x0004003b, 0x00000008, 0x00000019, 0x00000003,
    0x0004003b, 0x0000000a, 0x0000001a, 0x00000001, 0x0004003b, 0x00000014, 0x0000001c, 0x00000003,
    0x0004003b, 0x00000016, 0x0000001d, 0x00000001, 0x0004003b, 0x00000014, 0x0000001f, 0x00000003,
    0x0004003b, 0x00000016, 0x00000020, 0x00000001, 0x00040015, 0x00000022, 0x00000020, 0x00000000,
    0x0004002b, 0x00000022, 0x00000023, 0x00000001, 0x0004001c, 0x00000024, 0x00000006, 0x00000023,
    0x0006001e, 0x00000025, 0x00000013, 0x00000006, 0x00000024, 0x00000024, 0x00040020, 0x00000026,
    0x00000003, 0x00000025, 0x0004003b, 0x00000026, 0x00000027, 0x00000003, 0x00040015, 0x00000028,
    0x00000020, 0x00000001, 0x0004002b, 0x00000028, 0x00000029, 0x00000000, 0x0004003b, 0x0000000a,
    0x0000002a, 0x00000001, 0x0004002b, 0x00000022, 0x0000002b, 0x00000000, 0x00040020, 0x0000002c,
    0x00000001, 0x00000006, 0x0003001e, 0x0000002f, 0x00000007, 0x00040020, 0x00000030, 0x00000009,
    0x0000002f, 0x0004003b, 0x00000030, 0x00000031, 0x00000009, 0x00040020, 0x00000032, 0x00000009,
    0x00000006, 0x0004002b, 0x00000006, 0x00000036, 0x40000000, 0x0004002b, 0x00000006, 0x00000038,
    0x3f800000, 0x0004002b, 0x00000006, 0x00000041, 0x00000000, 0x00050036, 0x00000002, 0x00000004,
    0x00000000, 0x00000003, 0x000200f8, 0x00000005, 0x0004003d, 0x00000007, 0x0000000c, 0x0000000b,
    0x0003003e, 0x00000009, 0x0000000c, 0x0004003d, 0x00000007, 0x0000000f, 0x0000000e, 0x0003003e,
    0x0000000d, 0x0000000f, 0x0004003d, 0x00000007, 0x00000012, 0x00000011, 0x0003003e, 0x00000010,
    0x00000012, 0x0004003d, 0x00000013, 0x00000018, 0x00000017, 0x0003003e, 0x00000015, 0x00000018,
    0x0004003d, 0x00000007, 0x0000001b, 0x0000001a, 0x0003003e, 0x00000019, 0x0000001b, 0x0004003d,
    0x00000013, 0x0000001e, 0x0000001d, 0x0003003e, 0x0000001c, 0x0000001e, 0x0004003d, 0x00000013,
    0x00000021, 0x00000020, 0x0003003e, 0x0000001f, 0x00000021, 0x00050041, 0x0000002c, 0x0000002d,
    0x0000002a, 0x0000002b, 0x0004003d, 0x00000006, 0x0000002e, 0x0000002d, 0x00060041, 0x00000032,
    0x00000033, 0x00000031, 0x00000029, 0x0000002b, 0x0004003d, 0x00000006, 0x00000034, 0x00000033,
    0x00050088, 0x00000006, 0x00000035, 0x0000002e, 0x00000034, 0x00050085, 0x00000006, 0x00000037,
    0x00000035, 0x00000036, 0x00050083, 0x00000006, 0x00000039, 0x00000037, 0x00000038, 0x00050041,
    0x0000002c, 0x0000003a, 0x0000002a, 0x00000023, 0x0004003d, 0x00000006, 0x0000003b, 0x0000003a,
    0x00060041, 0x00000032, 0x0000003c, 0x00000031, 0x00000029, 0x00000023, 0x0004003d, 0x00000006,
    0x0000003d, 0x0000003c, 0x00050088, 0x00000006, 0x0000003e, 0x0000003b, 0x0000003d, 0x00050085,
    0x00000006, 0x0000003f, 0x0000003e, 0x00000036, 0x00050083, 0x00000006, 0x00000040, 0x0000003f,
    0x00000038, 0x00070050, 0x00000013, 0x00000042, 0x00000039, 0x00000040, 0x00000041, 0x00000038,
    0x00050041, 0x00000014, 0x00000043, 0x00000027, 0x00000029, 0x0003003e, 0x00000043, 0x00000042,
    0x000100fd, 0x00010038,
};

/* Fragment shader (cui.frag)

#version 450
layout(location = 0) in vec2 v_uv;
layout(location = 1) in vec2 v_local;
layout(location = 2) flat in vec2 v_half;
layout(location = 3) flat in vec4 v_radii;
layout(location = 4) flat in vec2 v_params;
layout(location = 5) in vec4 v_color;
layout(location = 6) flat in vec4 v_bcolor;
layout(set = 0, binding = 0) uniform sampler2D u_tex;
layout(location = 0) out vec4 frag;
float sd_rrect(vec2 p, vec2 b, vec4 r) {
  float rr = p.x < 0.0 ? (p.y < 0.0 ? r.x : r.w) : (p.y < 0.0 ? r.y : r.z);
  rr = min(rr, min(b.x, b.y));
  vec2 q = abs(p) - b + rr;
  return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - rr;
}
float erf_(float x) {
  float s = sign(x); x = abs(x);
  float t = 1.0 / (1.0 + 0.3275911 * x);
  float y = 1.0 - (((((1.061405429 * t - 1.453152027) * t) + 1.421413741) * t - 0.284496736) * t + 0.254829592) * t * exp(-x * x);
  return s * y;
}
float bayer8(vec2 p) {
  ivec2 q = ivec2(mod(floor(p), 8.0));
  int a = q.x ^ q.y, y = q.y;
  int m = ((a & 1) << 5) | ((y & 1) << 4) | ((a & 2) << 2) | ((y & 2) << 1) | ((a & 4) >> 1) | ((y & 4) >> 2);
  return (float(m) + 0.5) / 64.0;
}
void main() {
  int mode = int(v_params.y + 0.5);
  vec4 c = v_color;
  if (mode == 4) { frag = vec4(c.rgb * c.a, c.a); return; }
  if (mode == 0) { float a = c.a * texture(u_tex, v_uv).r; frag = vec4(c.rgb * a, a); return; }
  float d = sd_rrect(v_local, v_half, v_radii);
  if (mode == 1) {
    float outer = clamp(0.5 - d, 0.0, 1.0);
    float bw = v_params.x;
    float inner = bw > 0.0 ? clamp(0.5 - (d + bw), 0.0, 1.0) : outer;
    float fa = c.a * inner, ba = v_bcolor.a * (outer - inner);
    frag = vec4(c.rgb * fa + v_bcolor.rgb * ba, fa + ba);
  } else if (mode == 2) {
    float s = max(v_params.x, 0.01);
    float a = c.a * 0.5 * (1.0 - erf_(d / (s * 1.41421356)));
    frag = vec4(c.rgb * a, a);
  } else if (mode == 5) {
    float t = bayer8(v_uv);
    float k = t >= v_params.x && t < c.a ? v_bcolor.a * clamp(0.5 - d, 0.0, 1.0) : 0.0;
    frag = vec4(v_bcolor.rgb * k, k);
  } else {
    float cov = clamp(0.5 - d, 0.0, 1.0);
    vec4 t = texture(u_tex, v_uv);
    float k = c.a * cov;
    frag = vec4(t.rgb * t.a * c.rgb * k, t.a * k);
  }
}
*/
static const uint32_t cui_vk_frag[] = {
    0x07230203, 0x00010000, 0x0008000b, 0x000001a9, 0x00000000, 0x00020011, 0x00000001, 0x0006000b,
    0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001,
    0x000d000f, 0x00000004, 0x00000004, 0x6e69616d, 0x00000000, 0x000000c6, 0x000000ce, 0x000000d5,
    0x000000f0, 0x00000100, 0x00000101, 0x00000102, 0x00000129, 0x00030010, 0x00000004, 0x00000007,
    0x00030003, 0x00000002, 0x000001c2, 0x00040005, 0x00000004, 0x6e69616d, 0x00000000, 0x00080005,
    0x0000000f, 0x725f6473, 0x74636572, 0x32667628, 0x3266763b, 0x3466763b, 0x0000003b, 0x00030005,
    0x0000000c, 0x00000070, 0x00030005, 0x0000000d, 0x00000062, 0x00030005, 0x0000000e, 0x00000072,
    0x00050005, 0x00000014, 0x5f667265, 0x3b316628, 0x00000000, 0x00030005, 0x00000013, 0x00000078,
    0x00050005, 0x00000018, 0x65796162, 0x76283872, 0x003b3266, 0x00030005, 0x00000017, 0x00000070,
    0x00030005, 0x0000001a, 0x00007272, 0x00030005, 0x00000049, 0x00000071, 0x00030005, 0x00000060,
    0x00000073, 0x00030005, 0x00000065, 0x00000074, 0x00030005, 0x0000006c, 0x00000079, 0x00030005,
    0x0000008f, 0x00000071, 0x00030005, 0x00000097, 0x00000061, 0x00030005, 0x0000009d, 0x00000079,
    0x00030005, 0x000000a0, 0x0000006d, 0x00040005, 0x000000c4, 0x65646f6d, 0x00000000, 0x00050005,
    0x000000c6, 0x61705f76, 0x736d6172, 0x00000000, 0x00030005, 0x000000cc, 0x00000063, 0x00040005,
    0x000000ce, 0x6f635f76, 0x00726f6c, 0x00040005, 0x000000d5, 0x67617266, 0x00000000, 0x00030005,
    0x000000e8, 0x00000061, 0x00040005, 0x000000ee, 0x65745f75, 0x00000078, 0x00040005, 0x000000f0,
    0x76755f76, 0x00000000, 0x00030005, 0x000000ff, 0x00000064, 0x00040005, 0x00000100, 0x6f6c5f76,
    0x006c6163, 0x00040005, 0x00000101, 0x61685f76, 0x0000666c, 0x00040005, 0x00000102, 0x61725f76,
    0x00696964, 0x00040005, 0x00000103, 0x61726170, 0x0000006d, 0x00040005, 0x00000105, 0x61726170,
    0x0000006d, 0x00040005, 0x00000107, 0x61726170, 0x0000006d, 0x00040005, 0x0000010e, 0x6574756f,
    0x00000072, 0x00030005, 0x00000112, 0x00007762, 0x00040005, 0x00000115, 0x656e6e69, 0x00000072,
    0x00030005, 0x00000123, 0x00006166, 0x00030005, 0x00000128, 0x00006162, 0x00050005, 0x00000129,
    0x63625f76, 0x726f6c6f, 0x00000000, 0x00030005, 0x00000145, 0x00000073, 0x00030005, 0x0000014a,
    0x00000061, 0x00040005, 0x00000153, 0x61726170, 0x0000006d, 0x00030005, 0x00000165, 0x00000074,
    0x00040005, 0x00000166, 0x61726170, 0x0000006d, 0x00030005, 0x00000169, 0x0000006b, 0x00030005,
    0x0000018a, 0x00766f63, 0x00030005, 0x0000018e, 0x00000074, 0x00030005, 0x00000192, 0x0000006b,
    0x00030047, 0x000000c6, 0x0000000e, 0x00040047, 0x000000c6, 0x0000001e, 0x00000004, 0x00040047,
    0x000000ce, 0x0000001e, 0x00000005, 0x00040047, 0x000000d5, 0x0000001e, 0x00000000, 0x00040047,
    0x000000ee, 0x00000021, 0x00000000, 0x00040047, 0x000000ee, 0x00000022, 0x00000000, 0x00040047,
    0x000000f0, 0x0000001e, 0x00000000, 0x00040047, 0x00000100, 0x0000001e, 0x00000001, 0x00030047,
    0x00000101, 0x0000000e, 0x00040047, 0x00000101, 0x0000001e, 0x00000002, 0x00030047, 0x00000102,
    0x0000000e, 0x00040047, 0x00000102, 0x0000001e, 0x00000003, 0x00030047, 0x00000129, 0x0000000e,
    0x00040047, 0x00000129, 0x0000001e, 0x00000006, 0x00020013, 0x00000002, 0x00030021, 0x00000003,
    0x00000002, 0x00030016, 0x00000006, 0x00000020, 0x00040017, 0x00000007, 0x00000006, 0x00000002,
    0x00040020, 0x00000008, 0x00000007, 0x00000007, 0x00040017, 0x00000009, 0x00000006, 0x00000004,
    0x00040020, 0x0000000a, 0x00000007, 0x00000009, 0x00060021, 0x0000000b, 0x00000006, 0x00000008,
    0x00000008, 0x0000000a, 0x00040020, 0x00000011, 0x00000007, 0x00000006, 0x00040021, 0x00000012,
    0x00000006, 0x00000011, 0x00040021, 0x00000016, 0x00000006, 0x00000008, 0x00040015, 0x0000001b,
    0x00000020, 0x00000000, 0x0004002b, 0x0000001b, 0x0000001c, 0x00000000, 0x0004002b, 0x00000006,
    0x0000001f, 0x00000000, 0x00020014, 0x00000020, 0x0004002b, 0x0000001b, 0x00000025, 0x00000001,
    0x0004002b, 0x0000001b, 0x0000002f, 0x00000003, 0x0004002b, 0x0000001b, 0x0000003d, 0x00000002,
    0x0004002b, 0x00000006, 0x00000066, 0x3f800000, 0x0004002b, 0x00000006, 0x00000067, 0x3ea7ba05,
    0x0004002b, 0x00000006, 0x0000006d, 0x3f87dc22, 0x0004002b, 0x00000006, 0x00000070, 0x3fba00e3,
    0x0004002b, 0x00000006, 0x00000074, 0x3fb5f0e3, 0x0004002b, 0x00000006, 0x00000078, 0x3e91a98e,
    0x0004002b, 0x00000006, 0x0000007c, 0x3e827906, 0x00040015, 0x0000008c, 0x00000020, 0x00000001,
    0x00040017, 0x0000008d, 0x0000008c, 0x00000002, 0x00040020, 0x0000008e, 0x00000007, 0x0000008d,
    0x0004002b, 0x00000006, 0x00000092, 0x41000000, 0x00040020, 0x00000096, 0x00000007, 0x0000008c,
    0x0004002b, 0x0000008c, 0x000000a2, 0x00000001, 0x0004002b, 0x0000008c, 0x000000a4, 0x00000005,
    0x0004002b, 0x0000008c, 0x000000a8, 0x00000004, 0x0004002b, 0x0000008c, 0x000000ac, 0x00000002,
    0x0004002b, 0x00000006, 0x000000be, 0x3f000000, 0x0004002b, 0x00000006, 0x000000c0, 0x42800000,
    0x00040020, 0x000000c5, 0x00000001, 0x00000007, 0x0004003b, 0x000000c5, 0x000000c6, 0x00000001,
    0x00040020, 0x000000c7, 0x00000001, 0x00000006, 0x00040020, 0x000000cd, 0x00000001, 0x00000009,
    0x0004003b, 0x000000cd, 0x000000ce, 0x00000001, 0x00040020, 0x000000d4, 0x00000003, 0x00000009,
    0x0004003b, 0x000000d4, 0x000000d5, 0x00000003, 0x00040017, 0x000000d6, 0x00000006, 0x00000003,
    0x0004002b, 0x0000008c, 0x000000e4, 0x00000000, 0x00090019, 0x000000eb, 0x00000006, 0x00000001,
    0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x0003001b, 0x000000ec, 0x000000eb,
    0x00040020, 0x000000ed, 0x00000000, 0x000000ec, 0x0004003b, 0x000000ed, 0x000000ee, 0x00000000,
    0x0004003b, 0x000000c5, 0x000000f0, 0x00000001, 0x0004003b, 0x000000c5, 0x00000100, 0x00000001,
    0x0004003b, 0x000000c5, 0x00000101, 0x00000001, 0x0004003b, 0x000000cd, 0x00000102, 0x00000001,
    0x0004003b, 0x000000cd, 0x00000129, 0x00000001, 0x0004002b, 0x00000006, 0x00000148, 0x3c23d70a,
    0x0004002b, 0x00000006, 0x00000150, 0x3fb504f3, 0x00050036, 0x00000002, 0x00000004, 0x00000000,
    0x00000003, 0x000200f8, 0x00000005, 0x0004003b, 0x00000096, 0x000000c4, 0x00000007, 0x0004003b,
    0x0000000a, 0x000000cc, 0x00000007, 0x0004003b, 0x00000011, 0x000000e8, 0x00000007, 0x0004003b,
    0x00000011, 0x000000ff, 0x00000007, 0x0004003b, 0x00000008, 0x00000103, 0x00000007, 0x0004003b,
    0x00000008, 0x00000105, 0x00000007, 0x0004003b, 0x0000000a, 0x00000107, 0x00000007, 0x0004003b,
    0x00000011, 0x0000010e, 0x00000007, 0x0004003b, 0x00000011, 0x00000112, 0x00000007, 0x0004003b,
    0x00000011, 0x00000115, 0x00000007, 0x0004003b, 0x00000011, 0x00000118, 0x00000007, 0x0004003b,
    0x00000011, 0x00000123, 0x00000007, 0x0004003b, 0x00000011, 0x00000128, 0x00000007, 0x0004003b,
    0x00000011, 0x00000145, 0x00000007, 0x0004003b, 0x00000011, 0x0000014a, 0x00000007, 0x0004003b,
    0x00000011, 0x00000153, 0x00000007, 0x0004003b, 0x00000011, 0x00000165, 0x00000007, 0x0004003b,
    0x00000008, 0x00000166, 0x00000007, 0x0004003b, 0x00000011, 0x00000169, 0x00000007, 0x0004003b,
    0x00000011, 0x00000175, 0x00000007, 0x0004003b, 0x00000011, 0x0000018a, 0x00000007, 0x0004003b,
    0x0000000a, 0x0000018e, 0x00000007, 0x0004003b, 0x00000011, 0x00000192, 0x00000007, 0x00050041,
    0x000000c7, 0x000000c8, 0x000000c6, 0x00000025, 0x0004003d, 0x00000006, 0x000000c9, 0x000000c8,
    0x00050081, 0x00000006, 0x000000ca, 0x000000c9, 0x000000be, 0x0004006e, 0x0000008c, 0x000000cb,
    0x000000ca, 0x0003003e, 0x000000c4, 0x000000cb, 0x0004003d, 0x00000009, 0x000000cf, 0x000000ce,
    0x0003003e, 0x000000cc, 0x000000cf, 0x0004003d, 0x0000008c, 0x000000d0, 0x000000c4, 0x000500aa,
    0x00000020, 0x000000d1, 0x000000d0, 0x000000a8, 0x000300f7, 0x000000d3, 0x00000000, 0x000400fa,
    0x000000d1, 0x000000d2, 0x000000d3, 0x000200f8, 0x000000d2, 0x0004003d, 0x00000009, 0x000000d7,
    0x000000cc, 0x0008004f, 0x000000d6, 0x000000d8, 0x000000d7, 0x000000d7, 0x00000000, 0x00000001,
    0x00000002, 0x00050041, 0x00000011, 0x000000d9, 0x000000cc, 0x0000002f, 0x0004003d, 0x00000006,
    0x000000da, 0x000000d9, 0x0005008e, 0x000000d6, 0x000000db, 0x000000d8, 0x000000da, 0x00050041,
    0x00000011, 0x000000dc, 0x000000cc, 0x0000002f, 0x0004003d, 0x00000006, 0x000000dd, 0x000000dc,
    0x00050051, 0x00000006, 0x000000de, 0x000000db, 0x00000000, 0x00050051, 0x00000006, 0x000000df,
    0x000000db, 0x00000001, 0x00050051, 0x00000006, 0x000000e0, 0x000000db, 0x00000002, 0x00070050,
    0x00000009, 0x000000e1, 0x000000de, 0x000000df, 0x000000e0, 0x000000dd, 0x0003003e, 0x000000d5,
    0x000000e1, 0x000100fd, 0x000200f8, 0x000000d3, 0x0004003d, 0x0000008c, 0x000000e3, 0x000000c4,
    0x000500aa, 0x00000020, 0x000000e5, 0x000000e3, 0x000000e4, 0x000300f7, 0x000000e7, 0x00000000,
    0x000400fa, 0x000000e5, 0x000000e6, 0x000000e7, 0x000200f8, 0x000000e6, 0x00050041, 0x00000011,
    0x000000e9, 0x000000cc, 0x0000002f, 0x0004003d, 0x00000006, 0x000000ea, 0x000000e9, 0x0004003d,
    0x000000ec, 0x000000ef, 0x000000ee, 0x0004003d, 0x00000007, 0x000000f1, 0x000000f0, 0x00050057,
    0x00000009, 0x000000f2, 0x000000ef, 0x000000f1, 0x00050051, 0x00000006, 0x000000f3, 0x000000f2,
    0x00000000, 0x00050085, 0x00000006, 0x000000f4, 0x000000ea, 0x000000f3, 0x0003003e, 0x000000e8,
    0x000000f4, 0x0004003d, 0x00000009, 0x000000f5, 0x000000cc, 0x0008004f, 0x000000d6, 0x000000f6,
    0x000000f5, 0x000000f5, 0x00000000, 0x00000001, 0x00000002, 0x0004003d, 0x00000006, 0x000000f7,
    0x000000e8, 0x0005008e, 0x000000d6, 0x000000f8, 0x000000f6, 0x000000f7, 0x0004003d, 0x00000006,
    0x000000f9, 0x000000e8, 0x00050051, 0x00000006, 0x000000fa, 0x000000f8, 0x00000000, 0x00050051,
    0x00000006, 0x000000fb, 0x000000f8, 0x00000001, 0x00050051, 0x00000006, 0x000000fc, 0x000000f8,
    0x00000002, 0x00070050, 0x00000009, 0x000000fd, 0x000000fa, 0x000000fb, 0x000000fc, 0x000000f9,
    0x0003003e, 0x000000d5, 0x000000fd, 0x000100fd, 0x000200f8, 0x000000e7, 0x0004003d, 0x00000007,
    0x00000104, 0x00000100, 0x0003003e, 0x00000103, 0x00000104, 0x0004003d, 0x00000007, 0x00000106,
    0x00000101, 0x0003003e, 0x00000105, 0x00000106, 0x0004003d, 0x00000009, 0x00000108, 0x00000102,
    0x0003003e, 0x00000107, 0x00000108, 0x00070039, 0x00000006, 0x00000109, 0x0000000f, 0x00000103,
    0x00000105, 0x00000107, 0x0003003e, 0x000000ff, 0x00000109, 0x0004003d, 0x0000008c, 0x0000010a,
    0x000000c4, 0x000500aa, 0x00000020, 0x0000010b, 0x0000010a, 0x000000a2, 0x000300f7, 0x0000010d,
    0x00000000, 0x000400fa, 0x0000010b, 0x0000010c, 0x00000140, 0x000200f8, 0x0000010c, 0x0004003d,
    0x00000006, 0x0000010f, 0x000000ff, 0x00050083, 0x00000006, 0x00000110, 0x000000be, 0x0000010f,
    0x0008000c, 0x00000006, 0x00000111, 0x00000001, 0x0000002b, 0x00000110, 0x0000001f, 0x00000066,
    0x0003003e, 0x0000010e, 0x00000111, 0x00050041, 0x000000c7, 0x00000113, 0x000000c6, 0x0000001c,
    0x0004003d, 0x00000006, 0x00000114, 0x00000113, 0x0003003e, 0x00000112, 0x00000114, 0x0004003d,
    0x00000006, 0x00000116, 0x00000112, 0x000500ba, 0x00000020, 0x00000117, 0x00000116, 0x0000001f,
    0x000300f7, 0x0000011a, 0x00000000, 0x000400fa, 0x00000117, 0x00000119, 0x00000120, 0x000200f8,
    0x00000119, 0x0004003d, 0x00000006, 0x0000011b, 0x000000ff, 0x0004003d, 0x00000006, 0x0000011c,
    0x00000112, 0x00050081, 0x00000006, 0x0000011d, 0x0000011b, 0x0000011c, 0x00050083, 0x00000006,
    0x0000011e, 0x000000be, 0x0000011d, 0x0008000c, 0x00000006, 0x0000011f, 0x00000001, 0x0000002b,
    0x0000011e, 0x0000001f, 0x00000066, 0x0003003e, 0x00000118, 0x0000011f, 0x000200f9, 0x0000011a,
    0x000200f8, 0x00000120, 0x0004003d, 0x00000006, 0x00000121, 0x0000010e, 0x0003003e, 0x00000118,
    0x00000121, 0x000200f9, 0x0000011a, 0x000200f8, 0x0000011a, 0x0004003d, 0x00000006, 0x00000122,
    0x00000118, 0x0003003e, 0x00000115, 0x00000122, 0x00050041, 0x00000011, 0x00000124, 0x000000cc,
    0x0000002f, 0x0004003d, 0x00000006, 0x00000125, 0x00000124, 0x0004003d, 0x00000006, 0x00000126,
    0x00000115, 0x00050085, 0x00000006, 0x00000127, 0x00000125, 0x00000126, 0x0003003e, 0x00000123,
    0x00000127, 0x00050041, 0x000000c7, 0x0000012a, 0x00000129, 0x0000002f, 0x0004003d, 0x00000006,
    0x0000012b, 0x0000012a, 0x0004003d, 0x00000006, 0x0000012c, 0x0000010e, 0x0004003d, 0x00000006,
    0x0000012d, 0x00000115, 0x00050083, 0x00000006, 0x0000012e, 0x0000012c, 0x0000012d, 0x00050085,
    0x00000006, 0x0000012f, 0x0000012b, 0x0000012e, 0x0003003e, 0x00000128, 0x0000012f, 0x0004003d,
    0x00000009, 0x00000130, 0x000000cc, 0x0008004f, 0x000000d6, 0x00000131, 0x00000130, 0x00000130,
    0x00000000, 0x00000001, 0x00000002, 0x0004003d, 0x00000006, 0x00000132, 0x00000123, 0x0005008e,
    0x000000d6, 0x00000133, 0x00000131, 0x00000132, 0x0004003d, 0x00000009, 0x00000134, 0x00000129,
    0x0008004f, 0x000000d6, 0x00000135, 0x00000134, 0x00000134, 0x00000000, 0x00000001, 0x00000002,
    0x0004003d, 0x00000006, 0x00000136, 0x00000128, 0x0005008e, 0x000000d6, 0x00000137, 0x00000135,
    0x00000136, 0x00050081, 0x000000d6, 0x00000138, 0x00000133, 0x00000137, 0x0004003d, 0x00000006,
    0x00000139, 0x00000123, 0x0004003d, 0x00000006, 0x0000013a, 0x00000128, 0x00050081, 0x00000006,
    0x0000013b, 0x00000139, 0x0000013a, 0x00050051, 0x00000006, 0x0000013c, 0x00000138, 0x00000000,
    0x00050051, 0x00000006, 0x0000013d, 0x00000138, 0x00000001, 0x00050051, 0x00000006, 0x0000013e,
    0x00000138, 0x00000002, 0x00070050, 0x00000009, 0x0000013f, 0x0000013c, 0x0000013d, 0x0000013e,
    0x0000013b, 0x0003003e, 0x000000d5, 0x0000013f, 0x000200f9, 0x0000010d, 0x000200f8, 0x00000140,
    0x0004003d, 0x0000008c, 0x00000141, 0x000000c4, 0x000500aa, 0x00000020, 0x00000142, 0x00000141,
    0x000000ac, 0x000300f7, 0x00000144, 0x00000000, 0x000400fa, 0x00000142, 0x00000143, 0x00000160,
    0x000200f8, 0x00000143, 0x00050041, 0x000000c7, 0x00000146, 0x000000c6, 0x0000001c, 0x0004003d,
    0x00000006, 0x00000147, 0x00000146, 0x0007000c, 0x00000006, 0x00000149, 0x00000001, 0x00000028,
    0x00000147, 0x00000148, 0x0003003e, 0x00000145, 0x00000149, 0x00050041, 0x00000011, 0x0000014b,
    0x000000cc, 0x0000002f, 0x0004003d, 0x00000006, 0x0000014c, 0x0000014b, 0x00050085, 0x00000006,
    0x0000014d, 0x0000014c, 0x000000be, 0x0004003d, 0x00000006, 0x0000014e, 0x000000ff, 0x0004003d,
    0x00000006, 0x0000014f, 0x00000145, 0x00050085, 0x00000006, 0x00000151, 0x0000014f, 0x00000150,
    0x00050088, 0x00000006, 0x00000152, 0x0000014e, 0x00000151, 0x0003003e, 0x00000153, 0x00000152,
    0x00050039, 0x00000006, 0x00000154, 0x00000014, 0x00000153, 0x00050083, 0x00000006, 0x00000155,
    0x00000066, 0x00000154, 0x00050085, 0x00000006, 0x00000156, 0x0000014d, 0x00000155, 0x0003003e,
    0x0000014a, 0x00000156, 0x0004003d, 0x00000009, 0x00000157, 0x000000cc, 0x0008004f, 0x000000d6,
    0x00000158, 0x00000157, 0x00000157, 0x00000000, 0x00000001, 0x00000002, 0x0004003d, 0x00000006,
    0x00000159, 0x0000014a, 0x0005008e, 0x000000d6, 0x0000015a, 0x00000158, 0x00000159, 0x0004003d,
    0x00000006, 0x0000015b, 0x0000014a, 0x00050051, 0x00000006, 0x0000015c, 0x0000015a, 0x00000000,
    0x00050051, 0x00000006, 0x0000015d, 0x0000015a, 0x00000001, 0x00050051, 0x00000006, 0x0000015e,
    0x0000015a, 0x00000002, 0x00070050, 0x00000009, 0x0000015f, 0x0000015c, 0x0000015d, 0x0000015e,
    0x0000015b, 0x0003003e, 0x000000d5, 0x0000015f, 0x000200f9, 0x00000144, 0x000200f8, 0x00000160,
    0x0004003d, 0x0000008c, 0x00000161, 0x000000c4, 0x000500aa, 0x00000020, 0x00000162, 0x00000161,
    0x000000a4, 0x000300f7, 0x00000164, 0x00000000, 0x000400fa, 0x00000162, 0x00000163, 0x00000189,
    0x000200f8, 0x00000163, 0x0004003d, 0x00000007, 0x00000167, 0x000000f0, 0x0003003e, 0x00000166,
    0x00000167, 0x00050039, 0x00000006, 0x00000168, 0x00000018, 0x00000166, 0x0003003e, 0x00000165,
    0x00000168, 0x0004003d, 0x00000006, 0x0000016a, 0x00000165, 0x00050041, 0x000000c7, 0x0000016b,
    0x000000c6, 0x0000001c, 0x0004003d, 0x00000006, 0x0000016c, 0x0000016b, 0x000500be, 0x00000020,
    0x0000016d, 0x0000016a, 0x0000016c, 0x000300f7, 0x0000016f, 0x00000000, 0x000400fa, 0x0000016d,
    0x0000016e, 0x0000016f, 0x000200f8, 0x0000016e, 0x0004003d, 0x00000006, 0x00000170, 0x00000165,
    0x00050041, 0x00000011, 0x00000171, 0x000000cc, 0x0000002f, 0x0004003d, 0x00000006, 0x00000172,
    0x00000171, 0x000500b8, 0x00000020, 0x00000173, 0x00000170, 0x00000172, 0x000200f9, 0x0000016f,
    0x000200f8, 0x0000016f, 0x000700f5, 0x00000020, 0x00000174, 0x0000016d, 0x00000163, 0x00000173,
    0x0000016e, 0x000300f7, 0x00000177, 0x00000000, 0x000400fa, 0x00000174, 0x00000176, 0x0000017e,
    0x000200f8, 0x00000176, 0x00050041, 0x000000c7, 0x00000178, 0x00000129, 0x0000002f, 0x0004003d,
    0x00000006, 0x00000179, 0x00000178, 0x0004003d, 0x00000006, 0x0000017a, 0x000000ff, 0x00050083,
    0x00000006, 0x0000017b, 0x000000be, 0x0000017a, 0x0008000c, 0x00000006, 0x0000017c, 0x00000001,
    0x0000002b, 0x0000017b, 0x0000001f, 0x00000066, 0x00050085, 0x00000006, 0x0000017d, 0x00000179,
    0x0000017c, 0x0003003e, 0x00000175, 0x0000017d, 0x000200f9, 0x00000177, 0x000200f8, 0x0000017e,
    0x0003003e, 0x00000175, 0x0000001f, 0x000200f9, 0x00000177, 0x000200f8, 0x00000177, 0x0004003d,
    0x00000006, 0x0000017f, 0x00000175, 0x0003003e, 0x00000169, 0x0000017f, 0x0004003d, 0x00000009,
    0x00000180, 0x00000129, 0x0008004f, 0x000000d6, 0x00000181, 0x00000180, 0x00000180, 0x00000000,
    0x00000001, 0x00000002, 0x0004003d, 0x00000006, 0x00000182, 0x00000169, 0x0005008e, 0x000000d6,
    0x00000183, 0x00000181, 0x00000182, 0x0004003d, 0x00000006, 0x00000184, 0x00000169, 0x00050051,
    0x00000006, 0x00000185, 0x00000183, 0x00000000, 0x00050051, 0x00000006, 0x00000186, 0x00000183,
    0x00000001, 0x00050051, 0x00000006, 0x00000187, 0x00000183, 0x00000002, 0x00070050, 0x00000009,
    0x00000188, 0x00000185, 0x00000186, 0x00000187, 0x00000184, 0x0003003e, 0x000000d5, 0x00000188,
    0x000200f9, 0x00000164, 0x000200f8, 0x00000189, 0x0004003d, 0x00000006, 0x0000018b, 0x000000ff,
    0x00050083, 0x00000006, 0x0000018c, 0x000000be, 0x0000018b, 0x0008000c, 0x00000006, 0x0000018d,
    0x00000001, 0x0000002b, 0x0000018c, 0x0000001f, 0x00000066, 0x0003003e, 0x0000018a, 0x0000018d,
    0x0004003d, 0x000000ec, 0x0000018f, 0x000000ee, 0x0004003d, 0x00000007, 0x00000190, 0x000000f0,
    0x00050057, 0x00000009, 0x00000191, 0x0000018f, 0x00000190, 0x0003003e, 0x0000018e, 0x00000191,
    0x00050041, 0x00000011, 0x00000193, 0x000000cc, 0x0000002f, 0x0004003d, 0x00000006, 0x00000194,
    0x00000193, 0x0004003d, 0x00000006, 0x00000195, 0x0000018a, 0x00050085, 0x00000006, 0x00000196,
    0x00000194, 0x00000195, 0x0003003e, 0x00000192, 0x00000196, 0x0004003d, 0x00000009, 0x00000197,
    0x0000018e, 0x0008004f, 0x000000d6, 0x00000198, 0x00000197, 0x00000197, 0x00000000, 0x00000001,
    0x00000002, 0x00050041, 0x00000011, 0x00000199, 0x0000018e, 0x0000002f, 0x0004003d, 0x00000006,
    0x0000019a, 0x00000199, 0x0005008e, 0x000000d6, 0x0000019b, 0x00000198, 0x0000019a, 0x0004003d,
    0x00000009, 0x0000019c, 0x000000cc, 0x0008004f, 0x000000d6, 0x0000019d, 0x0000019c, 0x0000019c,
    0x00000000, 0x00000001, 0x00000002, 0x00050085, 0x000000d6, 0x0000019e, 0x0000019b, 0x0000019d,
    0x0004003d, 0x00000006, 0x0000019f, 0x00000192, 0x0005008e, 0x000000d6, 0x000001a0, 0x0000019e,
    0x0000019f, 0x00050041, 0x00000011, 0x000001a1, 0x0000018e, 0x0000002f, 0x0004003d, 0x00000006,
    0x000001a2, 0x000001a1, 0x0004003d, 0x00000006, 0x000001a3, 0x00000192, 0x00050085, 0x00000006,
    0x000001a4, 0x000001a2, 0x000001a3, 0x00050051, 0x00000006, 0x000001a5, 0x000001a0, 0x00000000,
    0x00050051, 0x00000006, 0x000001a6, 0x000001a0, 0x00000001, 0x00050051, 0x00000006, 0x000001a7,
    0x000001a0, 0x00000002, 0x00070050, 0x00000009, 0x000001a8, 0x000001a5, 0x000001a6, 0x000001a7,
    0x000001a4, 0x0003003e, 0x000000d5, 0x000001a8, 0x000200f9, 0x00000164, 0x000200f8, 0x00000164,
    0x000200f9, 0x00000144, 0x000200f8, 0x00000144, 0x000200f9, 0x0000010d, 0x000200f8, 0x0000010d,
    0x000100fd, 0x00010038, 0x00050036, 0x00000006, 0x0000000f, 0x00000000, 0x0000000b, 0x00030037,
    0x00000008, 0x0000000c, 0x00030037, 0x00000008, 0x0000000d, 0x00030037, 0x0000000a, 0x0000000e,
    0x000200f8, 0x00000010, 0x0004003b, 0x00000011, 0x0000001a, 0x00000007, 0x0004003b, 0x00000011,
    0x00000022, 0x00000007, 0x0004003b, 0x00000011, 0x00000029, 0x00000007, 0x0004003b, 0x00000011,
    0x00000037, 0x00000007, 0x0004003b, 0x00000008, 0x00000049, 0x00000007, 0x00050041, 0x00000011,
    0x0000001d, 0x0000000c, 0x0000001c, 0x0004003d, 0x00000006, 0x0000001e, 0x0000001d, 0x000500b8,
    0x00000020, 0x00000021, 0x0000001e, 0x0000001f, 0x000300f7, 0x00000024, 0x00000000, 0x000400fa,
    0x00000021, 0x00000023, 0x00000033, 0x000200f8, 0x00000023, 0x00050041, 0x00000011, 0x00000026,
    0x0000000c, 0x00000025, 0x0004003d, 0x00000006, 0x00000027, 0x00000026, 0x000500b8, 0x00000020,
    0x00000028, 0x00000027, 0x0000001f, 0x000300f7, 0x0000002b, 0x00000000, 0x000400fa, 0x00000028,
    0x0000002a, 0x0000002e, 0x000200f8, 0x0000002a, 0x00050041, 0x00000011, 0x0000002c, 0x0000000e,
    0x0000001c, 0x0004003d, 0x00000006, 0x0000002d, 0x0000002c, 0x0003003e, 0x00000029, 0x0000002d,
    0x000200f9, 0x0000002b, 0x000200f8, 0x0000002e, 0x00050041, 0x00000011, 0x00000030, 0x0000000e,
    0x0000002f, 0x0004003d, 0x00000006, 0x00000031, 0x00000030, 0x0003003e, 0x00000029, 0x00000031,
    0x000200f9, 0x0000002b, 0x000200f8, 0x0000002b, 0x0004003d, 0x00000006, 0x00000032, 0x00000029,
    0x0003003e, 0x00000022, 0x00000032, 0x000200f9, 0x00000024, 0x000200f8, 0x00000033, 0x00050041,
    0x00000011, 0x00000034, 0x0000000c, 0x00000025, 0x0004003d, 0x00000006, 0x00000035, 0x00000034,
    0x000500b8, 0x00000020, 0x00000036, 0x00000035, 0x0000001f, 0x000300f7, 0x00000039, 0x00000000,
    0x000400fa, 0x00000036, 0x00000038, 0x0000003c, 0x000200f8, 0x00000038, 0x00050041, 0x00000011,
    0x0000003a, 0x0000000e, 0x00000025, 0x0004003d, 0x00000006, 0x0000003b, 0x0000003a, 0x0003003e,
    0x00000037, 0x0000003b, 0x000200f9, 0x00000039, 0x000200f8, 0x0000003c, 0x00050041, 0x00000011,
    0x0000003e, 0x0000000e, 0x0000003d, 0x0004003d, 0x00000006, 0x0000003f, 0x0000003e, 0x0003003e,
    0x00000037, 0x0000003f, 0x000200f9, 0x00000039, 0x000200f8, 0x00000039, 0x0004003d, 0x00000006,
    0x00000040, 0x00000037, 0x0003003e, 0x00000022, 0x00000040, 0x000200f9, 0x00000024, 0x000200f8,
    0x00000024, 0x0004003d, 0x00000006, 0x00000041, 0x00000022, 0x0003003e, 0x0000001a, 0x00000041,
    0x0004003d, 0x00000006, 0x00000042, 0x0000001a, 0x00050041, 0x00000011, 0x00000043, 0x0000000d,
    0x0000001c, 0x0004003d, 0x00000006, 0x00000044, 0x00000043, 0x00050041, 0x00000011, 0x00000045,
    0x0000000d, 0x00000025, 0x0004003d, 0x00000006, 0x00000046, 0x00000045, 0x0007000c, 0x00000006,
    0x00000047, 0x00000001, 0x00000025, 0x00000044, 0x00000046, 0x0007000c, 0x00000006, 0x00000048,
    0x00000001, 0x00000025, 0x00000042, 0x00000047, 0x0003003e, 0x0000001a, 0x00000048, 0x0004003d,
    0x00000007, 0x0000004a, 0x0000000c, 0x0006000c, 0x00000007, 0x0000004b, 0x00000001, 0x00000004,
    0x0000004a, 0x0004003d, 0x00000007, 0x0000004c, 0x0000000d, 0x00050083, 0x00000007, 0x0000004d,
    0x0000004b, 0x0000004c, 0x0004003d, 0x00000006, 0x0000004e, 0x0000001a, 0x00050050, 0x00000007,
    0x0000004f, 0x0000004e, 0x0000004e, 0x00050081, 0x00000007, 0x00000050, 0x0000004d, 0x0000004f,
    0x0003003e, 0x00000049, 0x00000050, 0x00050041, 0x00000011, 0x00000051, 0x00000049, 0x0000001c,
    0x0004003d, 0x00000006, 0x00000052, 0x00000051, 0x00050041, 0x00000011, 0x00000053, 0x00000049,
    0x00000025, 0x0004003d, 0x00000006, 0x00000054, 0x00000053, 0x0007000c, 0x00000006, 0x00000055,
    0x00000001, 0x00000028, 0x00000052, 0x00000054, 0x0007000c, 0x00000006, 0x00000056, 0x00000001,
    0x00000025, 0x00000055, 0x0000001f, 0x0004003d, 0x00000007, 0x00000057, 0x00000049, 0x00050050,
    0x00000007, 0x00000058, 0x0000001f, 0x0000001f, 0x0007000c, 0x00000007, 0x00000059, 0x00000001,
    0x00000028, 0x00000057, 0x00000058, 0x0006000c, 0x00000006, 0x0000005a, 0x00000001, 0x00000042,
    0x00000059, 0x00050081, 0x00000006, 0x0000005b, 0x00000056, 0x0000005a, 0x0004003d, 0x00000006,
    0x0000005c, 0x0000001a, 0x00050083, 0x00000006, 0x0000005d, 0x0000005b, 0x0000005c, 0x000200fe,
    0x0000005d, 0x00010038, 0x00050036, 0x00000006, 0x00000014, 0x00000000, 0x00000012, 0x00030037,
    0x00000011, 0x00000013, 0x000200f8, 0x00000015, 0x0004003b, 0x00000011, 0x00000060, 0x00000007,
    0x0004003b, 0x00000011, 0x00000065, 0x00000007, 0x0004003b, 0x00000011, 0x0000006c, 0x00000007,
    0x0004003d, 0x00000006, 0x00000061, 0x00000013, 0x0006000c, 0x00000006, 0x00000062, 0x00000001,
    0x00000006, 0x00000061, 0x0003003e, 0x00000060, 0x00000062, 0x0004003d, 0x00000006, 0x00000063,
    0x00000013, 0x0006000c, 0x00000006, 0x00000064, 0x00000001, 0x00000004, 0x00000063, 0x0003003e,
    0x00000013, 0x00000064, 0x0004003d, 0x00000006, 0x00000068, 0x00000013, 0x00050085, 0x00000006,
    0x00000069, 0x00000067, 0x00000068, 0x00050081, 0x00000006, 0x0000006a, 0x00000066, 0x00000069,
    0x00050088, 0x00000006, 0x0000006b, 0x00000066, 0x0000006a, 0x0003003e, 0x00000065, 0x0000006b,
    0x0004003d, 0x00000006, 0x0000006e, 0x00000065, 0x00050085, 0x00000006, 0x0000006f, 0x0000006d,
    0x0000006e, 0x00050083, 0x00000006, 0x00000071, 0x0000006f, 0x00000070, 0x0004003d, 0x00000006,
    0x00000072, 0x00000065, 0x00050085, 0x00000006, 0x00000073, 0x00000071, 0x00000072, 0x00050081,
    0x00000006, 0x00000075, 0x00000073, 0x00000074, 0x0004003d, 0x00000006, 0x00000076, 0x00000065,
    0x00050085, 0x00000006, 0x00000077, 0x00000075, 0x00000076, 0x00050083, 0x00000006, 0x00000079,
    0x00000077, 0x00000078, 0x0004003d, 0x00000006, 0x0000007a, 0x00000065, 0x00050085, 0x00000006,
    0x0000007b, 0x00000079, 0x0000007a, 0x00050081, 0x00000006, 0x0000007d, 0x0000007b, 0x0000007c,
    0x0004003d, 0x00000006, 0x0000007e, 0x00000065, 0x00050085, 0x00000006, 0x0000007f, 0x0000007d,
    0x0000007e, 0x0004003d, 0x00000006, 0x00000080, 0x00000013, 0x0004007f, 0x00000006, 0x00000081,
    0x00000080, 0x0004003d, 0x00000006, 0x00000082, 0x00000013, 0x00050085, 0x00000006, 0x00000083,
    0x00000081, 0x00000082, 0x0006000c, 0x00000006, 0x00000084, 0x00000001, 0x0000001b, 0x00000083,
    0x00050085, 0x00000006, 0x00000085, 0x0000007f, 0x00000084, 0x00050083, 0x00000006, 0x00000086,
    0x00000066, 0x00000085, 0x0003003e, 0x0000006c, 0x00000086, 0x0004003d, 0x00000006, 0x00000087,
    0x00000060, 0x0004003d, 0x00000006, 0x00000088, 0x0000006c, 0x00050085, 0x00000006, 0x00000089,
    0x00000087, 0x00000088, 0x000200fe, 0x00000089, 0x00010038, 0x00050036, 0x00000006, 0x00000018,
    0x00000000, 0x00000016, 0x00030037, 0x00000008, 0x00000017, 0x000200f8, 0x00000019, 0x0004003b,
    0x0000008e, 0x0000008f, 0x00000007, 0x0004003b, 0x00000096, 0x00000097, 0x00000007, 0x0004003b,
    0x00000096, 0x0000009d, 0x00000007, 0x0004003b, 0x00000096, 0x000000a0, 0x00000007, 0x0004003d,
    0x00000007, 0x00000090, 0x00000017, 0x0006000c, 0x00000007, 0x00000091, 0x00000001, 0x00000008,
    0x00000090, 0x00050050, 0x00000007, 0x00000093, 0x00000092, 0x00000092, 0x0005008d, 0x00000007,
    0x00000094, 0x00000091, 0x00000093, 0x0004006e, 0x0000008d, 0x00000095, 0x00000094, 0x0003003e,
    0x0000008f, 0x00000095, 0x00050041, 0x00000096, 0x00000098, 0x0000008f, 0x0000001c, 0x0004003d,
    0x0000008c, 0x00000099, 0x00000098, 0x00050041, 0x00000096, 0x0000009a, 0x0000008f, 0x00000025,
    0x0004003d, 0x0000008c, 0x0000009b, 0x0000009a, 0x000500c6, 0x0000008c, 0x0000009c, 0x00000099,
    0x0000009b, 0x0003003e, 0x00000097, 0x0000009c, 0x00050041, 0x00000096, 0x0000009e, 0x0000008f,
    0x00000025, 0x0004003d, 0x0000008c, 0x0000009f, 0x0000009e, 0x0003003e, 0x0000009d, 0x0000009f,
    0x0004003d, 0x0000008c, 0x000000a1, 0x00000097, 0x000500c7, 0x0000008c, 0x000000a3, 0x000000a1,
    0x000000a2, 0x000500c4, 0x0000008c, 0x000000a5, 0x000000a3, 0x000000a4, 0x0004003d, 0x0000008c,
    0x000000a6, 0x0000009d, 0x000500c7, 0x0000008c, 0x000000a7, 0x000000a6, 0x000000a2, 0x000500c4,
    0x0000008c, 0x000000a9, 0x000000a7, 0x000000a8, 0x000500c5, 0x0000008c, 0x000000aa, 0x000000a5,
    0x000000a9, 0x0004003d, 0x0000008c, 0x000000ab, 0x00000097, 0x000500c7, 0x0000008c, 0x000000ad,
    0x000000ab, 0x000000ac, 0x000500c4, 0x0000008c, 0x000000ae, 0x000000ad, 0x000000ac, 0x000500c5,
    0x0000008c, 0x000000af, 0x000000aa, 0x000000ae, 0x0004003d, 0x0000008c, 0x000000b0, 0x0000009d,
    0x000500c7, 0x0000008c, 0x000000b1, 0x000000b0, 0x000000ac, 0x000500c4, 0x0000008c, 0x000000b2,
    0x000000b1, 0x000000a2, 0x000500c5, 0x0000008c, 0x000000b3, 0x000000af, 0x000000b2, 0x0004003d,
    0x0000008c, 0x000000b4, 0x00000097, 0x000500c7, 0x0000008c, 0x000000b5, 0x000000b4, 0x000000a8,
    0x000500c3, 0x0000008c, 0x000000b6, 0x000000b5, 0x000000a2, 0x000500c5, 0x0000008c, 0x000000b7,
    0x000000b3, 0x000000b6, 0x0004003d, 0x0000008c, 0x000000b8, 0x0000009d, 0x000500c7, 0x0000008c,
    0x000000b9, 0x000000b8, 0x000000a8, 0x000500c3, 0x0000008c, 0x000000ba, 0x000000b9, 0x000000ac,
    0x000500c5, 0x0000008c, 0x000000bb, 0x000000b7, 0x000000ba, 0x0003003e, 0x000000a0, 0x000000bb,
    0x0004003d, 0x0000008c, 0x000000bc, 0x000000a0, 0x0004006f, 0x00000006, 0x000000bd, 0x000000bc,
    0x00050081, 0x00000006, 0x000000bf, 0x000000bd, 0x000000be, 0x00050088, 0x00000006, 0x000000c1,
    0x000000bf, 0x000000c0, 0x000200fe, 0x000000c1, 0x00010038,
};

/* Instanced quad vertex shader (cui_quad.vert)

#version 450
layout(location = 0) in vec4 a_rect;
layout(location = 1) in vec4 a_uv;
layout(location = 2) in vec4 a_local;
layout(location = 3) in vec2 a_half;
layout(location = 4) in vec4 a_radii;
layout(location = 5) in vec2 a_params;
layout(location = 6) in vec4 a_top;
layout(location = 7) in vec4 a_bottom;
layout(location = 8) in vec4 a_bcolor;
layout(push_constant) uniform Frame { vec2 u_viewport; };
layout(location = 0) out vec2 v_uv;
layout(location = 1) out vec2 v_local;
layout(location = 2) flat out vec2 v_half;
layout(location = 3) flat out vec4 v_radii;
layout(location = 4) flat out vec2 v_params;
layout(location = 5) out vec4 v_color;
layout(location = 6) flat out vec4 v_bcolor;
void main() {
  // TR, BR, TL, BL. Both triangles use the original TL-BR diagonal.
  bool right = gl_VertexIndex < 2;
  bool bottom = (gl_VertexIndex & 1) != 0;
  vec2 pos = vec2(right ? a_rect.z : a_rect.x, bottom ? a_rect.w : a_rect.y);
  v_uv = vec2(right ? a_uv.z : a_uv.x, bottom ? a_uv.w : a_uv.y);
  v_local = vec2(right ? a_local.z : a_local.x, bottom ? a_local.w : a_local.y);
  v_half = a_half;
  v_radii = a_radii;
  v_params = a_params;
  v_color = bottom ? a_bottom : a_top;
  v_bcolor = a_bcolor;
  gl_Position = vec4(pos.x / u_viewport.x * 2.0 - 1.0, pos.y / u_viewport.y * 2.0 - 1.0, 0.0, 1.0);
}
*/
static const uint32_t cui_vk_quad_vert[] = {
    0x07230203, 0x00010000, 0x0008000b, 0x00000099, 0x00000000, 0x00020011, 0x00000001, 0x0006000b,
    0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001,
    0x0017000f, 0x00000000, 0x00000004, 0x6e69616d, 0x00000000, 0x0000000b, 0x00000020, 0x00000039,
    0x0000003e, 0x00000050, 0x00000055, 0x00000067, 0x00000069, 0x0000006c, 0x0000006d, 0x0000006f,
    0x00000070, 0x00000072, 0x00000074, 0x00000076, 0x0000007b, 0x0000007c, 0x00000081, 0x00030003,
    0x00000002, 0x000001c2, 0x00040005, 0x00000004, 0x6e69616d, 0x00000000, 0x00040005, 0x00000008,
    0x68676972, 0x00000074, 0x00060005, 0x0000000b, 0x565f6c67, 0x65747265, 0x646e4978, 0x00007865,
    0x00040005, 0x0000000f, 0x74746f62, 0x00006d6f, 0x00030005, 0x00000018, 0x00736f70, 0x00040005,
    0x00000020, 0x65725f61, 0x00007463, 0x00040005, 0x00000039, 0x76755f76, 0x00000000, 0x00040005,
    0x0000003e, 0x76755f61, 0x00000000, 0x00040005, 0x00000050, 0x6f6c5f76, 0x006c6163, 0x00040005,
    0x00000055, 0x6f6c5f61, 0x006c6163, 0x00040005, 0x00000067, 0x61685f76, 0x0000666c, 0x00040005,
    0x00000069, 0x61685f61, 0x0000666c, 0x00040005, 0x0000006c, 0x61725f76, 0x00696964, 0x00040005,
    0x0000006d, 0x61725f61, 0x00696964, 0x00050005, 0x0000006f, 0x61705f76, 0x736d6172, 0x00000000,
    0x00050005, 0x00000070, 0x61705f61, 0x736d6172, 0x00000000, 0x00040005, 0x00000072, 0x6f635f76,
    0x00726f6c, 0x00050005, 0x00000074, 0x6f625f61, 0x6d6f7474, 0x00000000, 0x00040005, 0x00000076,
    0x6f745f61, 0x00000070, 0x00050005, 0x0000007b, 0x63625f76, 0x726f6c6f, 0x00000000, 0x00050005,
    0x0000007c, 0x63625f61, 0x726f6c6f, 0x00000000, 0x00060005, 0x0000007f, 0x505f6c67, 0x65567265,
    0x78657472, 0x00000000, 0x00060006, 0x0000007f, 0x00000000, 0x505f6c67, 0x7469736f, 0x006e6f69,
    0x00070006, 0x0000007f, 0x00000001, 0x505f6c67, 0x746e696f, 0x657a6953, 0x00000000, 0x00070006,
    0x0000007f, 0x00000002, 0x435f6c67, 0x4470696c, 0x61747369, 0x0065636e, 0x00070006, 0x0000007f,
    0x00000003, 0x435f6c67, 0x446c6c75, 0x61747369, 0x0065636e, 0x00030005, 0x00000081, 0x00000000,
    0x00040005, 0x00000084, 0x6d617246, 0x00000065, 0x00060006, 0x00000084, 0x00000000, 0x69765f75,
    0x6f707765, 0x00007472, 0x00030005, 0x00000086, 0x00000000, 0x00040047, 0x0000000b, 0x0000000b,
    0x0000002a, 0x00040047, 0x00000020, 0x0000001e, 0x00000000, 0x00040047, 0x00000039, 0x0000001e,
    0x00000000, 0x00040047, 0x0000003e, 0x0000001e, 0x00000001, 0x00040047, 0x00000050, 0x0000001e,
    0x00000001, 0x00040047, 0x00000055, 0x0000001e, 0x00000002, 0x00030047, 0x00000067, 0x0000000e,
    0x00040047, 0x00000067, 0x0000001e, 0x00000002, 0x00040047, 0x00000069, 0x0000001e, 0x00000003,
    0x00030047, 0x0000006c, 0x0000000e, 0x00040047, 0x0000006c, 0x0000001e, 0x00000003, 0x00040047,
    0x0000006d, 0x0000001e, 0x00000004, 0x00030047, 0x0000006f, 0x0000000e, 0x00040047, 0x0000006f,
    0x0000001e, 0x00000004, 0x00040047, 0x00000070, 0x0000001e, 0x00000005, 0x00040047, 0x00000072,
    0x0000001e, 0x00000005, 0x00040047, 0x00000074, 0x0000001e, 0x00000007, 0x00040047, 0x00000076,
    0x0000001e, 0x00000006, 0x00030047, 0x0000007b, 0x0000000e, 0x00040047, 0x0000007b, 0x0000001e,
    0x00000006, 0x00040047, 0x0000007c, 0x0000001e, 0x00000008, 0x00030047, 0x0000007f, 0x00000002,
    0x00050048, 0x0000007f, 0x00000000, 0x0000000b, 0x00000000, 0x00050048, 0x0000007f, 0x00000001,
    0x0000000b, 0x00000001, 0x00050048, 0x0000007f, 0x00000002, 0x0000000b, 0x00000003, 0x00050048,
    0x0000007f, 0x00000003, 0x0000000b, 0x00000004, 0x00030047, 0x00000084, 0x00000002, 0x00050048,
    0x00000084, 0x00000000, 0x00000023, 0x00000000, 0x00020013, 0x00000002, 0x00030021, 0x00000003,
    0x00000002, 0x00020014, 0x00000006, 0x00040020, 0x00000007, 0x00000007, 0x00000006, 0x00040015,
    0x00000009, 0x00000020, 0x00000001, 0x00040020, 0x0000000a, 0x00000001, 0x00000009, 0x0004003b,
    0x0000000a, 0x0000000b, 0x00000001, 0x0004002b, 0x00000009, 0x0000000d, 0x00000002, 0x0004002b,
    0x00000009, 0x00000011, 0x00000001, 0x0004002b, 0x00000009, 0x00000013, 0x00000000, 0x00030016,
    0x00000015, 0x00000020, 0x00040017, 0x00000016, 0x00000015, 0x00000002, 0x00040020, 0x00000017,
    0x00000007, 0x00000016, 0x00040020, 0x0000001a, 0x00000007, 0x00000015, 0x00040017, 0x0000001e,
    0x00000015, 0x00000004, 0x00040020, 0x0000001f, 0x00000001, 0x0000001e, 0x0004003b, 0x0000001f,
    0x00000020, 0x00000001, 0x00040015, 0x00000021, 0x00000020, 0x00000000, 0x0004002b, 0x00000021,
    0x00000022, 0x00000002, 0x00040020, 0x00000023, 0x00000001, 0x00000015, 0x0004002b, 0x00000021,
    0x00000027, 0x00000000, 0x0004002b, 0x00000021, 0x0000002f, 0x00000003, 0x0004002b, 0x00000021,
    0x00000033, 0x00000001, 0x00040020, 0x00000038, 0x00000003, 0x00000016, 0x0004003b, 0x00000038,
    0x00000039, 0x00000003, 0x0004003b, 0x0000001f, 0x0000003e, 0x00000001, 0x0004003b, 0x00000038,
    0x00000050, 0x00000003, 0x0004003b, 0x0000001f, 0x00000055, 0x00000001, 0x0004003b, 0x00000038,
    0x00000067, 0x00000003, 0x00040020, 0x00000068, 0x00000001, 0x00000016, 0x0004003b, 0x00000068,
    0x00000069, 0x00000001, 0x00040020, 0x0000006b, 0x00000003, 0x0000001e, 0x0004003b, 0x0000006b,
    0x0000006c, 0x00000003, 0x0004003b, 0x0000001f, 0x0000006d, 0x00000001, 0x0004003b, 0x00000038,
    0x0000006f, 0x00000003, 0x0004003b, 0x00000068, 0x00000070, 0x00000001, 0x0004003b, 0x0000006b,
    0x00000072, 0x00000003, 0x0004003b, 0x0000001f, 0x00000074, 0x00000001, 0x0004003b, 0x0000001f,
    0x00000076, 0x00000001, 0x00040017, 0x00000078, 0x00000006, 0x00000004, 0x0004003b, 0x0000006b,
    0x0000007b, 0x00000003, 0x0004003b, 0x0000001f, 0x0000007c, 0x00000001, 0x0004001c, 0x0000007e,
    0x00000015, 0x00000033, 0x0006001e, 0x0000007f, 0x0000001e, 0x00000015, 0x0000007e, 0x0000007e,
    0x00040020, 0x00000080, 0x00000003, 0x0000007f, 0x0004003b, 0x00000080, 0x00000081, 0x00000003,
    0x0003001e, 0x00000084, 0x00000016, 0x00040020, 0x00000085, 0x00000009, 0x00000084, 0x0004003b,
    0x00000085, 0x00000086, 0x00000009, 0x00040020, 0x00000087, 0x00000009, 0x00000015, 0x0004002b,
    0x00000015, 0x0000008b, 0x40000000, 0x0004002b, 0x00000015, 0x0000008d, 0x3f800000, 0x0004002b,
    0x00000015, 0x00000096, 0x00000000, 0x00050036, 0x00000002, 0x00000004, 0x00000000, 0x00000003,
    0x000200f8, 0x00000005, 0x0004003b, 0x00000007, 0x00000008, 0x00000007, 0x0004003b, 0x00000007,
    0x0000000f, 0x00000007, 0x0004003b, 0x00000017, 0x00000018, 0x00000007, 0x0004003b, 0x0000001a,
    0x0000001b, 0x00000007, 0x0004003b, 0x0000001a, 0x0000002c, 0x00000007, 0x0004003b, 0x0000001a,
    0x0000003b, 0x00000007, 0x0004003b, 0x0000001a, 0x00000046, 0x00000007, 0x0004003b, 0x0000001a,
    0x00000052, 0x00000007, 0x0004003b, 0x0000001a, 0x0000005d, 0x00000007, 0x0004003d, 0x00000009,
    0x0000000c, 0x0000000b, 0x000500b1, 0x00000006, 0x0000000e, 0x0000000c, 0x0000000d, 0x0003003e,
    0x00000008, 0x0000000e, 0x0004003d, 0x00000009, 0x00000010, 0x0000000b, 0x000500c7, 0x00000009,
    0x00000012, 0x00000010, 0x00000011, 0x000500ab, 0x00000006, 0x00000014, 0x00000012, 0x00000013,
    0x0003003e, 0x0000000f, 0x00000014, 0x0004003d, 0x00000006, 0x00000019, 0x00000008, 0x000300f7,
    0x0000001d, 0x00000000, 0x000400fa, 0x00000019, 0x0000001c, 0x00000026, 0x000200f8, 0x0000001c,
    0x00050041, 0x00000023, 0x00000024, 0x00000020, 0x00000022, 0x0004003d, 0x00000015, 0x00000025,
    0x00000024, 0x0003003e, 0x0000001b, 0x00000025, 0x000200f9, 0x0000001d, 0x000200f8, 0x00000026,
    0x00050041, 0x00000023, 0x00000028, 0x00000020, 0x00000027, 0x0004003d, 0x00000015, 0x00000029,
    0x00000028, 0x0003003e, 0x0000001b, 0x00000029, 0x000200f9, 0x0000001d, 0x000200f8, 0x0000001d,
    0x0004003d, 0x00000015, 0x0000002a, 0x0000001b, 0x0004003d, 0x00000006, 0x0000002b, 0x0000000f,
    0x000300f7, 0x0000002e, 0x00000000, 0x000400fa, 0x0000002b, 0x0000002d, 0x00000032, 0x000200f8,
    0x0000002d, 0x00050041, 0x00000023, 0x00000030, 0x00000020, 0x0000002f, 0x0004003d, 0x00000015,
    0x00000031, 0x00000030, 0x0003003e, 0x0000002c, 0x00000031, 0x000200f9, 0x0000002e, 0x000200f8,
    0x00000032, 0x00050041, 0x00000023, 0x00000034, 0x00000020, 0x00000033, 0x0004003d, 0x00000015,
    0x00000035, 0x00000034, 0x0003003e, 0x0000002c, 0x00000035, 0x000200f9, 0x0000002e, 0x000200f8,
    0x0000002e, 0x0004003d, 0x00000015, 0x00000036, 0x0000002c, 0x00050050, 0x00000016, 0x00000037,
    0x0000002a, 0x00000036, 0x0003003e, 0x00000018, 0x00000037, 0x0004003d, 0x00000006, 0x0000003a,
    0x00000008, 0x000300f7, 0x0000003d, 0x00000000, 0x000400fa, 0x0000003a, 0x0000003c, 0x00000041,
    0x000200f8, 0x0000003c, 0x00050041, 0x00000023, 0x0000003f, 0x0000003e, 0x00000022, 0x0004003d,
    0x00000015, 0x00000040, 0x0000003f, 0x0003003e, 0x0000003b, 0x00000040, 0x000200f9, 0x0000003d,
    0x000200f8, 0x00000041, 0x00050041, 0x00000023, 0x00000042, 0x0000003e, 0x00000027, 0x0004003d,
    0x00000015, 0x00000043, 0x00000042, 0x0003003e, 0x0000003b, 0x00000043, 0x000200f9, 0x0000003d,
    0x000200f8, 0x0000003d, 0x0004003d, 0x00000015, 0x00000044, 0x0000003b, 0x0004003d, 0x00000006,
    0x00000045, 0x0000000f, 0x000300f7, 0x00000048, 0x00000000, 0x000400fa, 0x00000045, 0x00000047,
    0x0000004b, 0x000200f8, 0x00000047, 0x00050041, 0x00000023, 0x00000049, 0x0000003e, 0x0000002f,
    0x0004003d, 0x00000015, 0x0000004a, 0x00000049, 0x0003003e, 0x00000046, 0x0000004a, 0x000200f9,
    0x00000048, 0x000200f8, 0x0000004b, 0x00050041, 0x00000023, 0x0000004c, 0x0000003e, 0x00000033,
    0x0004003d, 0x00000015, 0x0000004d, 0x0000004c, 0x0003003e, 0x00000046, 0x0000004d, 0x000200f9,
    0x00000048, 0x000200f8, 0x00000048, 0x0004003d, 0x00000015, 0x0000004e, 0x00000046, 0x00050050,
    0x00000016, 0x0000004f, 0x00000044, 0x0000004e, 0x0003003e, 0x00000039, 0x0000004f, 0x0004003d,
    0x00000006, 0x00000051, 0x00000008, 0x000300f7, 0x00000054, 0x00000000, 0x000400fa, 0x00000051,
    0x00000053, 0x00000058, 0x000200f8, 0x00000053, 0x00050041, 0x00000023, 0x00000056, 0x00000055,
    0x00000022, 0x0004003d, 0x00000015, 0x00000057, 0x00000056, 0x0003003e, 0x00000052, 0x00000057,
    0x000200f9, 0x00000054, 0x000200f8, 0x00000058, 0x00050041, 0x00000023, 0x00000059, 0x00000055,
    0x00000027, 0x0004003d, 0x00000015, 0x0000005a, 0x00000059, 0x0003003e, 0x00000052, 0x0000005a,
    0x000200f9, 0x00000054, 0x000200f8, 0x00000054, 0x0004003d, 0x00000015, 0x0000005b, 0x00000052,
    0x0004003d, 0x00000006, 0x0000005c, 0x0000000f, 0x000300f7, 0x0000005f, 0x00000000, 0x000400fa,
    0x0000005c, 0x0000005e, 0x00000062, 0x000200f8, 0x0000005e, 0x00050041, 0x00000023, 0x00000060,
    0x00000055, 0x0000002f, 0x0004003d, 0x00000015, 0x00000061, 0x00000060, 0x0003003e, 0x0000005d,
    0x00000061, 0x000200f9, 0x0000005f, 0x000200f8, 0x00000062, 0x00050041, 0x00000023, 0x00000063,
    0x00000055, 0x00000033, 0x0004003d, 0x00000015, 0x00000064, 0x00000063, 0x0003003e, 0x0000005d,
    0x00000064, 0x000200f9, 0x0000005f, 0x000200f8, 0x0000005f, 0x0004003d, 0x00000015, 0x00000065,
    0x0000005d, 0x00050050, 0x00000016, 0x00000066, 0x0000005b, 0x00000065, 0x0003003e, 0x00000050,
    0x00000066, 0x0004003d, 0x00000016, 0x0000006a, 0x00000069, 0x0003003e, 0x00000067, 0x0000006a,
    0x0004003d, 0x0000001e, 0x0000006e, 0x0000006d, 0x0003003e, 0x0000006c, 0x0000006e, 0x0004003d,
    0x00000016, 0x00000071, 0x00000070, 0x0003003e, 0x0000006f, 0x00000071, 0x0004003d, 0x00000006,
    0x00000073, 0x0000000f, 0x0004003d, 0x0000001e, 0x00000075, 0x00000074, 0x0004003d, 0x0000001e,
    0x00000077, 0x00000076, 0x00070050, 0x00000078, 0x00000079, 0x00000073, 0x00000073, 0x00000073,
    0x00000073, 0x000600a9, 0x0000001e, 0x0000007a, 0x00000079, 0x00000075, 0x00000077, 0x0003003e,
    0x00000072, 0x0000007a, 0x0004003d, 0x0000001e, 0x0000007d, 0x0000007c, 0x0003003e, 0x0000007b,
    0x0000007d, 0x00050041, 0x0000001a, 0x00000082, 0x00000018, 0x00000027, 0x0004003d, 0x00000015,
    0x00000083, 0x00000082, 0x00060041, 0x00000087, 0x00000088, 0x00000086, 0x00000013, 0x00000027,
    0x0004003d, 0x00000015, 0x00000089, 0x00000088, 0x00050088, 0x00000015, 0x0000008a, 0x00000083,
    0x00000089, 0x00050085, 0x00000015, 0x0000008c, 0x0000008a, 0x0000008b, 0x00050083, 0x00000015,
    0x0000008e, 0x0000008c, 0x0000008d, 0x00050041, 0x0000001a, 0x0000008f, 0x00000018, 0x00000033,
    0x0004003d, 0x00000015, 0x00000090, 0x0000008f, 0x00060041, 0x00000087, 0x00000091, 0x00000086,
    0x00000013, 0x00000033, 0x0004003d, 0x00000015, 0x00000092, 0x00000091, 0x00050088, 0x00000015,
    0x00000093, 0x00000090, 0x00000092, 0x00050085, 0x00000015, 0x00000094, 0x00000093, 0x0000008b,
    0x00050083, 0x00000015, 0x00000095, 0x00000094, 0x0000008d, 0x00070050, 0x0000001e, 0x00000097,
    0x0000008e, 0x00000095, 0x00000096, 0x0000008d, 0x00050041, 0x0000006b, 0x00000098, 0x00000081,
    0x00000013, 0x0003003e, 0x00000098, 0x00000097, 0x000100fd, 0x00010038,
};

static VkShaderModule shader(cui_vk_renderer *r, const uint32_t *words, size_t size) {
    VkShaderModuleCreateInfo info = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    info.codeSize = size;
    info.pCode = words;
    VkShaderModule out = VK_NULL_HANDLE;
    return VK_OK(r->vk.vkCreateShaderModule(r->device, &info, NULL, &out)) ? out : VK_NULL_HANDLE;
}

static bool pipeline(cui_vk_renderer *r) {
    VkDescriptorSetLayoutBinding binding = { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, NULL };
    VkDescriptorSetLayoutCreateInfo set = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    set.bindingCount = 1;
    set.pBindings = &binding;
    VkPushConstantRange range = { VK_SHADER_STAGE_VERTEX_BIT, 0, 2 * sizeof(float) };
    VkPipelineLayoutCreateInfo layout = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    layout.setLayoutCount = layout.pushConstantRangeCount = 1;
    layout.pSetLayouts = &r->set_layout;
    layout.pPushConstantRanges = &range;
    VkSamplerCreateInfo sampler = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
    sampler.magFilter = sampler.minFilter = VK_FILTER_LINEAR;
    sampler.addressModeU = sampler.addressModeV = sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    if (!VK_OK(r->vk.vkCreateDescriptorSetLayout(r->device, &set, NULL, &r->set_layout)) ||
        !VK_OK(r->vk.vkCreatePipelineLayout(r->device, &layout, NULL, &r->layout)) ||
        !VK_OK(r->vk.vkCreateSampler(r->device, &sampler, NULL, &r->sampler))) return false;

    VkAttachmentDescription color = {0};
    color.format = r->format;
    color.samples = VK_SAMPLE_COUNT_1_BIT;
    color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    VkAttachmentReference ref = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    VkSubpassDescription subpass = {0};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &ref;
    VkSubpassDependency dependency = { VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 0 };
    VkRenderPassCreateInfo pass = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    pass.attachmentCount = pass.subpassCount = pass.dependencyCount = 1;
    pass.pAttachments = &color;
    pass.pSubpasses = &subpass;
    pass.pDependencies = &dependency;
    if (!VK_OK(r->vk.vkCreateRenderPass(r->device, &pass, NULL, &r->pass))) return false;
    /* A compatible pass keeps the same pipeline and permits screenshot readback. */
    color.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    if (!VK_OK(r->vk.vkCreateRenderPass(r->device, &pass, NULL, &r->capture_pass))) return false;

    VkShaderModule vs = shader(r, cui_vk_vert, sizeof cui_vk_vert);
    VkShaderModule fs = shader(r, cui_vk_frag, sizeof cui_vk_frag);
    bool ok = false;
    if (vs && fs) {
        VkPipelineShaderStageCreateInfo stages[] = {
            { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, NULL, 0, VK_SHADER_STAGE_VERTEX_BIT, vs, "main", NULL },
            { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, NULL, 0, VK_SHADER_STAGE_FRAGMENT_BIT, fs, "main", NULL }
        };
        VkVertexInputBindingDescription stream = { 0, sizeof(cui_vertex), VK_VERTEX_INPUT_RATE_VERTEX };
#define ATTR(loc, fmt, field) { loc, 0, VK_FORMAT_##fmt, offsetof(cui_vertex, field) }
        VkVertexInputAttributeDescription attributes[] = {
            ATTR(0, R32G32_SFLOAT, x), ATTR(1, R32G32_SFLOAT, u), ATTR(2, R32G32_SFLOAT, lx),
            ATTR(3, R32G32_SFLOAT, hw), ATTR(4, R32G32B32A32_SFLOAT, r_tl), ATTR(5, R32G32_SFLOAT, border),
            ATTR(6, R8G8B8A8_UNORM, color), ATTR(7, R8G8B8A8_UNORM, border_color)
        };
#undef ATTR
        VkPipelineVertexInputStateCreateInfo vertex = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
        vertex.vertexBindingDescriptionCount = 1;
        vertex.pVertexBindingDescriptions = &stream;
        vertex.vertexAttributeDescriptionCount = sizeof attributes / sizeof *attributes;
        vertex.pVertexAttributeDescriptions = attributes;
        VkPipelineInputAssemblyStateCreateInfo assembly = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
        assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkPipelineViewportStateCreateInfo viewport = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
        viewport.viewportCount = viewport.scissorCount = 1;
        VkPipelineRasterizationStateCreateInfo raster = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
        raster.lineWidth = 1.0f;
        VkPipelineMultisampleStateCreateInfo samples = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
        samples.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        VkPipelineColorBlendAttachmentState blend = {0};
        blend.blendEnable = VK_TRUE;
        blend.srcColorBlendFactor = blend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        blend.dstColorBlendFactor = blend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        VkPipelineColorBlendStateCreateInfo blending = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
        blending.attachmentCount = 1;
        blending.pAttachments = &blend;
        VkDynamicState states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dynamic = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
        dynamic.dynamicStateCount = 2;
        dynamic.pDynamicStates = states;
        VkGraphicsPipelineCreateInfo info = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
        info.stageCount = 2;
        info.pStages = stages;
        info.pVertexInputState = &vertex;
        info.pInputAssemblyState = &assembly;
        info.pViewportState = &viewport;
        info.pRasterizationState = &raster;
        info.pMultisampleState = &samples;
        info.pColorBlendState = &blending;
        info.pDynamicState = &dynamic;
        info.layout = r->layout;
        info.renderPass = r->pass;
        ok = VK_OK(r->vk.vkCreateGraphicsPipelines(r->device, VK_NULL_HANDLE, 1, &info, NULL, &r->pipeline));
        if (ok) {
            VkShaderModule quad = shader(r, cui_vk_quad_vert, sizeof cui_vk_quad_vert);
            if (quad) {
#define ATTR(loc, fmt, field) { loc, 0, VK_FORMAT_##fmt, offsetof(vk_quad, field) }
                VkVertexInputAttributeDescription quad_attributes[] = {
                    ATTR(0, R32G32B32A32_SFLOAT, rect), ATTR(1, R32G32B32A32_SFLOAT, uv),
                    ATTR(2, R32G32B32A32_SFLOAT, local), ATTR(3, R32G32_SFLOAT, half),
                    ATTR(4, R32G32B32A32_SFLOAT, radii), ATTR(5, R32G32_SFLOAT, params),
                    ATTR(6, R8G8B8A8_UNORM, top), ATTR(7, R8G8B8A8_UNORM, bottom), ATTR(8, R8G8B8A8_UNORM, border)
                };
#undef ATTR
                stages[0].module = quad;
                stream.stride = sizeof(vk_quad);
                stream.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
                vertex.vertexAttributeDescriptionCount = sizeof quad_attributes / sizeof *quad_attributes;
                vertex.pVertexAttributeDescriptions = quad_attributes;
                assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
                r->vk.vkCreateGraphicsPipelines(r->device, VK_NULL_HANDLE, 1, &info, NULL, &r->quad_pipeline);
                r->vk.vkDestroyShaderModule(r->device, quad, NULL);
            }
        }
    }
    r->vk.vkDestroyShaderModule(r->device, vs, NULL);
    r->vk.vkDestroyShaderModule(r->device, fs, NULL);
    return ok;
}

static bool framebuffer(cui_vk_renderer *r, VkRenderPass pass, VkImageView view, VkExtent2D size, VkFramebuffer *out) {
    VkFramebufferCreateInfo fb = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
    fb.renderPass = pass;
    fb.attachmentCount = 1;
    fb.pAttachments = &view;
    fb.width = size.width;
    fb.height = size.height;
    fb.layers = 1;
    return VK_OK(r->vk.vkCreateFramebuffer(r->device, &fb, NULL, out));
}

static void free_swap(cui_vk_renderer *r) {
    for (uint32_t i = 0; i < r->back_count; i++) {
        vk_backbuffer *b = &r->backs[i];
        r->vk.vkDestroySemaphore(r->device, b->rendered, NULL);
        r->vk.vkDestroyFramebuffer(r->device, b->fb, NULL);
        r->vk.vkDestroyImageView(r->device, b->view, NULL);
    }
    free(r->backs);
    r->backs = NULL;
    r->back_count = 0;
    r->vk.vkDestroySwapchainKHR(r->device, r->swap, NULL);
    r->swap = VK_NULL_HANDLE;
}

static VkResult resize_swap(cui_vk_renderer *r, int w, int h) {
    VkSurfaceCapabilitiesKHR caps;
    VkResult result = r->vk.vkGetPhysicalDeviceSurfaceCapabilitiesKHR(r->physical, r->surface, &caps);
    if (result != VK_SUCCESS) return result;
    VkExtent2D extent = caps.currentExtent;
    if (extent.width == UINT32_MAX) extent = (VkExtent2D){ w, h };
    if (!extent.width || !extent.height) return VK_NOT_READY;
    if (!(caps.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)) return VK_ERROR_FORMAT_NOT_SUPPORTED;
    VkSwapchainCreateInfoKHR info = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    info.surface = r->surface;
    info.minImageCount = caps.minImageCount + 1;
    if (caps.maxImageCount && info.minImageCount > caps.maxImageCount) info.minImageCount = caps.maxImageCount;
    info.imageFormat = r->format;
    info.imageColorSpace = r->colorspace;
    info.imageExtent = extent;
    info.imageArrayLayers = 1;
    info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    info.preTransform = caps.currentTransform;
    info.compositeAlpha = (VkCompositeAlphaFlagBitsKHR)(caps.supportedCompositeAlpha & (0u - caps.supportedCompositeAlpha));
    info.presentMode = r->mode;
    info.clipped = VK_TRUE;
    info.oldSwapchain = r->swap;
    if (!wait_vk(r)) return VK_ERROR_DEVICE_LOST;
    VkSwapchainKHR swap = VK_NULL_HANDLE;
    result = r->vk.vkCreateSwapchainKHR(r->device, &info, NULL, &swap);
    free_swap(r);
    if (result != VK_SUCCESS) return result;
    r->swap = swap;
    uint32_t count = 0;
    if (!VK_OK(r->vk.vkGetSwapchainImagesKHR(r->device, swap, &count, NULL)) || !count)
        return VK_ERROR_INITIALIZATION_FAILED;
    VkImage *images = calloc(count, sizeof *images);
    r->backs = calloc(count, sizeof *r->backs);
    bool ok = images && r->backs && VK_OK(r->vk.vkGetSwapchainImagesKHR(r->device, swap, &count, images));
    if (ok) {
        r->back_count = count;
        VkSemaphoreCreateInfo sem = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        for (uint32_t i = 0; i < count && ok; i++) {
            vk_backbuffer *b = &r->backs[i];
            ok = image_view(r, images[i], r->format, &b->view) && framebuffer(r, r->pass, b->view, extent, &b->fb) &&
                 VK_OK(r->vk.vkCreateSemaphore(r->device, &sem, NULL, &b->rendered));
        }
    }
    free(images);
    if (!ok) return VK_ERROR_INITIALIZATION_FAILED;
    r->extent = extent;
    r->width = w;
    r->height = h;
    r->recreate = false;
    return VK_SUCCESS;
}

static bool quad_batch(const cui_draw_data *dd, const cui_draw_cmd *cmd) {
    if (!cmd->index_count || cmd->index_count % 6 != 0) return false;
    const uint32_t *ix = dd->indices + cmd->index_offset;
    uint32_t first = ix[0], count = cmd->index_count / 6;
    if (first >= (uint32_t)dd->vertex_count || count > ((uint32_t)dd->vertex_count - first) / 4) return false;
    const cui_vertex *v = dd->vertices + first;
    for (uint32_t q = 0; q < count; q++, v += 4, first += 4, ix += 6) {
        if (ix[0] != first || ix[1] != first + 1 || ix[2] != first + 2 ||
            ix[3] != first || ix[4] != first + 2 || ix[5] != first + 3) return false;
        /* The generic draw list emits modes TEXT through IMAGE only as axis-aligned
         * quads with a uniform color per row. Dither also supports arbitrary meshes. */
        if (v->mode < CUI_SHADE_TEXT || v->mode > CUI_SHADE_IMAGE) {
            if (v->mode != CUI_SHADE_DITHER ||
                v[1].x != v[2].x || v[3].x != v[0].x || v[1].y != v[0].y || v[3].y != v[2].y ||
                v[1].u != v[2].u || v[3].u != v[0].u || v[1].v != v[0].v || v[3].v != v[2].v ||
                v[1].lx != v[2].lx || v[3].lx != v[0].lx || v[1].ly != v[0].ly || v[3].ly != v[2].ly ||
                memcmp(&v[0].color, &v[1].color, sizeof(cui_color)) ||
                memcmp(&v[2].color, &v[3].color, sizeof(cui_color))) return false;
        }
    }
    return true;
}

static void pack_quads(vk_quad *dst, const cui_vertex *v, int count) {
    for (int i = 0; i < count; i++, v += 4) {
        vk_quad quad = {
            { v[0].x, v[0].y, v[2].x, v[2].y },
            { v[0].u, v[0].v, v[2].u, v[2].v },
            { v[0].lx, v[0].ly, v[2].lx, v[2].ly },
            { v[0].hw, v[0].hh },
            { v[0].r_tl, v[0].r_tr, v[0].r_br, v[0].r_bl },
            { v[0].border, v[0].mode },
            v[0].color, v[2].color, v[0].border_color
        };
        memcpy(&dst[i], &quad, sizeof quad);
    }
}

static bool geometry(cui_vk_renderer *r, vk_frame *f, const cui_draw_data *dd) {
    r->compact_geometry = false;
    r->geometry_bytes = 0;
    if (!dd->index_count) return true;
    uint32_t quads = 0;
    if (r->instancing && r->quad_pipeline && dd->index_count / 6 >= CUI_VK_MIN_INSTANCES) {
        size_t count = (size_t)dd->command_count;
        if (count > r->draw_capacity) {
            vk_draw *draws = realloc(r->draws, count * sizeof(*draws));
            if (!draws) return false;
            r->draws = draws;
            r->draw_capacity = count;
        }
        for (int i = 0; i < dd->command_count; i++) {
            const cui_draw_cmd *cmd = &dd->commands[i];
            uint32_t n = quad_batch(dd, cmd) ? cmd->index_count / 6 : 0;
            r->draws[i] = (vk_draw){ .instances = n };
            quads += n;
        }
    }

    size_t vertices = (size_t)dd->vertex_count, indices = (size_t)dd->index_count;
    if (quads < CUI_VK_MIN_INSTANCES) quads = 0;
    if (quads) {
        /* Keep each mixed batch indexed. Bound its vertex range from the public
         * indices so compaction does not depend on draw-list allocator metadata. */
        vertices = indices = 0;
        for (int i = 0; i < dd->command_count; i++) {
            const cui_draw_cmd *cmd = &dd->commands[i];
            vk_draw *batch = &r->draws[i];
            if (batch->instances || !cmd->index_count) continue;
            const uint32_t *ix = dd->indices + cmd->index_offset;
            uint32_t lo = ix[0], hi = ix[0];
            for (uint32_t j = 1; j < cmd->index_count; j++) {
                lo = ix[j] < lo ? ix[j] : lo;
                hi = ix[j] > hi ? ix[j] : hi;
            }
            batch->first = lo;
            batch->mesh_vertices = hi - lo + 1;
            vertices += batch->mesh_vertices;
            indices += cmd->index_count;
        }
    }

    size_t vs = vertices * sizeof(cui_vertex), is = indices * sizeof(uint32_t);
    size_t qs = (size_t)quads * sizeof(vk_quad);
    size_t stream_size = vs + qs;
    r->geometry_bytes = stream_size + is;
    if (stream_size > f->vertices.size &&
        !buffer(r, &f->vertices, stream_size + stream_size / 2, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)) return false;
    if (is > f->indices.size &&
        !buffer(r, &f->indices, is + is / 2, VK_BUFFER_USAGE_INDEX_BUFFER_BIT)) return false;
    if (!quads) {
        memcpy(f->vertices.map, dd->vertices, vs);
        memcpy(f->indices.map, dd->indices, is);
        return true;
    }

    r->quad_offset = vs;
    vk_quad *instances = (vk_quad *)((uint8_t *)f->vertices.map + vs);
    uint32_t q = 0, v = 0, offset = 0;
    for (int i = 0; i < dd->command_count; i++) {
        const cui_draw_cmd *cmd = &dd->commands[i];
        vk_draw *batch = &r->draws[i];
        if (!cmd->index_count) continue;
        const uint32_t *ix = dd->indices + cmd->index_offset;
        if (batch->instances) {
            pack_quads(instances + q, dd->vertices + ix[0], (int)batch->instances);
            batch->first = q;
            q += batch->instances;
        } else {
            uint32_t first = batch->first, n = batch->mesh_vertices;
            memcpy((cui_vertex *)f->vertices.map + v, dd->vertices + first, (size_t)n * sizeof(cui_vertex));
            memcpy((uint32_t *)f->indices.map + offset, ix, (size_t)cmd->index_count * sizeof(*ix));
            batch->first = offset;
            batch->vertex_offset = (int32_t)v - (int32_t)first;
            v += n;
            offset += cmd->index_count;
        }
    }
    r->compact_geometry = true;
    return true;
}

static void draw(cui_vk_renderer *r, VkCommandBuffer cmd, vk_frame *f, VkRenderPass pass, VkFramebuffer fb,
                 VkExtent2D size, const cui_draw_data *dd) {
    VkClearValue clear = {0};
    clear.color.float32[0] = dd->clear_color.r / 255.0f;
    clear.color.float32[1] = dd->clear_color.g / 255.0f;
    clear.color.float32[2] = dd->clear_color.b / 255.0f;
    clear.color.float32[3] = 1.0f;
    VkRenderPassBeginInfo info = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    info.renderPass = pass;
    info.framebuffer = fb;
    info.renderArea.extent = size;
    info.clearValueCount = 1;
    info.pClearValues = &clear;
    r->vk.vkCmdBeginRenderPass(cmd, &info, VK_SUBPASS_CONTENTS_INLINE);
    if (dd->index_count) {
        float resolution[] = { (float)dd->width_px, (float)dd->height_px };
        VkViewport viewport = { 0, 0, (float)size.width, (float)size.height, 0, 1 };
        r->vk.vkCmdSetViewport(cmd, 0, 1, &viewport);
        r->vk.vkCmdPushConstants(cmd, r->layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof resolution, resolution);
        VkDescriptorSet bound = VK_NULL_HANDLE;
        VkPipeline bound_pipeline = VK_NULL_HANDLE;
        bool index_bound = false;
        VkRect2D scissor = {0};
        for (int i = 0; i < dd->command_count; i++) {
            const cui_draw_cmd *c = &dd->commands[i];
            if (!c->index_count) continue;
            int x0 = (int)fmaxf(0, floorf(c->clip.x)), y0 = (int)fmaxf(0, floorf(c->clip.y));
            int x1 = (int)fminf((float)size.width, ceilf(c->clip.x + c->clip.w));
            int y1 = (int)fminf((float)size.height, ceilf(c->clip.y + c->clip.h));
            if (x1 <= x0 || y1 <= y0) continue;
            vk_draw range = r->compact_geometry ? r->draws[i] : (vk_draw){ .first = c->index_offset };
            VkPipeline pipeline = range.instances ? r->quad_pipeline : r->pipeline;
            if (pipeline != bound_pipeline) {
                VkDeviceSize offset = range.instances ? r->quad_offset : 0;
                r->vk.vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
                r->vk.vkCmdBindVertexBuffers(cmd, 0, 1, &f->vertices.handle, &offset);
                if (!range.instances && !index_bound) {
                    r->vk.vkCmdBindIndexBuffer(cmd, f->indices.handle, 0, VK_INDEX_TYPE_UINT32);
                    index_bound = true;
                }
                bound_pipeline = pipeline;
            }
            VkRect2D clip = { { x0, y0 }, { x1 - x0, y1 - y0 } };
            vk_texture *t = texture(r, c->texture ? c->texture : dd->font_atlas);
            if (!t) t = texture(r, r->white);
            if (bound != t->set) {
                bound = t->set;
                r->vk.vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r->layout, 0, 1, &bound, 0, NULL);
            }
            if (clip.offset.x != scissor.offset.x || clip.offset.y != scissor.offset.y ||
                clip.extent.width != scissor.extent.width || clip.extent.height != scissor.extent.height) {
                r->vk.vkCmdSetScissor(cmd, 0, 1, &clip);
                scissor = clip;
            }
            if (range.instances) r->vk.vkCmdDraw(cmd, 4, range.instances, 0, range.first);
            else r->vk.vkCmdDrawIndexed(cmd, c->index_count, 1, range.first, range.vertex_offset, 0);
        }
    }
    r->vk.vkCmdEndRenderPass(cmd);
}

int cui_vulkan_render(cui_vk_renderer *r, const cui_draw_data *dd) {
    if (r->hidden) return 1;
    if (r->failed) return -1;
    VkResult result = VK_SUCCESS;
    if (r->recreate || dd->width_px != r->width || dd->height_px != r->height) {
        result = resize_swap(r, dd->width_px, dd->height_px);
        if (result == VK_NOT_READY || result == VK_ERROR_OUT_OF_DATE_KHR) return 0;
        if (result != VK_SUCCESS) goto fail;
    }
    vk_frame *f = &r->frames[r->next];
    result = r->vk.vkWaitForFences(r->device, 1, &f->done, VK_TRUE, UINT64_MAX);
    if (result != VK_SUCCESS) goto fail;
    if (!geometry(r, f, dd) || !stage_uploads(r, f)) { result = VK_ERROR_OUT_OF_DEVICE_MEMORY; goto fail; }
    uint32_t index;
    result = r->vk.vkAcquireNextImageKHR(r->device, r->swap, 100000000, f->acquired, VK_NULL_HANDLE, &index);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) r->recreate = true;
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_TIMEOUT) return 0;
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) goto fail;
    vk_backbuffer *back = &r->backs[index];
    if (!begin(r, f->cmd)) { result = VK_ERROR_INITIALIZATION_FAILED; goto fail; }
    record_uploads(r, f, f->cmd);
    draw(r, f->cmd, f, r->pass, back->fb, r->extent, dd);
    VkPipelineStageFlags wait = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submit.waitSemaphoreCount = submit.commandBufferCount = submit.signalSemaphoreCount = 1;
    submit.pWaitSemaphores = &f->acquired;
    submit.pWaitDstStageMask = &wait;
    submit.pCommandBuffers = &f->cmd;
    submit.pSignalSemaphores = &back->rendered;
    result = r->vk.vkEndCommandBuffer(f->cmd);
    if (result != VK_SUCCESS) goto fail;
    result = r->vk.vkResetFences(r->device, 1, &f->done);
    if (result != VK_SUCCESS) goto fail;
    result = r->vk.vkQueueSubmit(r->queue, 1, &submit, f->done);
    if (result != VK_SUCCESS) goto fail;
    r->upload_count = r->upload_size = 0;
    r->next = (r->next + 1) % CUI_VK_FRAMES;
    VkPresentInfoKHR present = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    present.waitSemaphoreCount = present.swapchainCount = 1;
    present.pWaitSemaphores = &back->rendered;
    present.pSwapchains = &r->swap;
    present.pImageIndices = &index;
    result = r->vk.vkQueuePresentKHR(r->queue, &present);
    if (result == VK_SUCCESS) return 1;
    if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR) {
        r->recreate = true;
        return result == VK_SUBOPTIMAL_KHR ? 1 : 0;
    }
fail:
    r->failed = true;
    fprintf(stderr, "cui: Vulkan frame failed (VkResult %d)\n", result);
    return -1;
}

bool cui_vulkan_readback(cui_vk_renderer *r, const cui_draw_data *dd, void *rgba, int width) {
    if (r->failed || dd->width_px <= 0 || dd->height_px <= 0 || !wait_vk(r)) return false;
    VkExtent2D size = { dd->width_px, dd->height_px };
    vk_image image = {0};
    vk_buffer pixels = {0};
    VkFramebuffer fb = VK_NULL_HANDLE;
    vk_frame *f = &r->frames[r->next];
    size_t row = (size_t)size.width * 4;
    bool ok = make_image(r, &image, size.width, size.height, r->format,
                         VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT) &&
              framebuffer(r, r->capture_pass, image.view, size, &fb) && geometry(r, f, dd) &&
              buffer(r, &pixels, row * size.height, VK_BUFFER_USAGE_TRANSFER_DST_BIT) &&
              stage_uploads(r, f) && begin(r, r->immediate);
    if (ok) {
        record_uploads(r, f, r->immediate);
        draw(r, r->immediate, f, r->capture_pass, fb, size, dd);
        barrier(r, r->immediate, image.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT);
        VkBufferImageCopy copy = {0};
        copy.imageSubresource = (VkImageSubresourceLayers){ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        copy.imageExtent = (VkExtent3D){ size.width, size.height, 1 };
        r->vk.vkCmdCopyImageToBuffer(r->immediate, image.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, pixels.handle, 1, &copy);
        VkMemoryBarrier readable = { VK_STRUCTURE_TYPE_MEMORY_BARRIER };
        readable.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        readable.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
        r->vk.vkCmdPipelineBarrier(r->immediate, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT,
                                   0, 1, &readable, 0, NULL, 0, NULL);
        ok = finish_immediate(r);
        if (ok) r->upload_count = r->upload_size = 0;
        if (ok) for (uint32_t y = 0; y < size.height; y++) {
            uint8_t *dst = (uint8_t *)rgba + (size_t)y * width * 4;
            memcpy(dst, (uint8_t *)pixels.map + y * row, row);
            if (r->format == VK_FORMAT_B8G8R8A8_UNORM) for (size_t x = 0; x < row; x += 4) {
                uint8_t red = dst[x + 2]; dst[x + 2] = dst[x]; dst[x] = red;
            }
        }
    }
    r->vk.vkDestroyFramebuffer(r->device, fb, NULL);
    free_image(r, &image);
    free_buffer(r, &pixels);
    return ok;
}
static bool surface_format(cui_vk_renderer *r, VkPhysicalDevice gpu) {
    uint32_t count = 0;
    if (!VK_OK(r->vk.vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, r->surface, &count, NULL)) || !count) return false;
    VkSurfaceFormatKHR *formats = calloc(count, sizeof *formats);
    if (!formats) return false;
    bool ok = VK_OK(r->vk.vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, r->surface, &count, formats));
    VkFormat chosen = VK_FORMAT_UNDEFINED;
    if (ok) for (uint32_t i = 0; i < count; i++) {
        if (formats[i].colorSpace != VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) continue;
        if (formats[i].format == VK_FORMAT_B8G8R8A8_UNORM || formats[i].format == VK_FORMAT_UNDEFINED) {
            chosen = VK_FORMAT_B8G8R8A8_UNORM;
            break;
        }
        if (formats[i].format == VK_FORMAT_R8G8B8A8_UNORM) chosen = formats[i].format;
    }
    free(formats);
    r->format = chosen;
    r->colorspace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    return chosen != VK_FORMAT_UNDEFINED;
}

static bool device(cui_vk_renderer *r) {
    uint32_t count = 0;
    if (!VK_OK(r->vk.vkEnumeratePhysicalDevices(r->instance, &count, NULL)) || !count) return false;
    VkPhysicalDevice *gpus = calloc(count, sizeof *gpus);
    if (!gpus) return false;
    bool ok = VK_OK(r->vk.vkEnumeratePhysicalDevices(r->instance, &count, gpus));
    for (uint32_t i = 0; ok && i < count && !r->device; i++) {
        VkPhysicalDevice gpu = gpus[i];
        VkPhysicalDeviceProperties props;
        r->vk.vkGetPhysicalDeviceProperties(gpu, &props);
        if (props.apiVersion < VK_API_VERSION_1_1 || !surface_format(r, gpu)) continue;
        uint32_t queues = 0;
        r->vk.vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queues, NULL);
        VkQueueFamilyProperties *families = calloc(queues, sizeof *families);
        if (!families) break;
        r->vk.vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queues, families);
        for (uint32_t j = 0; j < queues && !r->device; j++) {
            VkBool32 present = VK_FALSE;
            if (!(families[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) || !families[j].queueCount ||
                !VK_OK(r->vk.vkGetPhysicalDeviceSurfaceSupportKHR(gpu, j, r->surface, &present)) || !present) continue;
            float priority = 1.0f;
            VkDeviceQueueCreateInfo queue = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
            queue.queueFamilyIndex = j;
            queue.queueCount = 1;
            queue.pQueuePriorities = &priority;
            const char *extension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
            VkDeviceCreateInfo info = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
            info.queueCreateInfoCount = info.enabledExtensionCount = 1;
            info.pQueueCreateInfos = &queue;
            info.ppEnabledExtensionNames = &extension;
            VkDevice logical = VK_NULL_HANDLE;
            if (VK_OK(r->vk.vkCreateDevice(gpu, &info, NULL, &logical))) {
                r->device = logical;
                r->physical = gpu;
                r->family = j;
            }
        }
        free(families);
    }
    free(gpus);
    return r->device != VK_NULL_HANDLE;
}

static VkPresentModeKHR present_mode(cui_vk_renderer *r, bool vsync) {
    if (vsync) return VK_PRESENT_MODE_FIFO_KHR;
    uint32_t count = 0;
    if (!VK_OK(r->vk.vkGetPhysicalDeviceSurfacePresentModesKHR(r->physical, r->surface, &count, NULL)) || !count)
        return VK_PRESENT_MODE_FIFO_KHR;
    VkPresentModeKHR *modes = calloc(count, sizeof *modes);
    if (!modes) return VK_PRESENT_MODE_FIFO_KHR;
    VkPresentModeKHR selected = VK_PRESENT_MODE_FIFO_KHR;
    if (VK_OK(r->vk.vkGetPhysicalDeviceSurfacePresentModesKHR(r->physical, r->surface, &count, modes))) {
        for (uint32_t i = 0; i < count; i++) {
            if (modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) { selected = modes[i]; break; }
            if (modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) selected = modes[i];
        }
    }
    free(modes);
    return selected;
}

cui_vk_renderer *cui_vulkan_create(cui_app *app) {
    float wpt, hpt, scale;
    int width, height;
    cui__plat_metrics(app, &wpt, &hpt, &scale, &width, &height);
    cui_vk_renderer *r = calloc(1, sizeof *r);
    if (!r) return NULL;
    r->loader = LoadLibraryExW(L"vulkan-1.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!r->loader) goto fail;
    r->vk.vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)(void *)GetProcAddress(r->loader, "vkGetInstanceProcAddr");
    if (!r->vk.vkGetInstanceProcAddr) goto fail;
#define LOAD_GLOBAL(name) r->vk.name = (PFN_##name)r->vk.vkGetInstanceProcAddr(NULL, #name); if (!r->vk.name) goto fail;
    CUI_VK_GLOBAL(LOAD_GLOBAL)
#undef LOAD_GLOBAL
    VkApplicationInfo app_info = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
    app_info.pEngineName = "CUI";
    app_info.apiVersion = VK_API_VERSION_1_1;
    const char *extensions[] = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME };
    VkInstanceCreateInfo instance = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    instance.pApplicationInfo = &app_info;
    instance.enabledExtensionCount = 2;
    instance.ppEnabledExtensionNames = extensions;
    if (!VK_OK(r->vk.vkCreateInstance(&instance, NULL, &r->instance))) goto fail;
#define LOAD_INSTANCE(name) r->vk.name = (PFN_##name)r->vk.vkGetInstanceProcAddr(r->instance, #name); if (!r->vk.name) goto fail;
    CUI_VK_INSTANCE(LOAD_INSTANCE)
#undef LOAD_INSTANCE
    VkWin32SurfaceCreateInfoKHR surface = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
    surface.hwnd = (HWND)cui__plat_native(app);
    surface.hinstance = (HINSTANCE)GetWindowLongPtrW(surface.hwnd, GWLP_HINSTANCE);
    if (!VK_OK(r->vk.vkCreateWin32SurfaceKHR(r->instance, &surface, NULL, &r->surface)) || !device(r)) goto fail;
#define LOAD_DEVICE(name) r->vk.name = (PFN_##name)r->vk.vkGetDeviceProcAddr(r->device, #name); if (!r->vk.name) goto fail;
    CUI_VK_DEVICE(LOAD_DEVICE)
#undef LOAD_DEVICE
    r->vk.vkGetDeviceQueue(r->device, r->family, 0, &r->queue);
    r->vk.vkGetPhysicalDeviceMemoryProperties(r->physical, &r->memory);
    r->mode = present_mode(r, !app->desc.no_vsync);
    r->hidden = app->desc.hidden;
    r->instancing = true;
    VkCommandPoolCreateInfo pool = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    pool.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool.queueFamilyIndex = r->family;
    if (!VK_OK(r->vk.vkCreateCommandPool(r->device, &pool, NULL, &r->commands))) goto fail;
    VkCommandBufferAllocateInfo alloc = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    alloc.commandPool = r->commands;
    alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc.commandBufferCount = CUI_VK_FRAMES + 1;
    VkCommandBuffer commands[CUI_VK_FRAMES + 1];
    if (!VK_OK(r->vk.vkAllocateCommandBuffers(r->device, &alloc, commands))) goto fail;
    r->immediate = commands[CUI_VK_FRAMES];
    VkFenceCreateInfo fence = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    fence.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    VkSemaphoreCreateInfo sem = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    for (uint32_t i = 0; i < CUI_VK_FRAMES; i++) {
        r->frames[i].cmd = commands[i];
        if (!VK_OK(r->vk.vkCreateFence(r->device, &fence, NULL, &r->frames[i].done)) ||
            !VK_OK(r->vk.vkCreateSemaphore(r->device, &sem, NULL, &r->frames[i].acquired))) goto fail;
    }
    if (!pipeline(r)) goto fail;
    uint8_t white = 255;
    r->white = tex_create(r, 1, 1, CUI_TEXTURE_R8, &white);
    if (!r->white) goto fail;
    VkResult swap = resize_swap(r, width, height);
    if (swap != VK_SUCCESS && swap != VK_NOT_READY) goto fail;
    r->recreate = swap != VK_SUCCESS;
    return r;
fail:
    cui_vulkan_destroy(r);
    return NULL;
}

void cui_vulkan_destroy(cui_vk_renderer *r) {
    if (!r) return;
    if (r->queue) {
        wait_vk(r);
        free_swap(r);
        for (size_t i = 0; i < r->pool_count; i++) r->vk.vkDestroyDescriptorPool(r->device, r->pools[i], NULL);
        for (size_t i = 0; i < r->texture_count; i++) free_image(r, &r->textures[i].image);
        for (uint32_t i = 0; i < CUI_VK_FRAMES; i++) {
            vk_frame *f = &r->frames[i];
            free_buffer(r, &f->vertices);
            free_buffer(r, &f->indices);
            free_buffer(r, &f->staging);
            r->vk.vkDestroySemaphore(r->device, f->acquired, NULL);
            r->vk.vkDestroyFence(r->device, f->done, NULL);
        }
        r->vk.vkDestroyCommandPool(r->device, r->commands, NULL);
        r->vk.vkDestroyPipeline(r->device, r->pipeline, NULL);
        r->vk.vkDestroyPipeline(r->device, r->quad_pipeline, NULL);
        r->vk.vkDestroyRenderPass(r->device, r->pass, NULL);
        r->vk.vkDestroyRenderPass(r->device, r->capture_pass, NULL);
        r->vk.vkDestroyPipelineLayout(r->device, r->layout, NULL);
        r->vk.vkDestroyDescriptorSetLayout(r->device, r->set_layout, NULL);
        r->vk.vkDestroySampler(r->device, r->sampler, NULL);
    }
    if (r->device && r->vk.vkDestroyDevice) r->vk.vkDestroyDevice(r->device, NULL);
    if (r->surface) r->vk.vkDestroySurfaceKHR(r->instance, r->surface, NULL);
    if (r->instance && r->vk.vkDestroyInstance) r->vk.vkDestroyInstance(r->instance, NULL);
    if (r->loader) FreeLibrary(r->loader);
    free(r->textures);
    free(r->pools);
    free(r->uploads);
    free(r->upload_pixels);
    free(r->draws);
    free(r);
}

/* Private benchmark controls. Production callers use the common app interface. */
#ifdef CUI_VULKAN_BENCH
bool cui__vk_bench_configure(cui_app *app, bool instancing) {
    if (!app || !app->vulkan) return false;
    app->vulkan->instancing = instancing;
    return true;
}

bool cui__vk_bench_wait(cui_app *app) {
    return app && app->vulkan && wait_vk(app->vulkan);
}

size_t cui__vk_bench_geometry_bytes(const cui_app *app) {
    return app->vulkan->geometry_bytes;
}
#endif
