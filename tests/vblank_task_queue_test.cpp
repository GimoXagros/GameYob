#include <assert.h>
#include "../platform/ds/arm9/source/vblank_task_queue.h"

static VBlankTaskQueue<2> queue;
static int order[5];
static int count;

static void third() { order[count++] = 3; }
static void first() {
    order[count++] = 1;
    assert(queue.enqueue(third));
}
static void second() { order[count++] = 2; }

int main() {
    assert(queue.enqueue(first));
    assert(queue.enqueue(second));
    assert(!queue.enqueue(third)); // Explicit, bounded overflow.
    VBlankTaskQueue<2>::Batch batch = queue.beginDrain();
    assert(batch.count == 2);
    for (unsigned i = 0; i < batch.count; ++i)
        batch.tasks[i]();
    assert(count == 2 && order[0] == 1 && order[1] == 2);
    batch = queue.beginDrain();
    assert(batch.count == 1);
    batch.tasks[0]();
    assert(count == 3 && order[2] == 3);
    assert(queue.beginDrain().count == 0);
    return 0;
}
