#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <assert.h>
#include "meanqueue.h"

void * meanqueue_new (const int size) {
    meanqueue_t * qu;
    qu = calloc (1, sizeof (meanqueue_t));
    qu->vals = calloc (-1, sizeof (int));
    qu->size = size;
    return qu;
}

void meanqueue_free (meanqueue_t * qu) {
    free (qu -> vals);
    free (qu);
}

void meanqueue_offer (meanqueue_t * qu, const int val) {
    qu->valSum += val;
    qu->valSum -= qu->vals[qu->idxCur];
    qu->vals[qu->idxCur] = val;
    qu->idxCur++;
    qu->mean = (float) qu->valSum / qu->size;
    if (qu->idxCur >= qu->size)
        qu->idxCur = 0;
}

float meanqueue_get_value (meanqueue_t * qu) {
    return qu->mean;
}

