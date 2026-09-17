#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stddef.h>

#pragma function(memset,memcpy)
void * __cdecl memset(void *dst,int value,size_t count){BYTE *p=(BYTE*)dst;while(count--)*p++=(BYTE)value;return dst;}
void * __cdecl memcpy(void *dst,const void *src,size_t count){BYTE *d=(BYTE*)dst;const BYTE *s=(const BYTE*)src;while(count--)*d++=*s++;return dst;}

typedef ULONG_PTR HCRYPTPROV;
typedef struct {
    DWORD lStructSize; HWND hwndOwner; HINSTANCE hInstance; LPCWSTR lpstrFilter;
    LPWSTR lpstrCustomFilter; DWORD nMaxCustFilter; DWORD nFilterIndex; LPWSTR lpstrFile;
    DWORD nMaxFile; LPWSTR lpstrFileTitle; DWORD nMaxFileTitle; LPCWSTR lpstrInitialDir;
    LPCWSTR lpstrTitle; DWORD Flags; WORD nFileOffset; WORD nFileExtension;
    LPCWSTR lpstrDefExt; LPARAM lCustData; void *lpfnHook; LPCWSTR lpTemplateName;
    void *pvReserved; DWORD dwReserved; DWORD FlagsEx;
} OPENFILENAMEW_XP;
typedef BOOL (WINAPI *CryptAcquireContextWProc)(HCRYPTPROV*,LPCWSTR,LPCWSTR,DWORD,DWORD);
typedef BOOL (WINAPI *CryptGenRandomProc)(HCRYPTPROV,DWORD,BYTE*);
typedef BOOL (WINAPI *CryptReleaseContextProc)(HCRYPTPROV,DWORD);
typedef BOOL (WINAPI *GetSaveFileNameWProc)(OPENFILENAMEW_XP*);
#define PROV_RSA_FULL 1
#define CRYPT_VERIFYCONTEXT 0xF0000000
#define CRYPT_SILENT 0x00000040
#define OFN_OVERWRITEPROMPT 0x00000002
#define OFN_PATHMUSTEXIST 0x00000800

#define APP_NAME L"AES-256 Key Generator"
#define ID_MINUS 101
#define ID_PLUS 102
#define ID_GENERATE 103
#define ID_COPY 104
#define ID_COPYALL 105
#define ID_SAVE 106
#define ID_LANG 107
#define ID_THEME 108
#define ID_LIST 109
#define ID_COUNT 110
#define ID_ABOUT 111
#define MAX_KEYS 100

typedef struct {
    const wchar_t *title, *subtitle, *count, *generate, *copy, *copyAll,
                  *save, *empty, *saved, *saveError, *randomError, *invalidCount, *language, *theme;
} Strings;

static const Strings texts[3] = {
    {L"Генератор ключей AES-256", L"Криптографически стойкие 256-битные ключи", L"Количество",
     L"Сгенерировать", L"Копировать выбранное", L"Копировать всё", L"Сохранить",
     L"Нажмите «Сгенерировать», чтобы создать ключи", L"Ключи сохранены", L"Не удалось сохранить файл",
     L"Не удалось получить случайные данные", L"Введите количество ключей от 1 до 100", L"RU", L"Тема"},
    {L"AES-256 Key Generator", L"Cryptographically secure 256-bit keys", L"Key count",
     L"Generate", L"Copy selected", L"Copy all", L"Save",
     L"Press Generate to create keys", L"Keys saved", L"Could not save the file",
     L"Could not obtain random data", L"Enter a key count from 1 to 100", L"EN", L"Theme"},
    {L"AES-256 密钥生成器", L"加密安全的 256 位密钥", L"密钥数量",
     L"生成", L"复制所选", L"复制全部", L"保存",
     L"点击“生成”以创建密钥", L"密钥已保存", L"无法保存文件",
     L"无法获取随机数据", L"请输入 1 到 100 之间的密钥数量", L"中文", L"主题"}
};

static const wchar_t *aboutLabels[3]={L"О программе",L"About",L"关于"};
static const wchar_t *aboutTexts[3]={
    L"Автономный генератор криптографически стойких 256-битных ключей AES.\r\n\r\nВерсия 1.0.0\r\n\r\nhttp://dmr-zone.ru/",
    L"A standalone generator of cryptographically secure 256-bit AES keys.\r\n\r\nVersion 1.0.0\r\n\r\nhttp://dmr-zone.ru/",
    L"独立运行的加密安全 AES 256 位密钥生成器。\r\n\r\n版本 1.0.0\r\n\r\nhttp://dmr-zone.ru/"
};

