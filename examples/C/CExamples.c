#include "CCollections/CList.h"

CLIST_FORWARD_DECLARE(int);
CLIST_DECLARE(int);

void clistRemove(CList_int* pList, int* pItem) {
    auto data = (pList)->Data;
    auto dataEnd = (pList)->Data + (pList)->Count;

    uint32_t i = 0;
    while (data != dataEnd)
    {
        if (memcmp(pList, pItem, sizeof(*(pList))) == 0)
            break;
        ++data;
        ++i;
    }

    clistRemoveAt(pList, i);
}

void ttest()
{

    CList_int listOfInt;

    CList(int) listInt;
    CList(int)* pList = &listInt;
    uint32_t capacity = 32;

    clistSetCapacity(pList, 32);
    clistNew(&listInt, 32);
    clistDelete(&listInt);

    listInt = (const struct CList_int_t){ 0 };

    //*pList = { 0 };

}



int main()
{

    return 0;
}