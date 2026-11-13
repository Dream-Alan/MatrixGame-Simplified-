//  模块 4：ui.cpp —— Win32 界面封装的实现
//
#include "ui.h"

namespace ui {
Window* Window::instance_ = nullptr;                 // 全程只有一个窗口

namespace {
const wchar_t kClassName[] = L"LagLiteWindow";
const wchar_t* kFontName = L"Microsoft YaHei";       // 微软雅黑，中文好看

// 屏幕 DPI：96 = 100%，120 = 125%，144 = 150%。用 static 缓存，只查一次。
int dpiValue() {
    static int value = 0;
    if (value == 0) {
        HDC dc = GetDC(nullptr);
        value = GetDeviceCaps(dc, LOGPIXELSX);
        ReleaseDC(nullptr, dc);
        if (value <= 0) value = 96;                  // 查不到就按 100% 处理
    }
    return value;
}

int clientSpan(HWND h, bool vertical) {
    RECT rc = {0, 0, 0, 0};
    if (h) GetClientRect(h, &rc);
    const int phys = vertical ? (rc.bottom - rc.top) : (rc.right - rc.left);
    return int(phys * 96.0 / dpiValue() + 0.5);      // +0.5 是为了四舍五入
}
}  // namespace

// 逻辑像素 → 物理像素。整个程序只在这里乘一次 DPI，
// 所以不会出现"偏移量被缩放两次、界面跑到右下角"的问题。
int px(int logical) { return int(logical * dpiValue() / 96.0 + 0.5); }

int Window::width() const { return clientSpan(hwnd_, false); }
int Window::height() const { return clientSpan(hwnd_, true); }

bool Window::create(const wstring& title, int width, int height) {
    instance_ = this;
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = &Window::wndProc;               // 消息处理函数
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);     // 鼠标指针用标准箭头
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.lpszClassName = kClassName;
    RegisterClassExW(&wc);
    // 固定样式：有标题栏和最小化按钮，但不能拖动改变大小
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    // 我们要的是"客户区"大小，CreateWindow 要的是"整窗"大小，
    // 所以先让 AdjustWindowRectEx 把标题栏和边框的厚度算进去
    RECT rc = {0, 0, px(width), px(height)};
    AdjustWindowRectEx(&rc, style, FALSE, 0);
    hwnd_ = CreateWindowExW(0, kClassName, title.c_str(), style, CW_USEDEFAULT, CW_USEDEFAULT,
                            rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr,
                            wc.hInstance, this);
    return hwnd_ != nullptr;
}

void Window::setTitle(const wstring& title) { if (hwnd_) SetWindowTextW(hwnd_, title.c_str()); }