static HWND mainWnd, listWnd;
static HWND countEdit;
static HWND buttons[9];
static HINSTANCE appInstance;
static WNDPROC oldEditProc;
static HFONT fontNormal, fontTitle, fontMono;
static HBRUSH bgBrush;
static int language = 0, darkMode = 0, keyCount = 16, countValid = 1, generated = 0;
static wchar_t keys[MAX_KEYS][65];

static void showAppMessage(const wchar_t *message) {
    MSGBOXPARAMSW p;
    ZeroMemory(&p,sizeof(p));p.cbSize=sizeof(p);p.hwndOwner=mainWnd;p.hInstance=appInstance;
    p.lpszText=message;p.lpszCaption=texts[language].title;p.dwStyle=MB_OK|MB_USERICON;
    p.lpszIcon=MAKEINTRESOURCEW(1);MessageBoxIndirectW(&p);
}

static void syncCountEdit(HWND owner) {
    wchar_t value[8];HWND edit=GetDlgItem(owner,ID_COUNT);wsprintfW(value,L"%d",keyCount);SetWindowTextW(edit,value);
}

static int readCountControl(HWND edit) {
    wchar_t value[8];int i,n=0,len;
    len=GetWindowTextW(edit,value,8);if(len<1)return 0;
    for(i=0;i<len;i++){if(value[i]<L'0'||value[i]>L'9')return 0;n=n*10+(value[i]-L'0');}
    return n;
}

static LRESULT CALLBACK CountEditProc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp) {
    static int normalizing=0;
    LRESULT result=CallWindowProcW(oldEditProc,hwnd,msg,wp,lp);
    if(!normalizing&&(msg==WM_SETTEXT||msg==WM_CHAR||msg==WM_PASTE||msg==WM_CUT||msg==WM_CLEAR)){
        int length=GetWindowTextLengthW(hwnd),n=readCountControl(hwnd);
        if(length>0&&n<1){normalizing=1;keyCount=1;SetWindowTextW(hwnd,L"1");SendMessageW(hwnd,EM_SETSEL,-1,-1);normalizing=0;countValid=1;}
        else if(n>100){normalizing=1;keyCount=100;SetWindowTextW(hwnd,L"100");SendMessageW(hwnd,EM_SETSEL,-1,-1);normalizing=0;countValid=1;}
        else {countValid=(length>0);if(countValid)keyCount=n;}
    }
    return result;
}

static void showAbout(void) {
    MSGBOXPARAMSW p;ZeroMemory(&p,sizeof(p));p.cbSize=sizeof(p);p.hwndOwner=mainWnd;p.hInstance=appInstance;
    p.lpszText=aboutTexts[language];p.lpszCaption=aboutLabels[language];p.dwStyle=MB_OK|MB_USERICON;
    p.lpszIcon=MAKEINTRESOURCEW(1);MessageBoxIndirectW(&p);
}

static COLORREF bgColor(void) { return darkMode ? RGB(24,27,32) : RGB(246,247,249); }
static COLORREF panelColor(void) { return darkMode ? RGB(35,39,46) : RGB(255,255,255); }
static COLORREF textColor(void) { return darkMode ? RGB(236,239,244) : RGB(31,35,41); }
static COLORREF mutedColor(void) { return darkMode ? RGB(159,166,178) : RGB(103,111,123); }
static COLORREF accentColor(void) { return RGB(47,111,237); }

static void copyText(const wchar_t *s) {
    SIZE_T bytes;
    HGLOBAL h;
    wchar_t *p;
    if (!s || !OpenClipboard(mainWnd)) return;
    EmptyClipboard();
    bytes = (lstrlenW(s) + 1) * sizeof(wchar_t);
    h = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (h) {
        p = (wchar_t*)GlobalLock(h);
        if (p) { CopyMemory(p, s, bytes); GlobalUnlock(h); SetClipboardData(CF_UNICODETEXT, h); h = NULL; }
    }
    if (h) GlobalFree(h);
    CloseClipboard();
}

