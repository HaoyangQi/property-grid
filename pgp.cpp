/** Property Grid Public Interfaces
 *
 **/

#include "PropertyGridView.h"

BOOL isCategory(HPROPERTY hProperty)
{
    PROPERTY_ITEM* property = (PROPERTY_ITEM*)hProperty;
    return property && property->strValue == NULL;
}

// If *value* is NULL, then it is a category
HPROPERTY PropertyGridNewProperty(PROPERTY_GRID* ppg, LPCWSTR key, LPCWSTR value)
{
    PROPERTY_ITEM* property = (PROPERTY_ITEM*)malloc(sizeof(PROPERTY_ITEM));

    if (property)
    {
        property->strKey = _wcsdup(key);
        property->strValueOriginal = _wcsdup(value);
        property->strValue = _wcsdup(value);
        property->strDescription = NULL;
        property->clrKey = ppg->clrTextDefault;
        property->clrVal = ppg->clrTextDefault;
        property->nLevel = 0;
        property->rcItem = { 0, 0, 0, 0 };
        property->bCollapse = FALSE;
        property->bSelect = FALSE;
        property->bDisable = FALSE;
        property->bAllowEdit = TRUE;
        property->bVisible = TRUE;
        property->lpfnVerifyProc = PropertyGridItemDefaultVerifier;
        property->data = _wcsdup(value);
        property->szData = value ? sizeof(WCHAR) * (wcslen(value) + 1) : 0;
        property->next = NULL;
        property->parent = NULL;
    }

    return property;
}

PROPERTY_ITEM* PropertyGridReleaseProperty(PROPERTY_GRID* ppg, HPROPERTY hProperty)
{
    PROPERTY_ITEM* property = (PROPERTY_ITEM*)hProperty;
    PROPERTY_ITEM* next = property->next;

    if (property->bSelect)
    {
        PropertyGridCancelSelection(ppg, FALSE);
    }

    if (property)
    {
        free(property->strKey);
        free(property->strValue);
        free(property->strValueOriginal);
        free(property->strDescription);
        free(property->data);
        free(property);
    }

    return next;
}

void PropertyGridDeleteProperty(PROPERTY_GRID* ppg, HPROPERTY hProperty)
{
    PROPERTY_ITEM* property = (PROPERTY_ITEM*)hProperty;
    PROPERTY_ITEM* previous;
    PROPERTY_ITEM* cur;
    int deleteHeight = 0;

    if (property)
    {
        LONG level = property->nLevel;
        BOOL bRepaint = PropertyGridItemIsValid(ppg, property, &previous);

        if (bRepaint)
        {
            // Delete category content
            cur = property->next;
            while (cur && cur->nLevel > level)
            {
                deleteHeight += cur->bVisible ? ppg->dmItemHeight : 0;
                cur = PropertyGridReleaseProperty(ppg, cur);
            }
            
            // Detach item
            if (previous)
            {
                previous->next = cur;
            }
            else
            {
                ppg->content = cur;
            }

            PropertyGridReleaseProperty(ppg, property);
            deleteHeight += property->bVisible ? ppg->dmItemHeight : 0;

            // Update scroll info, redraw
            if (cur)
            {
                RECT rc = { 0, cur->rcItem.top, ppg->szControl.cx, ppg->szControl.cy };
                while (cur)
                {
                    OffsetRect(&cur->rcItem, 0, -deleteHeight);
                    cur = cur->next;
                }
                PropertyGridUpdateScrollRange(ppg);
                ScrollWindowEx(ppg->hwnd, 0, ppg->dmItemHeight, &rc, &rc, NULL, NULL, SW_ERASE | SW_INVALIDATE);
            }
        }
    }
}

BOOL PropertyGridAddProperty(PROPERTY_GRID* ppg, HPROPERTY hProperty)
{
    PROPERTY_ITEM* current = ppg->content;
    PROPERTY_ITEM* property = (PROPERTY_ITEM*)hProperty;
    PROPERTY_ITEM* previous;

    if (PropertyGridItemIsValid(ppg, property, &previous))
    {
        return FALSE;
    }

    if (!current)
    {
        ppg->content = property;
        SetRect(&property->rcItem, 0, 0, ppg->szControl.cx, ppg->dmItemHeight);
    }
    else
    {
        previous->next = property;
        CopyRect(&property->rcItem, &previous->rcItem);
        OffsetRect(&property->rcItem, 0, ppg->dmItemHeight);
    }

    // Update scroll info
    PropertyGridUpdateScrollRange(ppg);

    // Redraw this item
    InvalidateRect(ppg->hwnd, &property->rcItem, TRUE);

    return TRUE;
}

