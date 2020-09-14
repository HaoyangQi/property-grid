/** Property Grid Message Handlers
 *
 **/

#include "PropertyGridView.h"

void OnPaint(PROPERTY_GRID* ppg, HDC hdc, LPRECT rcUpdate, BOOL bErase)
{
    SetBkMode(hdc, TRANSPARENT);

    PROPERTY_ITEM* prop = PropertyGridItemGetFirstVisible(ppg, rcUpdate->top, rcUpdate->bottom);
    RECT rc;
    HFONT font = (HFONT)GetCurrentObject(hdc, OBJ_FONT);

    while (prop)
    {
        if (isCategory(prop))
        {
            SelectObject(hdc, ppg->fontBold);

            CopyRect(&rc, &prop->rcItem);
            FillRect(hdc, &rc, ppg->brItemFrame);

            rc.left = (prop->nLevel + 1) * ppg->dmItemHeight;
            SetTextColor(hdc, prop->clrKey);
            DrawTextW(hdc, prop->strKey, wcslen(prop->strKey), &rc, DT_LEFT | DT_SINGLELINE | DT_VCENTER);

            ImageList_Draw(ppg->hStateImageList,
                prop->bCollapse ? 0 : 1,
                hdc,
                rc.left - ppg->dmItemHeight, rc.top,
                ILD_TRANSPARENT);
        }
        else
        {
            COLORREF colorText = prop->clrKey;
            SelectObject(hdc, ppg->fontRegular);

            // Indention region
            PropertyGridItemGetIndentionRect(ppg, prop, &rc);
            FillRect(hdc, &rc, ppg->brItemFrame);

            // Key name region
            PropertyGridItemGetKeyRect(ppg, prop, &rc);
            if (prop->bSelect)
            {
                colorText = ppg->clrTextDefault; // if selected, make text color stable
                HBRUSH br = CreateSolidBrush(ppg->clrSelect);
                FillRect(hdc, &rc, br);
                DeleteObject(br);
            }
            SetTextColor(hdc, colorText);
            FrameRect(hdc, &rc, ppg->brItemFrame);
            rc.left += 5;
            DrawTextW(hdc, prop->strKey, wcslen(prop->strKey), &rc, DT_LEFT | DT_SINGLELINE | DT_VCENTER);

            // Value name region
            PropertyGridItemGetValueRect(ppg, prop, &rc);
            if (prop->bSelect)
            {
                ; // TODO
            }
            SetTextColor(hdc, prop->clrVal);
            FrameRect(hdc, &rc, ppg->brItemFrame);
            rc.left += 5;
            DrawTextW(hdc, prop->strValue, wcslen(prop->strValue), &rc, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
        }
        
        prop = PropertyGridItemGetNextVisible(ppg, prop, rcUpdate->top, rcUpdate->bottom);
    }

    SelectObject(hdc, font);
}

void OnSize(PROPERTY_GRID* ppg, LONG w, LONG h)
{
    // Update size and scroll info
    ppg->szControl.cx = w;
    ppg->szControl.cy = h;
    PropertyGridUpdateScrollRange(ppg);

    // Update item rect
    PROPERTY_ITEM* prop = ppg->content;
    while (prop)
    {
        prop->rcItem.right = ppg->szControl.cx;
        prop = prop->next;
    }

    // scroll pos might be changed due to the size change, 
    // in this case, content needs to be scrolled accordingly
    if (ppg->content)
    {
        SCROLLINFO si;
        si.cbSize = sizeof(SCROLLINFO);
        si.fMask = SIF_ALL;
        GetScrollInfo(ppg->hwnd, SB_VERT, &si);

        LONG dy = abs(ppg->content->rcItem.top) - si.nPos;
        PROPERTY_ITEM* prop = ppg->content;

        while (prop)
        {
            OffsetRect(&prop->rcItem, 0, dy);
            prop = prop->next;
        }

        ScrollWindowEx(ppg->hwnd, 0, dy, NULL, NULL, NULL, NULL, SW_ERASE | SW_INVALIDATE);
    }

    // Scroll horizontally to preserve aspect ratio of divider
    LONG dx = ppg->szControl.cx * ppg->aspectRatioDivider - ppg->posDivider;
    // TODO: fix divider position if scrolling is out of tolerance
    /*if (0 || ppg->posDivider + dx > ppg->szControl.cx - ppg->dmItemHeight)
    {
        ppg->aspectRatioDivider = (float)ppg->posDivider / ppg->szControl.cx;
        dx = ppg->szControl.cx * ppg->aspectRatioDivider - ppg->posDivider;
    }*/
    ppg->posDivider = ppg->szControl.cx * ppg->aspectRatioDivider;
    RECT rc = { ppg->posDivider - dx, 0, ppg->szControl.cx, ppg->szControl.cy };
    ScrollWindowEx(ppg->hwnd, dx, 0, &rc, &rc, NULL, NULL, SW_ERASE | SW_INVALIDATE);

    // Finally, update edit window
    PropertyGridUpdateValueEditWindow(ppg);
}

void OnScroll(PROPERTY_GRID* ppg, int scrollType, int lineStep)
{
    SCROLLINFO si;
    LONG posOld = 0;

    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_ALL;
    GetScrollInfo(ppg->hwnd, SB_VERT, &si);
    posOld = si.nPos;

    switch (scrollType)
    {
        case SB_BOTTOM:
            si.nPos = si.nMax;
            break;
        case SB_LINEDOWN:
            si.nPos = posOld + lineStep;
            break;
        case SB_LINEUP:
            si.nPos = posOld - lineStep;
            break;
        case SB_PAGEDOWN:
            si.nPos = posOld + si.nPage;
            break;
        case SB_PAGEUP:
            si.nPos = posOld - si.nPage;
            break;
        case SB_THUMBTRACK:
            si.nPos = si.nTrackPos;
            break;
        case SB_TOP:
            si.nPos = si.nMin;
            break;
        case SB_THUMBPOSITION:
        default:
            break;
    }

    // update data
    si.fMask = SIF_POS;
    SetScrollInfo(ppg->hwnd, SB_VERT, &si, TRUE);

    LONG dy = posOld - si.nPos;
    PROPERTY_ITEM* prop = ppg->content;
    while (prop)
    {
        OffsetRect(&prop->rcItem, 0, dy);
        prop = prop->next;
    }

    // Scroll
    ScrollWindowEx(ppg->hwnd, 0, dy, NULL, NULL, NULL, NULL, SW_ERASE | SW_INVALIDATE | SW_SCROLLCHILDREN);
}

void OnMouseMove(PROPERTY_GRID* ppg, WPARAM virtualKey, int x, int y)
{
    POINT pt = { x, y };
    PROPERTY_ITEM* property = PropertyGridItemGetFirstVisible(ppg, 0, ppg->szControl.cy);
    int hit = PROP_HIT_NOTHING;
    LONG dx = x - ppg->ptDragPrevX;
    LONG posOld = ppg->posDivider;

    // Hit test iteration
    while (property)
    {
        hit = PropertyGridItemHitTest(ppg, property, pt);

        if (hit != PROP_HIT_NOTHING)
        {
            break;
        }

        property = PropertyGridItemGetNextVisible(ppg, property, 0, ppg->szControl.cy);
    }

    // Operations
    switch (hit)
    {
        case PROP_HIT_DIVIDER:
        {
            SetCursor(LoadCursor(NULL, IDC_SIZEWE));
            break;
        }
        default:
        {
            //SetCursor(LoadCursor(NULL, IDC_ARROW));
            break;
        }
    }

    if (ppg->bDragging && 
        posOld + dx >= ppg->dmItemHeight * 2 && // TODO: not good enough
        posOld + dx <= ppg->szControl.cx - ppg->dmItemHeight)
    {
        // Since paint procedure depends on divider position, 
        // so must update, then scroll
        ppg->posDivider += dx;
        ppg->aspectRatioDivider = (float)ppg->posDivider / ppg->szControl.cx;
        ppg->ptDragPrevX = x;

        // Scroll iteration (item-wise, category excluded)
        property = PropertyGridItemGetFirstVisible(ppg, 0, ppg->szControl.cy);
        while (property)
        {
            if (!isCategory(property))
            {
                RECT rc;
                CopyRect(&rc, &property->rcItem);

                // scroll value field content (left and right border must be excluded)
                rc.left = posOld + 1; // avoid capturing divider
                rc.right -= 1; // avoid capturing & dragging right border line
                ScrollWindowEx(ppg->hwnd, dx, 0, &rc, &rc, NULL, NULL, SW_ERASE | SW_INVALIDATE);
                
                // redraw divider region
                rc.left = min(posOld, ppg->posDivider) - 1;
                rc.right = max(posOld, ppg->posDivider) + 1;
                InvalidateRect(ppg->hwnd, &rc, TRUE);
            }

            property = PropertyGridItemGetNextVisible(ppg, property, 0, ppg->szControl.cy);
        }

        // Method 2: Scroll everything then only scroll category content 
        // back to its original position. In general, loop is much cheaper.
        // However, the fliker is more obvious than the method above
        /*RECT rc;

        // scroll value field content (left and right border must be excluded)
        rc.left = posOld + 1; // avoid capturing divider
        rc.right = ppg->szControl.cx - 1; // avoid capturing & dragging right border line
        rc.top = 0;
        rc.bottom = ppg->szControl.cy;
        ScrollWindowEx(ppg->hwnd, dx, 0, &rc, &rc, NULL, NULL, SW_ERASE | SW_INVALIDATE);

        // redraw divider region
        rc.left = min(posOld, ppg->posDivider) - 1;
        rc.right = max(posOld, ppg->posDivider) + 1;
        InvalidateRect(ppg->hwnd, &rc, TRUE);

        // Scroll categories back to cancel out scroll effect on them
        property = PropertyGridItemGetFirstVisible(ppg, 0, ppg->szControl.cy);
        while (property)
        {
            if (isCategory(property))
            {
                RECT rc;
                CopyRect(&rc, &property->rcItem);

                rc.left = ppg->posDivider + 1;
                rc.right -= 1;
                ScrollWindowEx(ppg->hwnd, -dx, 0, &rc, &rc, NULL, NULL, SW_INVALIDATE);

                // redraw divider region
                rc.left = min(posOld, ppg->posDivider);
                rc.right = max(posOld, ppg->posDivider);
                InvalidateRect(ppg->hwnd, &rc, FALSE);
            }

            property = PropertyGridItemGetNextVisible(ppg, property, 0, ppg->szControl.cy);
        }*/
    }
}

void OnMouseLeftButtonPress(PROPERTY_GRID* ppg, int x, int y)
{
    POINT pt = { x, y };
    PROPERTY_ITEM* property = PropertyGridItemGetFirstVisible(ppg, 0, ppg->szControl.cy);
    PROPERTY_ITEM* current;
    RECT rc;
    LONG subHeight = 0;
    int hit = PROP_HIT_NOTHING;

    // Hit test iteration
    while (property)
    {
        hit = PropertyGridItemHitTest(ppg, property, pt);

        if (hit != PROP_HIT_NOTHING)
        {
            break;
        }

        property = PropertyGridItemGetNextVisible(ppg, property, 0, ppg->szControl.cy);
    }

    // Operations
    switch (hit)
    {
        case PROP_HIT_STATEICON:
        {
            // Toggle state image
            property->bCollapse ^= 1;
            PropertyGridItemGetStateImageRect(ppg, property, &rc);
            InvalidateRect(ppg->hwnd, &rc, TRUE);

            // Measure total height of sub-items and set visibility
            current = property->next;
            while (current && current->nLevel > property->nLevel)
            {
                // if parent is visible and expanding its children, then it is visible
                current->bVisible = current->parent->bVisible && !current->parent->bCollapse;

                // if selection is collapsed, cancel selection and commit changes
                if (property->bCollapse && current->bSelect)
                {
                    PropertyGridCancelSelection(ppg, TRUE);
                }

                // visibility is not stable in this context, so calculate using collapse state
                if (current->parent == property)
                {
                    subHeight += ppg->dmItemHeight;
                }
                else
                {
                    subHeight += current->parent->bCollapse ? 0 : ppg->dmItemHeight;
                }

                current = current->next;
            }

            // Prepare information
            SetRect(&rc, 0, property->rcItem.bottom, ppg->szControl.cx, ppg->szControl.cy);
            subHeight = property->bCollapse ? -subHeight : subHeight;

            // Update item rect
            while (current)
            {
                OffsetRect(&current->rcItem, 0, subHeight);
                current = current->next;
            }

            // Scroll contents
            ScrollWindowEx(ppg->hwnd, 0, subHeight, &rc, &rc, NULL, NULL, SW_ERASE | SW_INVALIDATE);
            PropertyGridUpdateScrollRange(ppg);
            
            break;
        }
        case PROP_HIT_KEY:
        {
            DebugPrintf(L"Hit name field: %s, %s.\n", property->strKey, property->strValue);
            PropertyGridSetSelection(ppg, property, TRUE);
            break;
        }
        case PROP_HIT_VALUE:
        {
            DebugPrintf(L"Hit value field.\n");
            PropertyGridSetSelection(ppg, property, TRUE);
            break;
        }
        case PROP_HIT_DIVIDER:
        {
            DebugPrintf(L"Hit divider region.\n");
            SetCursor(LoadCursor(NULL, IDC_SIZEWE));
            ppg->ptDragPrevX = x;
            ppg->bDragging = TRUE;
            break;
        }
        case PROP_HIT_CATEGORY:
        case PROP_HIT_ITEM:
        default:
        {
            // Some tests are reserved
            // TODO: for now, we drop changes in this case
            PropertyGridCancelSelection(ppg, FALSE);
            break;
        }
    }
}