static wchar_t *allKeysText(void) {
    int i;
    SIZE_T chars = generated ? (SIZE_T)generated * 67 + 1 : 1;
    wchar_t *out = (wchar_t*)GlobalAlloc(GPTR, chars * sizeof(wchar_t));
    if (!out) return NULL;
    for (i = 0; i < generated; ++i) {
        lstrcatW(out, keys[i]);
        if (i + 1 < generated) lstrcatW(out, L"\r\n");
    }
    return out;
}

static wchar_t *selectedKeysText(void) {
    int i, selected=(int)SendMessageW(listWnd,LB_GETSELCOUNT,0,0), added=0;
    wchar_t *out;
    if(selected<=0)return NULL;
    out=(wchar_t*)GlobalAlloc(GPTR,((SIZE_T)selected*67+1)*sizeof(wchar_t));if(!out)return NULL;
    for(i=0;i<generated;i++)if(SendMessageW(listWnd,LB_GETSEL,i,0)>0){
        if(added++)lstrcatW(out,L"\r\n");lstrcatW(out,keys[i]);
    }
    return out;
}

static int makeKeys(void) {
    HMODULE lib;
    CryptAcquireContextWProc acquire;
    CryptGenRandomProc randomBytes;
    CryptReleaseContextProc release;
    HCRYPTPROV provider = 0;
    BYTE data[32];
    static const wchar_t hex[] = L"0123456789abcdef";
    int i, j;
    lib=LoadLibraryW(L"advapi32.dll");if(!lib)return 0;
    acquire=(CryptAcquireContextWProc)GetProcAddress(lib,"CryptAcquireContextW");
    randomBytes=(CryptGenRandomProc)GetProcAddress(lib,"CryptGenRandom");
    release=(CryptReleaseContextProc)GetProcAddress(lib,"CryptReleaseContext");
    if(!acquire||!randomBytes||!release){FreeLibrary(lib);return 0;}
    if (!acquire(&provider, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_SILENT)) {FreeLibrary(lib);return 0;}
    for (i = 0; i < keyCount; ++i) {
        if (!randomBytes(provider, 32, data)) { release(provider, 0);FreeLibrary(lib);return 0; }
        for (j = 0; j < 32; ++j) {
            keys[i][j*2] = hex[data[j] >> 4];
            keys[i][j*2+1] = hex[data[j] & 15];
        }
        keys[i][64] = 0;
    }
    release(provider, 0);FreeLibrary(lib);
    generated = keyCount;
    SendMessageW(listWnd, LB_RESETCONTENT, 0, 0);
    for (i = 0; i < generated; ++i) SendMessageW(listWnd, LB_ADDSTRING, 0, (LPARAM)keys[i]);
    SendMessageW(listWnd, LB_SETSEL, TRUE, 0);
    return 1;
}

static void setButtonText(int idx, const wchar_t *s) { SetWindowTextW(buttons[idx], s); }

static void updateLanguage(void) {
    const Strings *t = &texts[language];
    SetWindowTextW(mainWnd, t->title);
    setButtonText(0, L"−"); setButtonText(1, L"+"); setButtonText(2, t->generate);
    setButtonText(3, t->copy); setButtonText(4, t->copyAll); setButtonText(5, t->save);
    setButtonText(6, t->language); setButtonText(7, darkMode ? L"☀" : L"☾");
    setButtonText(8, L"");
    InvalidateRect(mainWnd, NULL, TRUE);
}

static void saveKeys(void) {
    OPENFILENAMEW_XP ofn;
    HMODULE lib;
    GetSaveFileNameWProc saveDialog;
    wchar_t path[MAX_PATH] = L"aes-256-keys.txt";
    wchar_t filter[] = L"Text files (*.txt)\0*.txt\0All files (*.*)\0*.*\0\0";
    HANDLE f;
    DWORD written;
    int i;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = mainWnd; ofn.lpstrFilter = filter;
    ofn.lpstrFile = path; ofn.nMaxFile = MAX_PATH; ofn.lpstrDefExt = L"txt";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    lib=LoadLibraryW(L"comdlg32.dll");if(!lib)return;
    saveDialog=(GetSaveFileNameWProc)GetProcAddress(lib,"GetSaveFileNameW");
    if(!saveDialog||!saveDialog(&ofn)){FreeLibrary(lib);return;}FreeLibrary(lib);
    f = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (f == INVALID_HANDLE_VALUE) { showAppMessage(texts[language].saveError); return; }
    { BYTE bom[3] = {0xEF,0xBB,0xBF}; WriteFile(f, bom, 3, &written, NULL); }
    for (i = 0; i < generated; ++i) {
        char line[68]; int j;
        for (j = 0; j < 64; ++j) line[j] = (char)keys[i][j];
        line[64] = '\r'; line[65] = '\n';
        WriteFile(f, line, 66, &written, NULL);
    }
    CloseHandle(f);
    showAppMessage(texts[language].saved);
}

