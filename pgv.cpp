/** Property Grid View Main Functions
 *
 **/

#include "PropertyGridView.h"

void InitPropertyGridView(PROPERTY_GRID* ppg, HWND hwnd, HINSTANCE hInst)
{
    NONCLIENTMETRICSW ncm;
    ncm.cbSize = sizeof(NONCLIENTMETRICSW);
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);

    // Init
    InitCommonControls();
    memset(ppg, 0, sizeof(PROPERTY_GRID));

    // Set scroll info
    SCROLLINFO si;
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_ALL;
    si.nMin = 0;
    si.nMax = 0;
    si.nPage = ppg->szControl.cy;
    si.nPos = 0;
    si.nTrackPos = 0;
    SetScrollInfo(hwnd, SB_VERT, &si, TRUE);

    HBITMAP bmp;
    RECT rc;
    HDC hdc = GetDC(hwnd);
    GetClientRect(hwnd, &rc);

    ppg->appInstance = hInst;
    ppg->hwnd = hwnd;
    ppg->hwndValueEdit = NULL;
    ppg->fontRegular = CreateFontIndirectW(&ncm.lfMenuFont);
    ncm.lfMenuFont.lfWeight = FW_BOLD;
    ppg->fontBold = CreateFontIndirectW(&ncm.lfMenuFont);
    ppg->brBackground = CreateSolidBrush(RGB(37, 37, 38));
    ppg->brItemFrame = CreateSolidBrush(RGB(45, 45, 48));
    ppg->clrBackground = RGB(37, 37, 38);
    ppg->clrTextDefault = RGB(241, 241, 241);
    ppg->clrTextDisable = RGB(153, 153, 153);
    ppg->clrSelect = RGB(51, 153, 255);
    ppg->dmItemHeight = 28;
    ppg->szControl.cx = rc.right - rc.left;
    ppg->szControl.cy = rc.bottom - rc.top;
    ppg->aspectRatioDivider = 0.5;
    ppg->posDivider = ppg->szControl.cx * ppg->aspectRatioDivider;
    ppg->ptDragPrevX = 0;
    ppg->bDragging = FALSE;
    ppg->procOldEditControl = NULL;

    // Load image resources
    ppg->hStateImageList = ImageList_Create(ppg->dmItemHeight, ppg->dmItemHeight, ILC_COLOR | ILC_MASK, 0, 0);
    // load collapse state image
    bmp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_STATE_COLLAPSE));
    PropertyGridAddImage(ppg->hStateImageList, bmp, RGB(255, 0, 255));
    DeleteObject(bmp);
    // load expand state icon
    bmp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_STATE_EXPAND));
    PropertyGridAddImage(ppg->hStateImageList, bmp, RGB(255, 0, 255));
    DeleteObject(bmp);
}

void ReleasePropertyGridView(PROPERTY_GRID* ppg)
{
    ImageList_Destroy(ppg->hStateImageList);
    DeleteObject(ppg->fontBold);
    DeleteObject(ppg->fontRegular);
    DeleteObject(ppg->brBackground);
    DeleteObject(ppg->brItemFrame);
    PropertyGridDeleteAll(ppg);
}

int PropertyGridAddImage(HIMAGELIST hImgList, HBITMAP bmp, COLORREF clrTransparent)
{
    if (!hImgList || !bmp) {
        return -1;
    }

    HDC hdc, hdcImg, hdcMem;
    HBITMAP bmpMem;
    BITMAP image;
    int w, h, idx;

    ImageList_GetIconSize(hImgList, &w, &h);
    hdc = GetDC(NULL);
    bmpMem = CreateCompatibleBitmap(hdc, w, h);
    hdcImg = CreateCompatibleDC(hdc);
    hdcMem = CreateCompatibleDC(hdc);

    // stretch
    GetObject(bmp, sizeof(BITMAP), &image);
    bmp = (HBITMAP)SelectObject(hdcImg, bmp);
    bmpMem = (HBITMAP)SelectObject(hdcMem, bmpMem);
    StretchBlt(hdcMem, 0, 0, w, h, hdcImg, 0, 0, image.bmWidth, image.bmHeight, SRCCOPY);
    bmp = (HBITMAP)SelectObject(hdcImg, bmp);
    bmpMem = (HBITMAP)SelectObject(hdcMem, bmpMem);

    idx = ImageList_AddMasked(hImgList, bmpMem, clrTransparent);

    DeleteObject(bmpMem);
    DeleteDC(hdcImg);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdc);

    return idx;
}

LONG PropertyGridGetScrollHeight(PROPERTY_GRID* ppg)
{
    PROPERTY_ITEM* current = ppg->content;
    LONG height = 0;

    while (current)
    {
        height += current->bVisible ? ppg->dmItemHeight : 0;
        current = current->next;
    }

    return height;
}

void PropertyGridUpdateClientSize(PROPERTY_GRID* ppg)
{
    RECT rc;
    GetClientRect(ppg->hwnd, &rc);
    ppg->szControl.cx = rc.right - rc.left;
    ppg->szControl.cy = rc.bottom - rc.top;
}

void PropertyGridUpdateScrollRange(PROPERTY_GRID* ppg)
{
    SCROLLINFO si;
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_RANGE | SIF_PAGE;
    si.nMin = 0;
    si.nMax = PropertyGridGetScrollHeight(ppg);
    si.nPage = ppg->szControl.cy;
    SetScrollInfo(ppg->hwnd, SB_VERT, &si, TRUE);
    PropertyGridUpdateClientSize(ppg);
}

void PropertyGridUpdateValueEditWindow(PROPERTY_GRID* ppg)
{
    if (ppg->itemSelect && ppg->hwndValueEdit)
    {
        RECT rc;
        PropertyGridItemGetValueRect(ppg, ppg->itemSelect, &rc);
        InflateRect(&rc, -1, -1);
        MoveWindow(ppg->hwndValueEdit,
            rc.left + VALUE_EDIT_TEXT_OFFSET, rc.top + 1, 
            rc.right - rc.left, rc.bottom - rc.top, TRUE);
    }
}
