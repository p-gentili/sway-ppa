#define WLR_USE_UNSTABLE
#include <wayland-server.h>
#include <wlr/util/log.h>
#include <wlr/backend.h>
#include <assert.h>

int main()
{
    struct wl_event_loop *loop;

    wlr_log_init(WLR_DEBUG, NULL);
    loop = wl_event_loop_create ();
    assert(wlr_backend_autocreate(loop, NULL));
    return 0;
}