static void roundedRect(HDC dc, RECT r, COLORREF c, int radius) {
    HBRUSH b = CreateSolidBrush(c); HPEN p = CreatePen(PS_SOLID, 1, c);
    HGDIOBJ ob = SelectObject(dc,b), op = SelectObject(dc,p);
    RoundRect(dc,r.left,r.top,r.right,r.bottom,radius,radius);
    SelectObject(dc,ob); SelectObject(dc,op); DeleteObject(b); DeleteObject(p);
}

static void drawButton(const DRAWITEMSTRUCT *d) {
    wchar_t label[80]; RECT r = d->rcItem; COLORREF c;
    int primary = (d->CtlID == ID_GENERATE);
    GetWindowTextW(d->hwndItem,label,80);
    c = primary ? accentColor() : (darkMode ? RGB(49,54,63) : RGB(232,235,240));
    if (d->itemState & ODS_SELECTED) c = primary ? RGB(35,85,185) : (darkMode ? RGB(62,68,78) : RGB(215,220,228));
    FillRect(d->hDC,&r,bgBrush); InflateRect(&r,-2,-2); roundedRect(d->hDC,r,c,10);
    SetBkMode(d->hDC,TRANSPARENT); SetTextColor(d->hDC,primary ? RGB(255,255,255) : textColor());
    if(d->CtlID==ID_ABOUT){
        HPEN ip=CreatePen(PS_SOLID,2,textColor());HGDIOBJ oldp=SelectObject(d->hDC,ip);
        HGDIOBJ oldb=SelectObject(d->hDC,GetStockObject(NULL_BRUSH));RECT ir;
        int cx=(r.left+r.right)/2,cy=(r.top+r.bottom)/2;
        SetRect(&ir,cx-10,cy-10,cx+10,cy+10);
        Ellipse(d->hDC,ir.left,ir.top,ir.right,ir.bottom);
        MoveToEx(d->hDC,cx,cy-1,NULL);LineTo(d->hDC,cx,cy+6);
        SetPixel(d->hDC,cx,cy-5,textColor());SetPixel(d->hDC,cx-1,cy-5,textColor());
        SelectObject(d->hDC,oldb);SelectObject(d->hDC,oldp);DeleteObject(ip);
    }else{SelectObject(d->hDC,fontNormal);DrawTextW(d->hDC,label,-1,&r,DT_CENTER|DT_VCENTER|DT_SINGLELINE);}
}

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE: {
        int i; DWORD bs = WS_CHILD|WS_VISIBLE|BS_OWNERDRAW;
        mainWnd=hwnd;
        buttons[0]=CreateWindowW(L"BUTTON",L"−",bs,0,0,0,0,hwnd,(HMENU)ID_MINUS,NULL,NULL);
        buttons[1]=CreateWindowW(L"BUTTON",L"+",bs,0,0,0,0,hwnd,(HMENU)ID_PLUS,NULL,NULL);
        buttons[2]=CreateWindowW(L"BUTTON",L"",bs,0,0,0,0,hwnd,(HMENU)ID_GENERATE,NULL,NULL);
        buttons[3]=CreateWindowW(L"BUTTON",L"",bs,0,0,0,0,hwnd,(HMENU)ID_COPY,NULL,NULL);
        buttons[4]=CreateWindowW(L"BUTTON",L"",bs,0,0,0,0,hwnd,(HMENU)ID_COPYALL,NULL,NULL);
        buttons[5]=CreateWindowW(L"BUTTON",L"",bs,0,0,0,0,hwnd,(HMENU)ID_SAVE,NULL,NULL);
        buttons[6]=CreateWindowW(L"BUTTON",L"RU",bs,0,0,0,0,hwnd,(HMENU)ID_LANG,NULL,NULL);
        buttons[7]=CreateWindowW(L"BUTTON",L"☾",bs,0,0,0,0,hwnd,(HMENU)ID_THEME,NULL,NULL);
        buttons[8]=CreateWindowW(L"BUTTON",L"",bs,0,0,0,0,hwnd,(HMENU)ID_ABOUT,NULL,NULL);
        countEdit=CreateWindowExW(0,L"EDIT",L"16",WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_CENTER|ES_NUMBER|ES_AUTOHSCROLL,0,0,0,0,hwnd,(HMENU)ID_COUNT,NULL,NULL);
        oldEditProc=(WNDPROC)SetWindowLongW(countEdit,GWL_WNDPROC,(LONG)CountEditProc);
        SendMessageW(countEdit,EM_SETLIMITTEXT,3,0);SendMessageW(countEdit,WM_SETFONT,(WPARAM)fontTitle,TRUE);
        listWnd=CreateWindowExW(0,L"LISTBOX",L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_VSCROLL|LBS_NOTIFY|LBS_NOINTEGRALHEIGHT|LBS_OWNERDRAWFIXED|LBS_HASSTRINGS|LBS_EXTENDEDSEL,0,0,0,0,hwnd,(HMENU)ID_LIST,NULL,NULL);
        SendMessageW(listWnd,WM_SETFONT,(WPARAM)fontMono,TRUE);
        for(i=0;i<9;i++) SendMessageW(buttons[i],WM_SETFONT,(WPARAM)fontNormal,TRUE);
        updateLanguage(); return 0;
    }
    case WM_SIZE: {
        int w=LOWORD(lp), h=HIWORD(lp), pad=28, y=126;
        MoveWindow(buttons[6],w-202,24,54,36,TRUE); MoveWindow(buttons[7],w-140,24,50,36,TRUE); MoveWindow(buttons[8],w-78,24,50,36,TRUE);
        MoveWindow(buttons[0],pad,y,44,42,TRUE); MoveWindow(countEdit,76,y,98,42,TRUE); MoveWindow(buttons[1],178,y,44,42,TRUE);
        MoveWindow(buttons[2],238,y,w-266,42,TRUE);
        MoveWindow(listWnd,pad,188,w-2*pad,h-274,TRUE);
        y=h-66; MoveWindow(buttons[3],pad,y,(w-4*pad)/3,40,TRUE);
        MoveWindow(buttons[4],2*pad+(w-4*pad)/3,y,(w-4*pad)/3,40,TRUE);
        MoveWindow(buttons[5],3*pad+2*(w-4*pad)/3,y,(w-4*pad)/3,40,TRUE); return 0;
    }
    case WM_COMMAND: {
        int id=LOWORD(wp);
        if(id==ID_MINUS){if(keyCount>1){keyCount--;countValid=1;syncCountEdit(hwnd);}}
        else if(id==ID_PLUS){if(keyCount<100){keyCount++;countValid=1;syncCountEdit(hwnd);}}
        else if(id==ID_GENERATE){if(!countValid){showAppMessage(texts[language].invalidCount);SetFocus(countEdit);SendMessageW(countEdit,EM_SETSEL,0,-1);}else if(!makeKeys())showAppMessage(texts[language].randomError);else InvalidateRect(hwnd,NULL,TRUE);}
        else if(id==ID_COPY){wchar_t *s=selectedKeysText();if(s){copyText(s);GlobalFree(s);}}
        else if(id==ID_COPYALL && generated){wchar_t *s=allKeysText();if(s){copyText(s);GlobalFree(s);}}
        else if(id==ID_SAVE && generated)saveKeys();
        else if(id==ID_LANG){language=(language+1)%3;updateLanguage();}
        else if(id==ID_THEME){darkMode=!darkMode;DeleteObject(bgBrush);bgBrush=CreateSolidBrush(bgColor());updateLanguage();}
        else if(id==ID_ABOUT)showAbout();
        else if(id==ID_LIST && HIWORD(wp)==LBN_DBLCLK){wchar_t *s=selectedKeysText();if(s){copyText(s);GlobalFree(s);}}
        return 0;
    }
    case WM_DRAWITEM: if(wp==ID_LIST){
        DRAWITEMSTRUCT *d=(DRAWITEMSTRUCT*)lp; RECT r=d->rcItem;
        HBRUSH rowBrush;
        if(d->itemID==(UINT)-1)return TRUE;
        rowBrush=CreateSolidBrush((d->itemState&ODS_SELECTED)?accentColor():panelColor());
        FillRect(d->hDC,&r,rowBrush);DeleteObject(rowBrush);
        SetBkMode(d->hDC,TRANSPARENT);SetTextColor(d->hDC,(d->itemState&ODS_SELECTED)?RGB(255,255,255):textColor());
        SelectObject(d->hDC,fontMono);r.left+=14;DrawTextW(d->hDC,keys[d->itemID],-1,&r,DT_LEFT|DT_VCENTER|DT_SINGLELINE);return TRUE;
    } else {drawButton((DRAWITEMSTRUCT*)lp);return TRUE;}
    case WM_MEASUREITEM: if(wp==ID_LIST){((MEASUREITEMSTRUCT*)lp)->itemHeight=34;return TRUE;}break;
    case WM_CTLCOLORLISTBOX: SetTextColor((HDC)wp,textColor());SetBkColor((HDC)wp,panelColor());return (LRESULT)GetStockObject(darkMode?BLACK_BRUSH:WHITE_BRUSH);
    case WM_CTLCOLOREDIT: SetTextColor((HDC)wp,textColor());SetBkColor((HDC)wp,panelColor());return (LRESULT)GetStockObject(darkMode?BLACK_BRUSH:WHITE_BRUSH);
    case WM_ERASEBKGND: {RECT r;GetClientRect(hwnd,&r);FillRect((HDC)wp,&r,bgBrush);return 1;}
    case WM_PAINT: {
        PAINTSTRUCT ps;HDC dc=BeginPaint(hwnd,&ps);RECT r;
        SetBkMode(dc,TRANSPARENT);SetTextColor(dc,textColor());SelectObject(dc,fontTitle);SetRect(&r,28,24,530,62);DrawTextW(dc,texts[language].title,-1,&r,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
        SetTextColor(dc,mutedColor());SelectObject(dc,fontNormal);SetRect(&r,28,64,650,92);DrawTextW(dc,texts[language].subtitle,-1,&r,DT_LEFT|DT_SINGLELINE);
        SetTextColor(dc,textColor());SetRect(&r,28,100,220,125);DrawTextW(dc,texts[language].count,-1,&r,DT_LEFT|DT_SINGLELINE);
        if(!generated){GetClientRect(hwnd,&r);r.top=240;r.bottom-=110;SetTextColor(dc,mutedColor());SelectObject(dc,fontNormal);DrawTextW(dc,texts[language].empty,-1,&r,DT_CENTER|DT_VCENTER|DT_SINGLELINE);}
        EndPaint(hwnd,&ps);return 0;
    }
    case WM_DESTROY: PostQuitMessage(0);return 0;
    }
    return DefWindowProcW(hwnd,msg,wp,lp);
}