void Window::resize(int width, int height) {
    if (!hwnd_) return;
    DWORD style = (DWORD)GetWindowLongPtrW(hwnd_, GWL_STYLE);
    RECT rc = {0, 0, px(width), px(height)};
    AdjustWindowRectEx(&rc, style, FALSE, 0);
    SetWindowPos(hwnd_, nullptr, 0, 0, rc.right - rc.left, rc.bottom - rc.top,
                 SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}

// 按字号取字体（同一字号只创建一次，之后从缓存里拿，因为矩阵有很多格子）
HFONT Window::fontFor(int pointSize) {
    for (const auto& f : fonts_) if (f.first == pointSize) return f.second;
    // 字号单位是"点"，换算成像素高度（1 点 = 1/72 英寸）
    HFONT font = CreateFontW(-px(pointSize * 96 / 72), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             CLEARTYPE_QUALITY, DEFAULT_PITCH, kFontName);
    fonts_.push_back({pointSize, font});
    return font;
}

HBRUSH Window::brushFor(COLORREF color) {            // 按颜色取画刷，同样带缓存
    for (const auto& b : brushes_) if (b.first == color) return b.second;
    HBRUSH brush = CreateSolidBrush(color);
    brushes_.push_back({color, brush});
    return brush;
}

HWND Window::makeControl(const wchar_t* cls, const wstring& text, int x, int y, int w, int h,
                         DWORD style, int fontSize, function<void()> onClick) {
    const int id = int(callbacks_.size()) + 1;       // 回调下标 + 1 就是控件 id
    callbacks_.push_back(onClick);
    HWND child = CreateWindowExW(0, cls, text.c_str(), style, px(x), px(y), px(w), px(h), hwnd_,
                                 (HMENU)(INT_PTR)id,               // Win32 要求把 id 塞进 HMENU
                                 GetModuleHandleW(nullptr), nullptr);
    SendMessageW(child, WM_SETFONT, (WPARAM)fontFor(fontSize), TRUE);
    controls_.push_back(child);
    backColors_.push_back({child, RGB(255, 255, 255)});            // 默认白底
    return child;
}

HWND Window::addButton(const wstring& text, int x, int y, int w, int h, function<void()> onClick) {
    return makeControl(L"BUTTON", text, x, y, w, h,
                       WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP, 11, onClick);
}

HWND Window::addLabel(const wstring& text, int x, int y, int w, int h, int fontSize) {
    return makeControl(L"STATIC", text, x, y, w, h,
                       WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE, fontSize, nullptr);
}

HWND Window::addCell(int x, int y, int w, int h, int fontSize, function<void()> onClick) {
    return makeControl(L"STATIC", L"", x, y, w, h,
                       WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE | SS_NOTIFY | WS_BORDER,
                       fontSize, onClick);
}

void Window::clear() {                               // 销毁全部控件，准备画下一个界面
    for (HWND child : controls_) if (child) DestroyWindow(child);
    controls_.clear();
    callbacks_.clear();
    backColors_.clear();
}

void Window::setText(HWND h, const wstring& text) { SetWindowTextW(h, text.c_str()); }

void Window::setBackColor(HWND h, COLORREF color) {
    for (auto& item : backColors_) {
        if (item.first != h) continue;
        item.second = color;
        InvalidateRect(h, nullptr, TRUE);            // 让系统重画这个格子
        return;
    }
}

// 延迟执行：把动作存起来，再给自己投递一条自定义消息，等当前消息处理完才执行，
// 避免"在自己的回调里把自己销毁"
void Window::post(function<void()> fn) {
    pending_ = fn;
    PostMessageW(hwnd_, WM_APP + 1, 0, 0);
}

LRESULT CALLBACK Window::wndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_NCCREATE) {                        // 创建时把 this 存进窗口，之后每条消息取回来
        CREATESTRUCTW* cs = (CREATESTRUCTW*)lp;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
    }
    Window* self = (Window*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    if (self) return self->handleMessage(hwnd, msg, wp, lp);
    return DefWindowProcW(hwnd, msg, wp, lp);
}

LRESULT Window::handleMessage(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_COMMAND) {                         // 按钮或单元格被点击
        const int id = LOWORD(wp);                   // 控件 id，减 1 就能找回回调
        if (id >= 1 && id <= int(callbacks_.size())) {
            function<void()> callback = callbacks_[id - 1];   // 拷一份，回调里可能会 clear()
            if (callback) callback();
        }
        return 0;
    }
    if (msg == WM_CTLCOLORSTATIC) {                  // 系统要画静态控件，先问我们要底色
        HDC dc = (HDC)wp;
        HWND child = (HWND)lp;
        COLORREF color = RGB(255, 255, 255);
        for (const auto& item : backColors_) if (item.first == child) { color = item.second; break; }
        SetTextColor(dc, RGB(0, 0, 0));              // 文字黑色
        SetBkColor(dc, color);
        return (LRESULT)brushFor(color);
    }
    if (msg == WM_APP + 1) {                         // post() 挂起的动作在这里执行
        function<void()> action = pending_;
        pending_ = nullptr;
        if (action) action();
        return 0;
    }
    if (msg == WM_DESTROY) { PostQuitMessage(0); return 0; }   // 窗口关了，退出消息循环
    return DefWindowProcW(hwnd, msg, wp, lp);        // 其它消息交给系统默认处理
}

// 消息循环：程序的主心跳。按键在这里统一截获，这样即使焦点落在某个按钮上，
// 按字母键也能生效（若只在窗口层收键盘消息，焦点在按钮上时按键会被按钮吃掉）。
int Window::runLoop() {
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        if (msg.message == WM_KEYDOWN && instance_ && instance_->onKey_) {
            wchar_t key = 0;
            if (msg.wParam >= 'A' && msg.wParam <= 'Z') key = wchar_t(msg.wParam - 'A' + 'a');
            else if (msg.wParam >= '0' && msg.wParam <= '9') key = wchar_t(msg.wParam);
            if (key) instance_->onKey_(key, (GetKeyState(VK_CONTROL) & 0x8000) != 0);
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);                      // 转交给 wndProc
    }
    return int(msg.wParam);
}

void alert(const wstring& text, const wstring& caption) {
    MessageBoxW(nullptr, text.c_str(), caption.c_str(), MB_OK | MB_ICONINFORMATION);
}

bool confirm(const wstring& text, const wstring& caption) {
    return MessageBoxW(nullptr, text.c_str(), caption.c_str(), MB_YESNO | MB_ICONQUESTION) == IDYES;
}
}  // namespace ui