BOOL PropertyGridAddSubItem(PROPERTY_GRID* ppg, HPROPERTY hParent, HPROPERTY hProperty)
{
    PROPERTY_ITEM* parent = (PROPERTY_ITEM*)hParent;
    PROPERTY_ITEM* property = (PROPERTY_ITEM*)hProperty;
    LONG levelParent = parent->nLevel;

    if (!isCategory(parent) || !property || !parent || 
        PropertyGridItemIsValid(ppg, property, NULL) || !PropertyGridItemIsValid(ppg, parent, NULL))
    {
        return FALSE;
    }

    property->nLevel = levelParent + 1;
    property->parent = parent;
    property->bVisible = TRUE;

    // Test visibility
    while (parent)
    {
        if (!parent->bVisible || parent->bCollapse)
        {
            property->bVisible = FALSE;
            break;
        }
        parent = parent->parent;
    }

    // Add property
    parent = (PROPERTY_ITEM*)hParent;
    while (parent && parent->next && parent->next->nLevel > levelParent)
    {
        parent = parent->next;
    }
    property->next = parent->next;
    parent->next = property;

    // Adjust item rect
    CopyRect(&property->rcItem, &parent->rcItem);
    while (property)
    {
        OffsetRect(&property->rcItem, 0, ppg->dmItemHeight);
        property = property->next;
    }

    // Update scroll info, scroll content, and redraw
    property = (PROPERTY_ITEM*)hProperty;
    if (property->bVisible)
    {
        RECT rc = { 0, property->rcItem.top, ppg->szControl.cx, ppg->szControl.cy };

        PropertyGridUpdateScrollRange(ppg);
        ScrollWindowEx(ppg->hwnd, 0, ppg->dmItemHeight, &rc, &rc, NULL, NULL, SW_ERASE | SW_INVALIDATE);
    }

    return TRUE;
}

void PropertyGridAddOption(PROPERTY_GRID* ppg)
{
    // TODO
}

void PropertyGridAddColorProperty(PROPERTY_GRID* ppg)
{
    // TODO
}

void PropertyGridAddFontProperty(PROPERTY_GRID* ppg)
{
    // TODO
}

void PropertyGridSetVerifier(HPROPERTY hProperty, PROCVERIFY verifier, BOOL bValidate)
{
    PROPERTY_ITEM* property = (PROPERTY_ITEM*)hProperty;

    if (verifier)
    {
        property->lpfnVerifyProc = verifier;
    }
    else
    {
        property->lpfnVerifyProc = PropertyGridItemDefaultVerifier;
    }

    if (bValidate)
    {
        PropertyGridItemVerify(property, property->data, property->szData);
    }
}

void PropertyGridDeleteAll(PROPERTY_GRID* ppg)
{
    PROPERTY_ITEM* cur = ppg->content;

    while (cur)
    {
        cur = PropertyGridReleaseProperty(ppg, cur);
    }

    ppg->content = NULL;
}

void PropertyGridCancelSelection(PROPERTY_GRID* ppg, BOOL bVerify)
{
    RECT rc;

    if (ppg->itemSelect)
    {
        // Cancel selection
        ppg->itemSelect->bSelect = FALSE;
        PropertyGridItemGetKeyRect(ppg, ppg->itemSelect, &rc);
        InvalidateRect(ppg->hwnd, &rc, TRUE);
        //PropertyGridItemGetValueRect(ppg, ppg->itemSelect, &rc);
        //InvalidateRect(ppg->hwnd, &rc, TRUE);

        // Complete edit process and commit changes
        if (ppg->hwndValueEdit)
        {
            if (bVerify)
            {
                // verify data
                int len = GetWindowTextLength(ppg->hwndValueEdit) + 1;
                WCHAR* buf = (WCHAR*)malloc(len * sizeof(WCHAR));

                if (buf)
                {
                    len = GetWindowText(ppg->hwndValueEdit, buf, len);
                    buf[len] = L'\0';
                    PropertyGridItemVerify(ppg->itemSelect, buf, len * sizeof(WCHAR));
                    free(buf);
                }
            }

            if (ppg->procOldEditControl)
            {
                SetWindowLongPtr(ppg->hwndValueEdit, GWLP_WNDPROC, (LONG_PTR)(ppg->procOldEditControl));
                ppg->procOldEditControl = NULL;
            }
            DestroyWindow(ppg->hwndValueEdit);
        }

        ppg->itemSelect = NULL;
    }
}