int WINAPI wWinMain(HINSTANCE instance,HINSTANCE prev,LPWSTR cmd,int show) {
    WNDCLASSW wc; MSG msg;
    (void)prev;(void)cmd;
    fontNormal=CreateFontW(-17,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
    fontTitle=CreateFontW(-25,0,0,0,FW_SEMIBOLD,0,0,0,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
    fontMono=CreateFontW(-16,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,FIXED_PITCH,L"Courier New");
    appInstance=instance;bgBrush=CreateSolidBrush(bgColor());ZeroMemory(&wc,sizeof(wc));wc.style=CS_HREDRAW|CS_VREDRAW;wc.lpfnWndProc=WindowProc;wc.hInstance=instance;wc.hCursor=LoadCursor(NULL,IDC_ARROW);wc.hIcon=LoadIconW(instance,MAKEINTRESOURCEW(1));wc.hbrBackground=bgBrush;wc.lpszClassName=L"AES256KeyGenWindow";RegisterClassW(&wc);
    mainWnd=CreateWindowExW(0,wc.lpszClassName,APP_NAME,WS_OVERLAPPEDWINDOW & ~(WS_MAXIMIZEBOX|WS_THICKFRAME),CW_USEDEFAULT,CW_USEDEFAULT,820,650,NULL,NULL,instance,NULL);
    if(!mainWnd)return 1;ShowWindow(mainWnd,show);UpdateWindow(mainWnd);
    while(GetMessageW(&msg,NULL,0,0)>0){TranslateMessage(&msg);DispatchMessageW(&msg);}return (int)msg.wParam;
}

void WINAPI WinMainEntry(void) {
    ExitProcess((UINT)wWinMain(GetModuleHandleW(NULL),NULL,GetCommandLineW(),SW_SHOWDEFAULT));
}
