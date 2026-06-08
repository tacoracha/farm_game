#ifdef _WIN32

#include "farm/ui/TextureManager.h"

#include <fstream>
#include <vector>

namespace farm::ui {

TextureManager::TextureManager() {
    Gdiplus::GdiplusStartupInput input;
    Gdiplus::GdiplusStartup(&gdiplus_token_, &input, nullptr);
}

TextureManager::~TextureManager() {
    for (auto& [key, texture] : textures_) {
        (void)key;
        Release(texture);
    }
    if (gdiplus_token_ != 0) {
        Gdiplus::GdiplusShutdown(gdiplus_token_);
    }
}

bool TextureManager::Load(const std::wstring& key, const std::wstring& preferred_png,
                          const std::wstring& fallback_ppm) {
    Texture texture;
    auto png = std::make_unique<Gdiplus::Bitmap>(preferred_png.c_str());
    if (png->GetLastStatus() == Gdiplus::Ok && png->GetWidth() > 0 && png->GetHeight() > 0) {
        texture.width = static_cast<int>(png->GetWidth());
        texture.height = static_cast<int>(png->GetHeight());
        texture.png = std::move(png);
        textures_[key] = std::move(texture);
        return true;
    }

    if (LoadPpm(fallback_ppm, texture)) {
        textures_[key] = std::move(texture);
        return true;
    }
    return false;
}

bool TextureManager::Draw(HDC hdc, const std::wstring& key, const RECT& rect) const {
    const auto it = textures_.find(key);
    if (it == textures_.end()) {
        return false;
    }
    const Texture& texture = it->second;
    if (texture.png) {
        Gdiplus::Graphics graphics(hdc);
        graphics.SetInterpolationMode(Gdiplus::InterpolationModeNearestNeighbor);
        graphics.DrawImage(texture.png.get(), static_cast<INT>(rect.left),
                           static_cast<INT>(rect.top),
                           static_cast<INT>(rect.right - rect.left),
                           static_cast<INT>(rect.bottom - rect.top));
        return true;
    }
    if (texture.ppm_bitmap != nullptr) {
        HDC memory_dc = CreateCompatibleDC(hdc);
        HGDIOBJ old = SelectObject(memory_dc, texture.ppm_bitmap);
        StretchBlt(hdc, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
                   memory_dc, 0, 0, texture.width, texture.height, SRCCOPY);
        SelectObject(memory_dc, old);
        DeleteDC(memory_dc);
        return true;
    }
    return false;
}

bool TextureManager::LoadPpm(const std::wstring& path, Texture& texture) const {
    std::ifstream in(path.c_str());
    if (!in) {
        return false;
    }
    std::string magic;
    int width = 0;
    int height = 0;
    int max_value = 0;
    in >> magic >> width >> height >> max_value;
    if (magic != "P3" || width <= 0 || height <= 0 || max_value <= 0) {
        return false;
    }

    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = width;
    info.bmiHeader.biHeight = -height;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;

    void* pixels = nullptr;
    HBITMAP bitmap = CreateDIBSection(nullptr, &info, DIB_RGB_COLORS, &pixels, nullptr, 0);
    if (bitmap == nullptr || pixels == nullptr) {
        return false;
    }
    auto* out = static_cast<unsigned char*>(pixels);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int r = 0;
            int g = 0;
            int b = 0;
            if (!(in >> r >> g >> b)) {
                DeleteObject(bitmap);
                return false;
            }
            const int offset = (y * width + x) * 4;
            out[offset + 0] = static_cast<unsigned char>(b);
            out[offset + 1] = static_cast<unsigned char>(g);
            out[offset + 2] = static_cast<unsigned char>(r);
            out[offset + 3] = 255;
        }
    }

    texture.ppm_bitmap = bitmap;
    texture.width = width;
    texture.height = height;
    return true;
}

void TextureManager::Release(Texture& texture) const {
    if (texture.ppm_bitmap != nullptr) {
        DeleteObject(texture.ppm_bitmap);
        texture.ppm_bitmap = nullptr;
    }
}

}  // namespace farm::ui

#endif
