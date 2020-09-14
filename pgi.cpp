/** Property Grid Item Operations
 *
 **/

#include "PropertyGridView.h"

PROPERTY_ITEM* PropertyGridItemGetFirstVisible(PROPERTY_GRID* ppg, int startY, int endY)
{
    PROPERTY_ITEM* pCur = ppg->content;

    while (pCur)
    {
        if (PropertyGridItemIsVisibleWithin(pCur, startY, endY))
        {
            break;
        }
        pCur = pCur->next;
    }

    return pCur;
}

PROPERTY_ITEM* PropertyGridItemGetNextVisible(PROPERTY_GRID* ppg, PROPERTY_ITEM* current, int startY, int endY)
{
    if (!PropertyGridItemIsVisibleWithin(current, startY, endY))
    {
        return NULL;
    }

    current = current->next;

    while (current)
    {
        if (PropertyGridItemIsVisibleWithin(current, startY, endY))
        {
            break;
        }
        current = current->next;
    }

    return current;
}

LPWSTR __stdcall PropertyGridItemDefaultVerifier(void* data, size_t size)
{
    WCHAR* valueFormatted = NULL;

    if (data)
    {
        valueFormatted = (WCHAR*)data;

        if (valueFormatted[size / sizeof(WCHAR) - 1] != '\0')
        {
            size += sizeof(WCHAR);
        }

        valueFormatted = (WCHAR*)malloc(size);
        if (valueFormatted)
        {
            memset(valueFormatted, 0, size);
            memcpy(valueFormatted, data, size);
        }
    }
    
    return valueFormatted;
}

void PropertyGridItemVerify(PROPERTY_ITEM* property, void* data, size_t size)
{
    LPWSTR value = property->lpfnVerifyProc(data, size);
    if (value)
    {
        free(property->strValue);
        free(property->data);

        property->strValue = value;
        property->data = malloc(size);
        if (property->data)
        {
            memcpy(property->data, data, size);
        }
    }
}

void PropertyGridItemGetIndentionRect(PROPERTY_GRID* ppg, PROPERTY_ITEM* property, LPRECT prc)
{
    CopyRect(prc, &property->rcItem);
    prc->right = property->nLevel * ppg->dmItemHeight;
}

void PropertyGridItemGetKeyRect(PROPERTY_GRID* ppg, PROPERTY_ITEM* property, LPRECT prc)
{
    CopyRect(prc, &property->rcItem);
    prc->left = property->nLevel * ppg->dmItemHeight;
    prc->right = ppg->posDivider;
}

void PropertyGridItemGetValueRect(PROPERTY_GRID* ppg, PROPERTY_ITEM* property, LPRECT prc)
{
    CopyRect(prc, &property->rcItem);
    prc->left = ppg->posDivider;
}

void PropertyGridItemGetStateImageRect(PROPERTY_GRID* ppg, PROPERTY_ITEM* property, LPRECT prc)
{
    PropertyGridItemGetIndentionRect(ppg, property, prc);
    prc->left = prc->right;
    prc->right = prc->left + ppg->dmItemHeight;
}

BOOL PropertyGridItemIsValid(PROPERTY_GRID* ppg, PROPERTY_ITEM* property, PROPERTY_ITEM** previous)
{
    PROPERTY_ITEM* current = ppg->content;

    if (previous)
    {
        *previous = NULL;
    }

    while (current)
    {
        if (current == property)
        {
            return TRUE;
        }

        if (previous)
        {
            *previous = current;
        }
        current = current->next;
    }

    return FALSE;
}

PROPERTY_ITEM* PropertyGridItemGetPrevious(PROPERTY_GRID* ppg, PROPERTY_ITEM* property)
{
    PROPERTY_ITEM* current = ppg->content;
    PROPERTY_ITEM* prev = NULL;

    while (current)
    {
        if (current == property)
        {
            return prev;
        }

        prev = current;
        current = current->next;
    }

    return NULL;
}

int PropertyGridItemHitTest(PROPERTY_GRID* ppg, PROPERTY_ITEM* property, POINT pt)
{
    RECT rcDivider = { ppg->posDivider - 5, 0, ppg->posDivider + 5, ppg->szControl.cy };

    if (property && property->bVisible && PtInRect(&property->rcItem, pt))
    {
        if (isCategory(property))
        {
            RECT rc;
            PropertyGridItemGetStateImageRect(ppg, property, &rc);
            return PtInRect(&rc, pt) ? PROP_HIT_STATEICON : PROP_HIT_CATEGORY;
        }
        else if (PtInRect(&rcDivider, pt))
        {
            return PROP_HIT_DIVIDER;
        }
        else
        {
            RECT rc1, rc2;
            PropertyGridItemGetKeyRect(ppg, property, &rc1);
            PropertyGridItemGetValueRect(ppg, property, &rc2);

            if (PtInRect(&rc1, pt))
            {
                return PROP_HIT_KEY;
            }
            else if (PtInRect(&rc2, pt))
            {
                return PROP_HIT_VALUE;
            }
            else
            {
                return PROP_HIT_ITEM;
            }
        }
    }

    return PROP_HIT_NOTHING;
}
