
#include "CCollections/CList.h"


int main(void)
{
    assert(sizeof(CList) == 32);

    CList a = {0};
    CList* p = &a;
    clistAlloc(p, sizeof(int), 32);
    clistRealloc(p, 64);

    int two = 2;
    uint32_t i = 0xFFFFFFFF;
    clistAdd(p, &two);
    i = clistFindIndex(p, &two);
    assert(i == 0);
    assert(two == 2);
    assert(*(int*)clistItemAt(p, 0) == two);
    clistFree(p);

    return 0;
}