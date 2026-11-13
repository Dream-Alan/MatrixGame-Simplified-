//  模块 4：ui.h —— Win32 界面的最小封装
//
//  主菜单和游戏界面都在同一窗口里换一批控件。
#pragma once

#include <windows.h>
#include <functional>
#include <string>
#include <vector>
using namespace std;

namespace ui {
int px(int logical);                       // 逻辑像素 → 物理像素，全程序只在这里乘一次 DPI

class Window {
public:
    bool create(const wstring& title, int width, int height);        // 尺寸是**逻辑**客户区尺寸
    // ---- 加控件：坐标和尺寸都用逻辑像素 ----
    HWND addButton(const wstring& text, int x, int y, int w, int h, function<void()> onClick);
    HWND addLabel(const wstring& text, int x, int y, int w, int h, int fontSize);
    HWND addCell(int x, int y, int w, int h, int fontSize, function<void()> onClick);

    void clear();                          // 销毁全部控件，准备画下一个界面
    void setText(HWND h, const wstring& text);           // 改控件上的文字
    void setBackColor(HWND h, COLORREF color);           // 改控件底色
    void setTitle(const wstring& title);                 // 改窗口标题
    void resize(int width, int height);                  // 按逻辑尺寸重设窗口
    void post(function<void()> fn);        // 延迟执行：在按钮回调里重建界面必须走它
    void onKey(function<void(wchar_t key, bool ctrl)> fn) { onKey_ = fn; }   // 注册按键回调
    HWND handle() const { return hwnd_; }
    int  width() const;                                  // 逻辑宽度
    int  height() const;                                 // 逻辑高度
    static int runLoop();                                // 消息循环，窗口关闭后返回

private:
    // 底层创建控件的公共函数，上面三个 addXxx 都是它的"薄壳"
    HWND makeControl(const wchar_t* cls, const wstring& text, int x, int y, int w, int h,
                     DWORD style, int fontSize, function<void()> onClick);
    HFONT  fontFor(int pointSize);                       // 按字号取字体（带缓存）
    HBRUSH brushFor(COLORREF color);                     // 按颜色取画刷（带缓存）
    static LRESULT CALLBACK wndProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT handleMessage(HWND, UINT, WPARAM, LPARAM);

    static Window* instance_;                            // 全程只有一个窗口，靠它转发按键
    HWND hwnd_ = nullptr;
    vector<HWND> controls_;                              // 本窗口创建的所有控件
    vector<function<void()>> callbacks_;                 // 控件 id - 1 就是下标
    vector<pair<HWND, COLORREF>> backColors_;            // 自己设过底色的控件
    vector<pair<int, HFONT>> fonts_;                     // 字体缓存：字号 → 字体句柄
    vector<pair<COLORREF, HBRUSH>> brushes_;             // 画刷缓存：颜色 → 画刷句柄
    function<void(wchar_t, bool)> onKey_;                 // 按键回调
    function<void()> pending_;                           // post() 挂起的动作
};

void alert(const wstring& text, const wstring& caption = L"提示");
bool confirm(const wstring& text, const wstring& caption = L"确认");
}  // namespace ui
