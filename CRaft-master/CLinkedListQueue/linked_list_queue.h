typedef struct llqnode_s llqnode_t;

struct llqnode_s
{
    llqnode_t *next;
    void *item;
};

typedef struct
{
    llqnode_t *head, *tail;
    int count;
} linked_list_queue_t;

void *llqueue_new(
);

void llqueue_free(
    linked_list_queue_t * qu
);

void *llqueue_poll(
    linked_list_queue_t * qu
);

void llqueue_offer(
    linked_list_queue_t * qu,
    void *item
);

void *llqueue_remove_item(
    linked_list_queue_t * qu,
    const void *item
);

int llqueue_count(
    const linked_list_queue_t * qu
);

void *llqueue_remove_item_via_cmpfunction(
    linked_list_queue_t * qu,
    const void *item,
    int (*cmp)(const void*, const void*));

void *llqueue_get_item_via_cmpfunction(
    linked_list_queue_t * qu,
    const void *item,
    long (*cmp)(const void*, const void*));