void PropertyGridSetSelection(PROPERTY_GRID* ppg, HPROPERTY hProperty, BOOL bVerify)
{
    PROPERTY_ITEM* property = (PROPERTY_ITEM*)hProperty;
    RECT rc;

    // Cancel previous selection
    if (property != ppg->itemSelect)
    {
        PropertyGridCancelSelection(ppg, bVerify);
    }

    if (property && property != ppg->itemSelect && property->bVisible)
    {
        ppg->itemSelect = property;

        // Highlight new selection
        property->bSelect = TRUE;
        PropertyGridItemGetKeyRect(ppg, property, &rc);
        InvalidateRect(ppg->hwnd, &rc, TRUE);
        PropertyGridItemGetValueRect(ppg, property, &rc);
        // TODO: as of current, value field is not necessarily redrawn
        //InvalidateRect(ppg->hwnd, &rc, TRUE);

        // Show value edit field
        // TODO: now only support edit control
        InflateRect(&rc, -1, -1);
        ppg->hwndValueEdit = CreateWindow(WC_EDIT, NULL, 
            WS_CHILD | WS_VISIBLE | ES_LEFT | (property->bAllowEdit ? 0 : ES_READONLY),
            rc.left + VALUE_EDIT_TEXT_OFFSET, rc.top, rc.right - rc.left, rc.bottom - rc.top,
            ppg->hwnd, NULL, ppg->appInstance, NULL);
        // TODO: bold modified text
        SendMessage(ppg->hwndValueEdit, WM_SETFONT, (WPARAM)(ppg->fontRegular), (LPARAM)0);
        SetWindowText(ppg->hwndValueEdit, property->strValue);
        SendMessage(ppg->hwndValueEdit, EM_SETSEL, 0, -1);
        ppg->procOldEditControl = (WNDPROC)SetWindowLongPtr(ppg->hwndValueEdit, GWLP_WNDPROC, (LONG_PTR)ValueEditSubclassProc);
        SetFocus(ppg->hwndValueEdit);
    }
}

void PropertyGridDisableProperty(PROPERTY_GRID* ppg, HPROPERTY hProperty, BOOL bDisbale)
{
    PROPERTY_ITEM* property = (PROPERTY_ITEM*)hProperty;

    if (property && !isCategory(property) && property->bDisable != bDisbale)
    {
        property->bDisable = bDisbale;
        property->bAllowEdit = !bDisbale;
        property->clrKey = ppg->clrTextDisable;
        property->clrVal = ppg->clrTextDisable;
        InvalidateRect(ppg->hwnd, &property->rcItem, TRUE);
    }
}

void PropertyGridSetEditable(PROPERTY_GRID* ppg, HPROPERTY hProperty, BOOL bEditable)
{
    PROPERTY_ITEM* property = (PROPERTY_ITEM*)hProperty;

    if (property && !isCategory(property) && property->bAllowEdit != bEditable)
    {
        property->bAllowEdit = !bEditable;
    }

    // If setting selected item, cancel the selection and drop changes
    if (property == ppg->itemSelect)
    {
        PropertyGridCancelSelection(ppg, FALSE);
    }
}

HPROPERTY PropertyGridFindProperty(PROPERTY_GRID* ppg, LPCWSTR key)
{
    PROPERTY_ITEM* property = NULL;

    if (key)
    {
        property = ppg->content;

        while (property)
        {
            if (property->strKey && wcscmp(property->strKey, key) == 0)
            {
                break;
            }
            property = property->next;
        }
    }

    return property;
}

HPROPERTY PropertyGridFindNextProperty(PROPERTY_GRID* ppg, HPROPERTY hFrom)
{
    PROPERTY_ITEM* property = (PROPERTY_ITEM*)hFrom;
    
    if (property)
    {
        LPWSTR key = property->strKey;
        property = property->next;

        while (property)
        {
            if (property->strKey && wcscmp(property->strKey, key) == 0)
            {
                break;
            }
            property = property->next;
        }
    }

    return property;
}

BOOL PropertyGridSetValue(HPROPERTY hProperty, LPCWSTR val)
{
    PROPERTY_ITEM* property = (PROPERTY_ITEM*)hProperty;

    if (!property || isCategory(property))
    {
        return FALSE;
    }

    free(property->strValue);
    property->strValue = _wcsdup(val);

    return TRUE;
}

size_t PropertyGridGetStagedDataSize(HPROPERTY hProperty)
{
    return hProperty ? ((PROPERTY_ITEM*)hProperty)->szData : 0;
}

BOOL PropertyGridGetStagedData(HPROPERTY hProperty, size_t c, void* object)
{
    PROPERTY_ITEM* property = (PROPERTY_ITEM*)hProperty;

    if (!property || c > property->szData || !object)
    {
        return FALSE;
    }

    memcpy(object, property->data, c);
    return TRUE;
}
