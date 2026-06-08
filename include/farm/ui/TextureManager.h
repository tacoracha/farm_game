#pragma once

#ifdef _WIN32

#include <windows.h>

#include <gdiplus.h>
#include <map>
#include <memory>
#include <string>

namespace farm::ui {

class TextureManager {
public:
    TextureManager();
    ~TextureManager();

    TextureManager(const TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;

    bool Load(const std::wstring& key, const std::wstring& preferred_png,
              const std::wstring& fallback_ppm);
    bool Draw(HDC hdc, const std::wstring& key, const RECT& rect) const;

private:
    struct Texture {
        std::unique_ptr<Gdiplus::Bitmap> png;
        HBITMAP ppm_bitmap = nullptr;
        int width = 0;
        int height = 0;
    };

    bool LoadPpm(const std::wstring& path, Texture& texture) const;
    void Release(Texture& texture) const;

    ULONG_PTR gdiplus_token_ = 0;
    std::map<std::wstring, Texture> textures_;
};

}  // namespace farm::ui

#endif

