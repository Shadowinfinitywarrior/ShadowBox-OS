#include "sys.h"

/*
 * Screenshot tool for ShadowBox OS.
 * Captures the current framebuffer and writes it as a 24‑bit BMP file
 * named "screenshot.bmp" in the current working directory.
 *
 * Steps:
 *   1. Query framebuffer info (width, height, pitch, bits‑per‑pixel).
 *   2. mmap the framebuffer address.
 *   3. Open the BMP file via sb_acquire with O_WRONLY|O_CREAT|O_TRUNC.
 *   4. Write BMP file header (BITMAPFILEHEADER + BITMAPINFOHEADER).
 *   5. Convert each pixel to BGR format (ignore alpha) and write rows top‑down
 *      using a negative height in the header (no row inversion needed).
 *   6. Close the file and terminate.
 */

static void write_le32(unsigned char *dst, uint32_t val) {
    dst[0] = (unsigned char)(val & 0xFF);
    dst[1] = (unsigned char)((val >> 8) & 0xFF);
    dst[2] = (unsigned char)((val >> 16) & 0xFF);
    dst[3] = (unsigned char)((val >> 24) & 0xFF);
}

static void write_le16(unsigned char *dst, uint16_t val) {
    dst[0] = (unsigned char)(val & 0xFF);
    dst[1] = (unsigned char)((val >> 8) & 0xFF);
}

void _start(void) {
    uint32_t width = 0, height = 0, pitch = 0;
    uint8_t bpp = 0;
    sys_fb_info(&width, &height, &pitch, &bpp);

    // Map the framebuffer. The call returns a fixed user‑space address.
    uint8_t *fb = (uint8_t *)sys_fb_mmap();

    // Open output file.
    const char *filename = "screenshot.bmp";
    int fd = sb_acquire(filename, 0x1 /*O_WRONLY*/ | 0x40 /*O_CREAT*/ | 0x200 /*O_TRUNC*/);
    if (fd < 0) {
        sb_terminate(1);
    }

    // BMP header preparation.
    // Row size must be aligned to 4‑byte boundary.
    uint32_t row_bytes = ((width * 3 + 3) & ~3u);
    uint32_t img_size = row_bytes * height;
    uint32_t file_size = 54 + img_size; // 14 (file hdr) + 40 (info hdr)

    unsigned char header[54] = {0};
    // BITMAPFILEHEADER (14 bytes)
    header[0] = 'B';
    header[1] = 'M';
    write_le32(&header[2], file_size);
    // bfReserved1 and bfReserved2 are already zero.
    write_le32(&header[10], 54); // offset to pixel data
    // BITMAPINFOHEADER (40 bytes)
    write_le32(&header[14], 40); // biSize
    write_le32(&header[18], width);
    // Negative height for top‑down bitmap.
    write_le32(&header[22], (uint32_t)(-(int32_t)height));
    write_le16(&header[26], 1); // biPlanes
    write_le16(&header[28], 24); // biBitCount
    // biCompression (0 = BI_RGB) already zero.
    write_le32(&header[34], img_size); // biSizeImage
    // Remaining fields (XPelsPerMeter, YPelsPerMeter, ClrUsed, ClrImportant) stay zero.

    // Write header.
    sb_push(fd, header, 54);

    // Allocate a temporary row buffer once.
    unsigned char *row_buf = (unsigned char *)sys_sbrk(row_bytes);
    // Fill and write each row.
    for (uint32_t y = 0; y < height; ++y) {
        // Zero the row buffer to ensure padding bytes are 0.
        for (uint32_t i = 0; i < row_bytes; ++i) {
            row_buf[i] = 0;
        }
        uint8_t *src = fb + y * pitch;
        for (uint32_t x = 0; x < width; ++x) {
            uint8_t b = 0, g = 0, r = 0;
            if (bpp >= 24) {
                if (bpp == 32) {
                    b = src[x * 4];
                    g = src[x * 4 + 1];
                    r = src[x * 4 + 2];
                } else { // assume 24‑bpp
                    b = src[x * 3];
                    g = src[x * 3 + 1];
                    r = src[x * 3 + 2];
                }
            }
            // BMP expects B, G, R order.
            row_buf[x * 3]     = b;
            row_buf[x * 3 + 1] = g;
            row_buf[x * 3 + 2] = r;
        }
        sb_push(fd, row_buf, row_bytes);
    }

    sb_release(fd);
    sb_terminate(0);
}
