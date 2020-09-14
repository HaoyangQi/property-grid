#pragma once

#include <windows.h>
#include <commctrl.h>
#include <windowsx.h>
#include "resource.h"

#define PROP_HIT_NOTHING 0
#define PROP_HIT_STATEICON 1
#define PROP_HIT_CATEGORY 2
#define PROP_HIT_KEY 3
#define PROP_HIT_VALUE 4
#define PROP_HIT_ITEM 5
#define PROP_HIT_DIVIDER 6

#define VALUE_EDIT_TEXT_OFFSET 2 // TODO: in fact, this is pretty hacky

#define PropertyGridItemIsVisibleWithin(item, beginY, endY) (\
	(item) != NULL && \
	(item)->bVisible && \
	(item)->rcItem.bottom >= (beginY) && \
	(item)->rcItem.top <= (endY))
#define PropertyGridNewCategory(p, k) PropertyGridNewProperty(p, k, NULL)

#include <wchar.h>
#define DebugPrintf(str, ...) {\
	wchar_t buf[256] = L"\0";\
	swprintf_s(buf, 256, str, __VA_ARGS__);\
	OutputDebugString(buf); }

typedef void* HPROPERTY;
typedef LPWSTR (__stdcall *PROCVERIFY)(void*, size_t);

typedef struct property_item {
	LPWSTR strKey;
	LPWSTR strValue;
	LPWSTR strValueOriginal;
	LPWSTR strDescription;
	COLORREF clrKey;
	COLORREF clrVal;
	LONG nLevel;
	RECT rcItem;
	BOOL bCollapse;
	BOOL bSelect;
	BOOL bDisable;
	BOOL bAllowEdit;
	BOOL bVisible;

	PROCVERIFY lpfnVerifyProc;

	void* data;
	size_t szData;

	struct property_item* next;
	struct property_item* parent;
} PROPERTY_ITEM;

typedef struct property_grid {
	HINSTANCE appInstance;
	HWND hwnd;
	HWND hwndValueEdit;
	HFONT fontBold;
	HFONT fontRegular;
	HIMAGELIST hStateImageList;
	HBRUSH brBackground;
	HBRUSH brItemFrame;

	COLORREF clrBackground;
	COLORREF clrTextDefault;
	COLORREF clrTextDisable;
	COLORREF clrSelect;
	LONG dmItemHeight;
	SIZE szControl;
	LONG posDivider;
	float aspectRatioDivider;

	LONG ptDragPrevX;
	BOOL bDragging;

	WNDPROC procOldEditControl;

	PROPERTY_ITEM* content;
	PROPERTY_ITEM* itemSelect;
} PROPERTY_GRID;

// Public Data Structure
typedef struct pg_item_staged_data
{
	NMHDR hdr;
	HPROPERTY property;
	LPVOID data;
	size_t size;
} PGITEM_STAGED_DATA;

// Public
BOOL isCategory(HPROPERTY);
HPROPERTY PropertyGridNewProperty(PROPERTY_GRID*, LPCWSTR, LPCWSTR);
void PropertyGridDeleteProperty(PROPERTY_GRID*, HPROPERTY);
BOOL PropertyGridAddProperty(PROPERTY_GRID*, HPROPERTY);
BOOL PropertyGridAddSubItem(PROPERTY_GRID*, HPROPERTY, HPROPERTY);
void PropertyGridSetVerifier(HPROPERTY, PROCVERIFY, BOOL);
void PropertyGridDeleteAll(PROPERTY_GRID*);
void PropertyGridCancelSelection(PROPERTY_GRID*, BOOL);
void PropertyGridSetSelection(PROPERTY_GRID*, HPROPERTY, BOOL);
void PropertyGridDisableProperty(PROPERTY_GRID*, HPROPERTY, BOOL);
HPROPERTY PropertyGridFindProperty(PROPERTY_GRID*, LPCWSTR);
HPROPERTY PropertyGridFindNextProperty(PROPERTY_GRID*, HPROPERTY);

// Item
PROPERTY_ITEM* PropertyGridItemGetFirstVisible(PROPERTY_GRID*, int, int);
PROPERTY_ITEM* PropertyGridItemGetNextVisible(PROPERTY_GRID*, PROPERTY_ITEM*, int, int);
LPWSTR __stdcall PropertyGridItemDefaultVerifier(void*, size_t);
void PropertyGridItemVerify(PROPERTY_ITEM*, void*, size_t);
void PropertyGridItemGetIndentionRect(PROPERTY_GRID*, PROPERTY_ITEM*, LPRECT);
void PropertyGridItemGetKeyRect(PROPERTY_GRID*, PROPERTY_ITEM*, LPRECT);
void PropertyGridItemGetValueRect(PROPERTY_GRID*, PROPERTY_ITEM*, LPRECT);
void PropertyGridItemGetStateImageRect(PROPERTY_GRID*, PROPERTY_ITEM*, LPRECT);
BOOL PropertyGridItemIsValid(PROPERTY_GRID*, PROPERTY_ITEM*, PROPERTY_ITEM**);
int PropertyGridItemHitTest(PROPERTY_GRID*, PROPERTY_ITEM*, POINT);

// Message
void OnPaint(PROPERTY_GRID*, HDC, LPRECT, BOOL);
void OnSize(PROPERTY_GRID*, LONG, LONG);
void OnScroll(PROPERTY_GRID*, int, int);
void OnMouseMove(PROPERTY_GRID*, WPARAM, int, int);
void OnMouseLeftButtonPress(PROPERTY_GRID*, int, int);

// Main
void InitPropertyGridView(PROPERTY_GRID*, HWND, HINSTANCE);
void ReleasePropertyGridView(PROPERTY_GRID*);
int PropertyGridAddImage(HIMAGELIST, HBITMAP, COLORREF);
LONG PropertyGridGetScrollHeight(PROPERTY_GRID*);
void PropertyGridUpdateScrollRange(PROPERTY_GRID*);
void PropertyGridUpdateValueEditWindow(PROPERTY_GRID*);

// Subclass
LRESULT CALLBACK ValueEditSubclassProc(HWND, UINT, WPARAM, LPARAM);
