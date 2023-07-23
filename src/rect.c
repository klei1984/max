/* Copyright (c) 2020 M.A.X. Port Team
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "rect.h"

#include "gnw.h"

static RectPtr rlist;

void GNW_rect_exit(void) {
    while (rlist) {
        RectPtr ptr = rlist->next;
        free(rlist);
        rlist = ptr;
    }
}

void rect_clip_list(RectPtr *pCur, Rect *bound) {
    RectPtr *clip;
    RectPtr cur;
    RectPtr temp;
    RectPtr temp2;
    RectPtr temp3;

    temp = NULL;
    clip = &temp;
    if (*pCur) {
        do {
            cur = (*pCur)->next;
            *clip = rect_clip(&(*pCur)->r, bound);
            if (*clip) {
                do {
                    temp2 = (*clip)->next;
                    clip = &(*clip)->next;
                } while (temp2);
            }
            temp3 = rlist;
            rlist = (*pCur);
            (*pCur)->next = temp3;
            (*pCur) = cur;
        } while (cur);
    }
    *pCur = temp;
}

RectPtr rect_clip(Rect *b, Rect *t) {
    Rect rect[4];
    Rect clipped;
    RectPtr ptr;

    ptr = NULL;
    if (rect_inside_bound(t, b, &clipped)) {
        ptr = rect_malloc();
        if (ptr) {
            ptr->r = *b;
            ptr->next = NULL;
        }
    } else {
        rect[0].ulx = b->ulx;
        rect[0].uly = b->uly;
        rect[0].lrx = b->lrx;
        rect[0].lry = clipped.uly - 1;

        rect[1].ulx = b->ulx;
        rect[1].uly = clipped.uly;
        rect[1].lrx = clipped.ulx - 1;
        rect[1].lry = clipped.lry;

        rect[2].ulx = clipped.lrx + 1;
        rect[2].uly = clipped.uly;
        rect[2].lrx = b->lrx;
        rect[2].lry = clipped.lry;

        rect[3].ulx = b->ulx;
        rect[3].uly = clipped.lry + 1;
        rect[3].lrx = b->lrx;
        rect[3].lry = b->lry;

        {
            RectPtr *rect_ptr;
            rect_ptr = &ptr;
            int32_t i = 0;

            do {
                if (rect[i].ulx <= rect[i].lrx && rect[i].uly <= rect[i].lry) {
                    *rect_ptr = rect_malloc();
                    if (*rect_ptr == NULL) {
                        return NULL;
                    }

                    (*rect_ptr)->r.ulx = rect[i].ulx;
                    (*rect_ptr)->r.uly = rect[i].uly;
                    (*rect_ptr)->r.lrx = rect[i].lrx;
                    (*rect_ptr)->r.lry = rect[i].lry;

                    (*rect_ptr)->next = NULL;
                    rect_ptr = &(*rect_ptr)->next;
                }
                ++i;
            } while (i < 4);
        }
    }

    return ptr;
}

RectPtr rect_malloc(void) {
    RectPtr ptr;

    if (!rlist) {
        int32_t i = 0;

        do {
            ptr = (RectPtr)malloc(sizeof(struct rectdata));
            if (!ptr) {
                break;
            } else {
                RectPtr temp;

                temp = rlist;
                rlist = ptr;
                ptr->next = temp;
                i++;
            }
        } while (i < 10);
    }

    if (rlist) {
        ptr = rlist;
        rlist = rlist->next;
    } else {
        ptr = 0;
    }

    return ptr;
}

void rect_free(RectPtr ptr) {
    RectPtr temp;

    temp = rlist;
    rlist = ptr;
    ptr->next = temp;
}

void rect_min_bound(Rect *r1, Rect *r2, Rect *min_bound) {
    if (r1->ulx >= r2->ulx) {
        min_bound->ulx = r2->ulx;
    } else {
        min_bound->ulx = r1->ulx;
    }

    if (r1->uly >= r2->uly) {
        min_bound->uly = r2->uly;
    } else {
        min_bound->uly = r1->uly;
    }

    if (r1->lrx <= r2->lrx) {
        min_bound->lrx = r2->lrx;
    } else {
        min_bound->lrx = r1->lrx;
    }

    if (r1->lry <= r2->lry) {
        min_bound->lry = r2->lry;
    } else {
        min_bound->lry = r1->lry;
    }
}

int32_t rect_inside_bound(Rect *r1, Rect *bound, Rect *r2) {
    int32_t result;

    *r2 = *r1;

    if (r1->ulx <= bound->lrx && r1->lrx >= bound->ulx && r1->uly <= bound->lry && r1->lry >= bound->uly) {
        if (bound->ulx > r1->ulx) {
            r2->ulx = bound->ulx;
        }
        if (r1->lrx > bound->lrx) {
            r2->lrx = bound->lrx;
        }
        if (r1->uly < bound->uly) {
            r2->uly = bound->uly;
        }
        if (r1->lry > bound->lry) {
            r2->lry = bound->lry;
        }

        result = 0;
    } else {
        result = -1;
    }

    return result;
}

Rect *rect_init(Rect *r, int32_t ulx, int32_t uly, int32_t lrx, int32_t lry) {
    r->ulx = ulx;
    r->uly = uly;
    r->lrx = lrx;
    r->lry = lry;

    return r;
}

int32_t rect_get_width(Rect *r) { return r->lrx - r->ulx; }

int32_t rect_get_height(Rect *r) { return r->lry - r->uly; }
