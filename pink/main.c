#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

static void *map_fb(int fd, drmModeFB *fb, uint32_t handle, size_t *map_size, uint32_t *pitch) {
    struct drm_mode_map_dumb mreq = {0};
    mreq.handle = handle;
    if (drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &mreq) < 0) {
        perror("DRM_IOCTL_MODE_MAP_DUMB");
        exit(1);
    }
    *map_size = fb->height * fb->pitch;
    *pitch = fb->pitch;
    void *map = mmap(0, *map_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, mreq.offset);
    if (map == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    return map;
}

int main() {
    int fd = open("/dev/dri/card1", O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    drmModeRes *res = drmModeGetResources(fd);
    if (!res) {
        perror("drmModeGetResources");
        return 1;
    }

    drmModeConnector *conn = NULL;
    drmModeEncoder *enc = NULL;
    uint32_t conn_id = 0, crtc_id = 0;
    for (int i = 0; i < res->count_connectors; i++) {
        conn = drmModeGetConnector(fd, res->connectors[i]);
        if (conn && conn->connection == DRM_MODE_CONNECTED && conn->count_modes > 0) {
            conn_id = conn->connector_id;
            enc = drmModeGetEncoder(fd, conn->encoder_id);
            crtc_id = enc->crtc_id;
            break;
        }
        drmModeFreeConnector(conn);
        conn = NULL;
    }
    if (!conn) {
        fprintf(stderr, "No active connector found\n");
        return 1;
    }

    drmModeCrtc *crtc = drmModeGetCrtc(fd, crtc_id);
    if (!crtc) {
        perror("drmModeGetCrtc");
        return 1;
    }

    drmModeFB *fb = drmModeGetFB(fd, crtc->buffer_id);
    if (!fb) {
        perror("drmModeGetFB");
        return 1;
    }

    size_t map_size;
    uint32_t pitch;
    uint8_t *map = map_fb(fd, fb, fb->handle, &map_size, &pitch);

    uint32_t pink = 0xFFFF00FF; // ARGB or XRGB depending on fb->depth
    for (uint32_t y = 0; y < fb->height; y++) {
        uint32_t *row = (uint32_t *)(map + y * pitch);
        for (uint32_t x = 0; x < fb->width; x++) {
            row[x] = pink;
        }
    }

    munmap(map, map_size);
    drmModeFreeFB(fb);
    drmModeFreeCrtc(crtc);
    drmModeFreeEncoder(enc);
    drmModeFreeConnector(conn);
    drmModeFreeResources(res);
    close(fd);
    return 0;
}

