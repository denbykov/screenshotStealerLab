#include <windows.h>

#include <iostream>
#include <filesystem>
#include <chrono>
#include <thread>

using namespace std::chrono;

bool saveBmp(HBITMAP hBitmap, const std::filesystem::path filePath) {
    BITMAP bmp;
    if (GetObject(hBitmap, sizeof(BITMAP), &bmp)) {
        HDC hDC = CreateCompatibleDC(NULL);
        SelectObject(hDC, hBitmap);
        int bitsPerPixel = bmp.bmBitsPixel;
        int bytesPerPixel = bitsPerPixel / 8;
        int bitmapSize = bmp.bmWidth * bmp.bmHeight * bytesPerPixel;
        char* bitmapData = new char[bitmapSize];
        BITMAPFILEHEADER bfh;
        BITMAPINFOHEADER bih;
        bih.biSize = sizeof(BITMAPINFOHEADER);
        bih.biWidth = bmp.bmWidth;
        bih.biHeight = bmp.bmHeight;
        bih.biPlanes = 1;
        bih.biBitCount = bitsPerPixel;
        bih.biCompression = BI_RGB;
        bih.biSizeImage = 0;
        bih.biXPelsPerMeter = 0;
        bih.biYPelsPerMeter = 0;
        bih.biClrUsed = 0;
        bih.biClrImportant = 0;
        bfh.bfType = 0x4D42;
        bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
        bfh.bfSize = bfh.bfOffBits + bitmapSize;
        if (GetDIBits(hDC, hBitmap, 0, bmp.bmHeight, bitmapData, (BITMAPINFO*)&bih, DIB_RGB_COLORS)) {
            FILE* file = fopen(filePath.string().c_str(), "wb");
            if (file) {
                fwrite(&bfh, sizeof(BITMAPFILEHEADER), 1, file);
                fwrite(&bih, sizeof(BITMAPINFOHEADER), 1, file);
                fwrite(bitmapData, 1, bitmapSize, file);
                fclose(file);
                delete[] bitmapData;
                DeleteDC(hDC);
                return true;
            }
        }
        delete[] bitmapData;
        DeleteDC(hDC);
    }
    return false;
}

HBITMAP makeScreenshot() {
    HDC screenDC = GetDC(NULL);

    HDC memDC = CreateCompatibleDC(screenDC);

    int screenX = GetSystemMetrics(SM_CXSCREEN);
    int screenY = GetSystemMetrics(SM_CYSCREEN);

    HBITMAP hBitmap = CreateCompatibleBitmap(screenDC, screenX, screenY);

    SelectObject(memDC, hBitmap);

    BitBlt(memDC, 0, 0, screenX, screenY, screenDC, 0, 0, SRCCOPY);

    DeleteDC(memDC);
    ReleaseDC(NULL, screenDC);

    return hBitmap;
}

void handleUserInput() {
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

int main() {
    handleUserInput();

    auto bitmap = makeScreenshot();

    if (saveBmp(bitmap, "screenshot.bmp")) {
        std::cout << "Screenshot saved successfully." << std::endl;
    }
    else {
        std::cerr << "Failed to save the screenshot." << std::endl;
    }

    DeleteObject(bitmap);

    return 0;
}